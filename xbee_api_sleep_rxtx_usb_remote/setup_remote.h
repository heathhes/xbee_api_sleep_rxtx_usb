/*
  setup.h - Library for setting up xbee network and 328 base pcb
  interrupt pin, baudrate, software serial pins
*/
#ifndef setup_h
#define setup_h

// address of destination
#define ADDR_B1  0x00
#define ADDR_B2  0x13
#define ADDR_B3  0xA2
#define ADDR_B4  0x00
#define ADDR_B5  0x41
#define ADDR_B6  0x25
#define ADDR_B7  0xA4
#define ADDR_B8  0x79

enum ID
{
  XBEE_X = 0,
  XBEE_1 = 1,
  XBEE_2 = 2,
  XBEE_3 = 3,
  XBEE_4 = 4,
  XBEE_5 = 5,
  XBEE_6 = 6
};

enum RX_STATUS
{
  SUCCESS = 0,
  FAILED = 1
};

// all addresses  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF // broadcast
// xbee1 address  0x00, 0x13, 0xA2, 0x00, 0x41, 0x25, 0xA4, 0x79 // sleep coordinator
// xbee2 address  0x00, 0x13, 0xA2, 0x00, 0x41, 0x25, 0xA4, 0x81
// xbee3 address  0x00, 0x13, 0xA2, 0x00, 0x41, 0x4E, 0x65, 0x93
// xbee4 address  0x00, 0x13, 0xA2, 0x00, 0x41, 0x4E, 0x65, 0x8D
// xbee5 address  0x00, 0x13, 0xA2, 0x00, 0x41, 0x4E, 0x65, 0x8E
// xbee6 address  0x00, 0x13, 0xA2, 0x00, 0x41, 0x25, 0xA4, 0x95

#define SOM      0  // byte 0
#define MSB_LEN  1  // byte 1
#define LSB_LEN  2
#define CMD_ID   3  // 
#define ACK      4  // 0x00 is n ack, 0x8B is received ack
#define DEST_0   5  // MSB
#define DEST_1   6
#define DEST_2   7
#define DEST_3   8
#define DEST_4   9
#define DEST_5   10
#define DEST_6   11
#define DEST_7   12
#define RESV_1   13
#define RESV_2   14
#define BRDCST_RAD 15  // number of hops for broadcast transmission 0x00 is max
#define TX_OPT     16  // transmit options, bit pack 0 = Disable Ack, 1=Disable RD, 2=NACK, 3=Trace Route
// 17 - n is RF packet bytes
// last byte is checksum

#endif
