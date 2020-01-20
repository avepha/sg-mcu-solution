/* There are still bugs in the system
 *  - when checksum = stop or start byte, system will be crashed.
 *  - we still can not identify whenever sensor is inactive (currently 0 is identify as inactive sensor -> not good solution)
 *  TODO: revise all logic again!!!
 *  TODO: set sensor station in build flag
 */

#define SLAVE_ID 16
#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_TIMECRITICAL
#define _TASK_PRIORITY
#define VERSION "1.0.0"

#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TaskScheduler.h>
#include <FastCRC.h>
#include "util/converter.h"
#include "./config/config.h"
#include "./util/packetUtil.h"

#include "par.h"

#define DIR_485_PIN 8
FastCRC16 crc16;
SoftwareSerial outletPort(SG_SENSOR_RX, SG_SENSOR_TX);

Scheduler schCom, schMain;

float ec = 0;
float ph = 0;
float waterTemperature = 0;


void getEc() {
  uint16_t rawEcAnalog = analogRead(EC_PIN);
  ec = 1.5;
}

void getpH() {
  uint16_t rawpHAnalog = analogRead(EC_PIN);
  ph = 6.5;
}

void getWaterTemperature() {
  waterTemperature = 25;
}

Task tGetTemperature(2000L, TASK_FOREVER, &getEc, &schMain, true);
Task tGetHumidity(2050L, TASK_FOREVER, &getpH, &schMain, true);
Task tGetSoilTemperature(2200L, TASK_FOREVER, &getWaterTemperature, &schMain, true);

void fPrintSensor() {
  uint32_t sensors[3];
  sensors[0] = ec;
  sensors[1] = ph;
  sensors[2] = waterTemperature;

  Serial.print("read sensor:");
  for (int i = 0 ; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
    Serial.print(" ");
    Serial.print(String((float)sensors[i]));
  }
  Serial.println();
}
Task tPrintSensor(2000L, TASK_FOREVER, &fPrintSensor, &schMain, true);

void fCheckRequestAndResponse() {
  if (!outletPort.available()) {
    return;
  }

  byte requestPacket[50];
  uint16_t size = 0;
  uint32_t timestamp = 0;
  while (true) {
    if (outletPort.available()) {
      requestPacket[size] = outletPort.read();
      size++;

      uint32_t timeDiff = micros() - timestamp;
      if (timeDiff > 1500 && timeDiff < 4000) {
        Serial.println("[Error] invalid packet, 1.5ms > timediff < 4ms");
      }

      timestamp = micros();
    }
    else {
      if (size == 0) {
        break;
      }

      uint32_t timeDiff = micros() - timestamp;
      if (size > 0 && timeDiff >= 4000) {
        if (requestPacket[0] != SLAVE_ID) {
          break;
        }

        Serial.print("[Info] Got data: ");
        printBytes(requestPacket, size);

        byte crcByte[2] = {requestPacket[size - 2], requestPacket[size - 1]};
        uint16_t packetCrc;
        memcpy(&packetCrc, crcByte, sizeof(packetCrc));

        byte data[size - 4];
        memcpy(data, &requestPacket[2], sizeof(data));

        uint16_t recalCrc = crc16.ccitt(data, sizeof(data));

        if (recalCrc != packetCrc) {
          // crc is not match
          // response error
          Serial.println("[Error] Crc is not match");
          break;
        }
        else {
          Serial.println("[Info] Got valid packet, func: " + String(requestPacket[1], HEX));
        }

        byte funcCode = requestPacket[1];
        switch (funcCode) {
          case 0x04: {
            uint32_t sensors[3];
            sensors[0] = ec;
            sensors[1] = ph;
            sensors[2] = waterTemperature;
#ifdef SG_TEST
            sensors[0] = 1.5;
            sensors[1] = 6.5;
            sensors[2] = 25;
#endif
            // response sensors
            byte packets[100];
            byte data[100];
            data[0] = 0x01; // 0x01 = type gsensor
            uint16_t dataIndex = 1;
            for (uint16_t i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
              memcpy(&data[dataIndex], &sensors[i], sizeof(sensors[i]));
              dataIndex += 4;
            }

            uint16_t packetSize = generatePacket(packets, SLAVE_ID, 0x04, data, sizeof(sensors));
            digitalWrite(SG_SENSOR_DIR, SG_SENSOR_SEND_MODE);
            outletPort.write(packets, packetSize);
            digitalWrite(SG_SENSOR_DIR, SG_SENSOR_RECV_MODE);

            Serial.print("[Info] write data: ");
            printBytes(packets, packetSize);

            break;
          }
          case 0x17: {
            // response slave id
            byte packets[100];
            byte data[100] = {SLAVE_ID};

            uint16_t packetSize = generatePacket(packets, SLAVE_ID, 0x04, data, 4);
            outletPort.write(packets, packetSize);
          }
        }

        size = 0;
      }
    }
  }
}
Task tCheckRequestAndResponse(50, TASK_FOREVER, &fCheckRequestAndResponse, &schCom, true);

void setup() {
  analogReference(EXTERNAL);
  pinMode(DIR_485_PIN, OUTPUT);
  digitalWrite(DIR_485_PIN, HIGH);
  Wire.begin();
  Serial.begin(9600);
  outletPort.begin(9600);

  schMain.setHighPriorityScheduler(&schCom);

  Serial.println("initializing...");
  Serial.println("BUILD VERSION: " + String(VERSION));
}

void loop() {
  schMain.execute();
}
