#ifndef XBee_lib_h
#define XBee_lib_h

#include "Arduino.h"

class XBee_lib
{
public:
  XBee_lib();
  uint8_t Get_checksum(uint8_t frame[], uint8_t len);
  void set_address(uint8_t frame[], uint8_t xbee);
  uint8_t get_address(uint8_t address_byte);
  void fill_payload(uint8_t payload[]);
  void get_payload(uint8_t payload[]);
  void reset_payload();
  void reset_frame();
  void clear_array(uint8_t array[], uint8_t len);
protected:

private:
  uint8_t m_id;
  uint8_t m_address;
  uint8_t m_checksum;
  uint8_t m_frame_length;
  uint8_t m_payload_length;
  uint8_t m_frame[];
  uint8_t m_payload[];

};
#endif
