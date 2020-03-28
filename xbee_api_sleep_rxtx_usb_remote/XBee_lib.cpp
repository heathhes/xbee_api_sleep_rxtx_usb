#include "XBee_lib.h"
#include "setup.h"

XBee_lib::XBee_lib()
{
  m_id = 0;
  m_address = 0;
  m_checksum = 0;
  m_frame_length = 23;
  m_payload[5] = {};
};

////////////////////////////////////////////////////////////

uint8_t XBee_lib::Get_checksum(uint8_t frame[], uint8_t length)
{
  long sum = 0;
  for(int i = 3; i < (length - 1); i++){
    sum += frame[i];
  }
  uint8_t check_sum = 0xFF - (sum & 0xFF);
  return check_sum;
};

////////////////////////////////////////////////////////////

void XBee_lib::set_address(uint8_t frame[], uint8_t xbee)
{
  frame[5] = ADDR_B1;
  frame[6] = ADDR_B2;
  frame[7] = ADDR_B3;
  frame[8] = ADDR_B4;
  frame[9] = ADDR_B5;
  frame[10] = ADDR_B6;
  frame[11] = ADDR_B7;
  frame[11] = ADDR_B8;
};

////////////////////////////////////////////////////////////

uint8_t XBee_lib::get_address(uint8_t address_byte)
{
  int xbee = 0;
  switch(address_byte)
  {
    case 0x79:
      xbee = 1;
      break;
    case 0x81:
      xbee = 2;
      break;
    case 0x93:
      xbee = 3;
      break;
    case 0x8D:
      xbee = 4;
      break;
    case 0x8E:
      xbee = 5;
      break;
    case 0x95:
      xbee = 6;
      break;
    default:
      xbee = 0;
      break;
  }
  return xbee;
};

////////////////////////////////////////////////////////////

void XBee_lib::fill_payload(uint8_t payload[])
{
  m_frame[17] = payload[0];
  m_frame[18] = payload[1];
  m_frame[19] = payload[2];
  m_frame[20] = payload[3];
  m_frame[21] = payload[4];
};

////////////////////////////////////////////////////////////

void XBee_lib::get_payload(uint8_t payload[])
{

};

////////////////////////////////////////////////////////////

void XBee_lib::clear_array(uint8_t array[], uint8_t len)
{
  for(int i = 0; i < len; i++)
  {
    array[i] = 0;
  }
};
