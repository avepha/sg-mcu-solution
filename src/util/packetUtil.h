extern FastCRC16 crc16;
#ifndef SG_MCU_SENSOR_PACKETUTIL_H
#define SG_MCU_SENSOR_PACKETUTIL_H

uint16_t generatePacket(byte *packets, byte station, byte func, byte *data, uint8_t dataSize) {
  packets[0] = station;
  packets[1] = func;
  packets[2] = dataSize;
  memcpy(&packets[3], data, dataSize);
  uint16_t crc = crc16.modbus(packets, dataSize + 3);
  packets[dataSize + sizeof(station) + sizeof(func) + sizeof(dataSize)] = crc & 0xff;
  packets[dataSize + sizeof(station) + sizeof(func) + sizeof(dataSize) + 1] = crc >> 8;
  return sizeof(dataSize) + sizeof(station) + sizeof(func) + dataSize + sizeof(crc);
}

#endif
