#include <Arduino.h>

void printBytes(byte* values, uint16_t size) {
  for (int i = 0 ; i < size; i++) {
    Serial.print(values[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void floatToBytes(float val, byte *bytes_array)
{
    union {
        float float_variable;
        byte temp_array[4];
    } u;
    u.float_variable = val;
    memcpy(bytes_array, u.temp_array, 4);
}

void Uint16ToBytes(uint16_t val, byte *bytes_array) {
  union {
    uint16_t variable;
    byte temp_array[2];
  } u;
  u.variable = val;
  memcpy(bytes_array, u.temp_array, 2);
}

uint16_t BytesToUint16(byte* bytes_array) {
  union {
    uint16_t variable;
    byte temp_array[2];
  } u;
  memcpy(u.temp_array, bytes_array, 2);
  return u.variable;
}

byte modsum(byte *payload, byte payloadSize) {
  int sum = 0;
  for(int i = 0; i < payloadSize; i++) {
    sum += payload[i];
  }

  return sum % 255;
}