#include "par.h"

Par::Par(int pin) : pin(pin) {
  pinMode(pin, INPUT);
}

float Par::getPar() {
  int raw = analogRead(pin);
  float k = 2.514/1023; // convert to volt (Volt/ADC)
  float j = 1000; // sensitivity = 1000(Watt/Volt)

  return (float)raw*j*k;
}