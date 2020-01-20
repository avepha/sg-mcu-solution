//
// Created by Alfarie-MBP on 2019-08-15.
//
#include <Arduino.h>
#ifndef SG_MCU_SENSOR_PAR_H
#define SG_MCU_SENSOR_PAR_H

class Par {
public:
  explicit Par(int pin);

  float getPar();
private:
  int pin;
};




#endif //SG_MCU_SENSOR_PAR_H
