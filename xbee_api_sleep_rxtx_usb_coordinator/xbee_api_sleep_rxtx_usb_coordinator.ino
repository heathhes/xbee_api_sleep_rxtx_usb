#include "LowPower.h"
#include "setup_coordinator.h".h"
#include "SoftwareSerial.h"
#include "millisDelay.h"
#include "version.h"
#include "Xbee_lib.h"
#include "Xbee_lib_defs.h"

#define RX_MSG_SIZE  21 // payload 5
#define TX_MSG_SIZE  23 // payload 5
#define LED_PIN   13

SoftwareSerial ss(7,8);  //(rx,tx)
Xbee_lib m_xbee(ID::XBEE_1); // id
millisDelay m_system_timer;
uint8_t m_tx_count = 0;

uint8_t tx_array[] = {0x7E, 0x00, 0x13, 0x10, 0x00,
                      ADDR_B1, ADDR_B2, ADDR_B3,
                      ADDR_B4, ADDR_B5, ADDR_B6,
                      ADDR_B7, ADDR_B8,
                      0xFF, 0xFE, 0x00, 0x00, 0x11,
                      0x99, 0x88, 0x77, 0x66, 0xB6};  // 23 bytes

//////////////////////////////////////////////////////////////////////
 
void setup() 
{  
  delay(3000);
  
  ss.begin(9600);
  Serial.begin(19200);
  Serial.println("**** SERIAL ****");
  ss.print("ss: xbee_api_sleep_txrx_usb_coordinator : ");
  ss.println(version);
  
  pinMode(LED_PIN, OUTPUT); 

  // delay for handling wireless interface
  m_system_timer.start(200);
  
}  

//////////////////////////////////////////////////////////////////////

void loop() 
{ 
  if(m_system_timer.justFinished())
  {
    m_system_timer.repeat();
    handle_wireless();
  }
}

//////////////////////////////////////////////////////////////////////

void handle_wireless()
{  
  bool respond = false;
  bool new_rx = false;
  uint8_t rx_array[50] = {};
  uint8_t rx_msg_array[21] = {};



  while(Serial.available())
  {
    static uint8_t x = 0;
    rx_array[x] = Serial.read();
    x++;
    new_rx = true;
  } // delay required?

  if(new_rx)
  {
    bool rx_status = m_xbee.Process_received(rx_array,
                                             sizeof(rx_array),
                                             rx_msg_array,
                                             sizeof(rx_msg_array));
    if(rx_status)
    {
      print_array(rx_msg_array, sizeof(rx_msg_array));
      ss.println("Rx'd valid frame, responding");
      respond = true;
    }
    else
    {
      ss.print("Received invalid frame: ");
      print_array(rx_msg_array, sizeof(rx_msg_array));
    }
  }

  // insert payloads
  tx_array[17] = m_tx_count;  

  
  // tx data back to xbee
  if(respond)
  {
    m_tx_count = m_xbee.Transmit_data(tx_array, sizeof(tx_array), ID::XBEE_3);
    respond = false;
  }


  // clear the rx array
  m_xbee.Clear_array(rx_array, sizeof(rx_array));
}

//////////////////////////////////////////////////////////////////////

void print_array(uint8_t array[], uint8_t len)
{
  for(int i = 0; i < len; i++)
  {
    ss.print(array[i],HEX);
    ss.print(", ");
  } 
  ss.println();
}

// check to see if any received messages were missed
//      static uint8_t m_last_rx_count = 0;
//    if((m_last_rx_count + 1) == rx_msg_array[15])
//    {
//      m_last_rx_count = rx_msg_array[15];
//    }
//    else
//    {
//      ss.print("MISSED RX MESSAGE: local count | received count : ");
//      ss.print(m_last_rx_count);
//      ss.print(" | ");
//      ss.println(rx_msg_array[15]);
//      m_last_rx_count = rx_msg_array[15];
//    }
