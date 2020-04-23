#define SLAVE_ID 16
#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_TIMECRITICAL
#define _TASK_PRIORITY
#define VERSION "1.1.6"

#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TaskScheduler.h>
#include <FastCRC.h>
#include "util/converter.h"
#include "./config/config.h"
#include "./util/packetUtil.h"

#define DIR_485_PIN 8
FastCRC16 crc16;
SoftwareSerial outletPort(SG_STATION_RX, SG_STATION_TX);

Scheduler schCom, schMain;

float ec = 0;
float ph = 0;
float waterTemperature = 0;


void getEc() {
#ifdef SG_TEST
  ec = (float)random(1000, 1500) / 1000;
#else
  uint16_t rawEcAnalog = analogRead(EC_PIN);
  ec = 2.538045247 * rawEcAnalog - 500;
  if (ec <= 0) {
    ec = 0;
  }
#endif
}

void getpH() {
#ifdef SG_TEST
  ph = (float)random(6500, 7000) / 1000;
#else
  uint16_t rawpHAnalog = analogRead(PH_PIN);
  ph = 0.017766317 * rawpHAnalog - 3.5;
#endif
}

void getWaterTemperature() {
#ifdef SG_TEST
  waterTemperature = (float)random(2500, 2600) / 100;
#else
  waterTemperature = (float)random(2500, 2600) / 100;
#endif
}

Task tGetTemperature(2000L, TASK_FOREVER, &getEc, &schMain, true);
Task tGetHumidity(2050L, TASK_FOREVER, &getpH, &schMain, true);
Task tGetSoilTemperature(2200L, TASK_FOREVER, &getWaterTemperature, &schMain, true);

void fPrintSensor() {
  float sensors[3];
  sensors[0] = ec;
  sensors[1] = ph;
  sensors[2] = waterTemperature;

  Serial.print("[Info] Read sensor:");
  for (int i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
    Serial.print(" ");
    Serial.print(String((float) sensors[i]));
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
      if (timeDiff > 3000 && timeDiff < 4000) {
        Serial.println("[Error] invalid packet, 1.5ms > timediff < 4ms");
      }

      timestamp = micros();
    }
    else {
      if (size == 0) {
        break;
      }
      uint32_t timeDiff = micros() - timestamp;
      if (timeDiff >= 4000) {
        if (size > 7) {
          byte packet[8];
          memcpy(packet, requestPacket, 8);
          if (packet[0] != SLAVE_ID) {
            break;
          }

          byte crcByte[2] = {packet[sizeof(packet) - 2], packet[sizeof(packet) - 1]};
          uint16_t packetCrc;
          memcpy(&packetCrc, crcByte, sizeof(packetCrc));

          uint16_t recalCrc = crc16.modbus(packet, sizeof(packet) - 2);

          if (recalCrc != packetCrc) {
            // crc is not match
            // response error
            Serial.println("[Error] Crc is not match");
            break;
          }
          else {
            Serial.println("[Info] Got valid packet, func: " + String(packet[1], HEX));
          }

          byte funcCode = packet[1];
          switch (funcCode) {
            case 0x04: {
              float sensors[3];
              sensors[0] = ec;
              sensors[1] = ph;
              sensors[2] = waterTemperature;
              // response sensors
              byte packets[100];
              byte data[100];
              uint16_t dataIndex = 0;
              for (uint16_t i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
                memcpy(&data[dataIndex], &sensors[i], sizeof(sensors[i]));
                dataIndex += 4;
              }

              uint16_t packetSize = generatePacket(packets, SLAVE_ID, 0x04, data, sizeof(sensors));
              digitalWrite(SG_STATION_DIR_PIN, RS485_SEND_MODE);
              outletPort.write(packets, packetSize);
              digitalWrite(SG_STATION_DIR_PIN, RS485_RECV_MODE);

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
        }

        size = 0;
      }
    }
  }
}

Task tCheckRequestAndResponse(50, TASK_FOREVER, &fCheckRequestAndResponse, &schCom, true);

void setup() {
  analogReference(EXTERNAL);
  pinMode(SG_STATION_DIR_PIN, OUTPUT);
  pinMode(SG_STATION_RX, INPUT);
  pinMode(SG_STATION_TX, OUTPUT);

  digitalWrite(SG_STATION_DIR_PIN, RS485_RECV_MODE);
  digitalWrite(SG_STATION_TX, HIGH);

  Wire.begin();
  Serial.begin(9600);
  outletPort.begin(9600);

  schMain.setHighPriorityScheduler(&schCom);

  Serial.println("initializing...");
  Serial.println("BUILD VERSION: " + String(VERSION));
#ifdef SG_TEST
  Serial.println("OP MODE: TEST");
#endif
}

void loop() {
  schMain.execute();
}
