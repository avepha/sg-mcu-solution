#include <Arduino.h>

#ifndef SG_MCU_ARR_PRINT_H
#define SG_MCU_ARR_PRINT_H

void arr_print(const String &topic, byte *arr, uint16_t size) {
  Serial.print("[" + topic + "]: ");
  for (uint16_t i = 0; i < size; i++) {
    Serial.print(arr[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

#endif //SG_MCU_ARR_PRINT_H
