extern FastCRC16 crc16;
#ifndef SG_MCU_SENSOR_PACKETUTIL_H
#define SG_MCU_SENSOR_PACKETUTIL_H

uint16_t generatePacket(byte* packets, byte station, byte func, byte *data, uint8_t dataSize) {
  packets[0] = station;
  packets[1] = func;
  memcpy(&packets[2], data, dataSize);
  uint16_t crc = crc16.modbus(packets, dataSize + 2);
  packets[dataSize + sizeof(station) + sizeof(func)] = crc & 0xff;
  packets[dataSize + sizeof(station) + sizeof(func) + 1] = crc >> 8;
  return dataSize + sizeof(station) + sizeof(func) + 2;
}

#endif
