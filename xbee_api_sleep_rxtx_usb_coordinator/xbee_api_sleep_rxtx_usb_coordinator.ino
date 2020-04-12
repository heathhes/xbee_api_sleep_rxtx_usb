#include "LowPower.h"
#include "setup_coordinator.h".h"
#include "SoftwareSerial.h"
#include "millisDelay.h"
#include "version.h"
#include "Xbee_lib.h"

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

  ss.begin(19200);
  Serial.begin(19200);
  Serial.println("**** SERIAL ****");
  ss.print("xbee_api_sleep_txrx_usb_coordinator : ");
  ss.println(version);

  pinMode(LED_PIN, OUTPUT);

  // delay for handling wireless interface
  m_system_timer.start(100);

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
  uint8_t x = 0;
  uint8_t rx_array[50] = {};

  while(Serial.available())
  {
    rx_array[x] = Serial.read();
    x++;
    new_rx = true;
  }

  if(new_rx)
  {
    delay(10);
    ss.println("");
    ss.print("Rx'd buffer: ");
    print_array(rx_array, sizeof(rx_array));

    struct Msg_data rx_data = m_xbee.Process_received(rx_array,
                                                      sizeof(rx_array));
    if(rx_data.valid)
    {
      ss.println("Rx'd valid frame, respond = true");
      print_msg(rx_data);
      print_array(rx_data.payload, sizeof(rx_data.payload));
      respond = true;
    }
    else
    {
      ss.print("RECEIVED INVALID FRAME: ");
      print_array(rx_data.payload, sizeof(rx_data.payload));
    }
  }


  // tx data back to xbee
  if(respond)
  {
    // insert payloads
    tx_array[17] = m_tx_count;

    m_tx_count = m_xbee.Transmit_data(tx_array,
                                      sizeof(tx_array),
                                      ID::XBEE_3);
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

//////////////////////////////////////////////////////////////////////

void print_msg(struct Msg_data msg)
{
  ss.print("Address: ");
  ss.println(msg.address, HEX);
  ss.print("Count: ");
  ss.println(msg.count, HEX);
  ss.print("Type_id: ");
  ss.println(msg.type_id, HEX);
  ss.print("Payload: ");
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
