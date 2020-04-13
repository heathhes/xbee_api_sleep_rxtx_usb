#include "SoftwareSerial.h"
#include "millisDelay.h"
#include "version.h"
#include "Xbee_lib.h"

#define LED_PIN   13
#define WAKE_PIN   3

SoftwareSerial ss(7,8);  //(rx,tx)
Xbee_lib m_xbee;

millisDelay m_system_timer;

uint8_t m_dest_addr = 0;
uint8_t m_tx_count[6] = {};
uint8_t m_tx_array[] = {0x7E, // SOM
                        0x00, // length MSB
                        0x13, // length LSB
                        0x10, // Frame Type (0x10 Tx request)
                        0x00, // Frame ID (used for ACK)
                        ADDR_B1, ADDR_B2, ADDR_B3,
                        ADDR_B4, ADDR_B5, ADDR_B6,
                        ADDR_B7, ADDR_B8,
                        0xFF, // Reserved 1
                        0xFE, // Reserved 2
                        0x00, // Broadcast radius
                        0x00, // Transmit options bit backed, (0xC1)
                        0x99, 0x88, 0x77, 0x66, 0x55,
                        0xD6};  // Checksum

//////////////////////////////////////////////////////////////////////

void setup()
{
  delay(2000);

  ss.begin(19200);
  Serial.begin(19200);
  Serial.println("**** SERIAL ****");
  ss.print("xbee_api_sleep_txrx_usb_coordinator : ");
  ss.println(version);

  pinMode(LED_PIN, OUTPUT);
  pinMode(WAKE_PIN, INPUT);

  // delay for handling wireless interface
  m_system_timer.start(25);

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
    ss.print(rx_array[x], HEX);
    ss.print(", ");

    x++;
    new_rx = true;
  }

  if(new_rx)
  {
    ss.println("");
    ss.print("Rx'd buffer: ");
    print_array(rx_array, sizeof(rx_array), true);

    struct Msg_data rx_data = m_xbee.Process_received(rx_array,
                                                      sizeof(rx_array));
    if(rx_data.valid)
    {
      ss.println("Rx'd valid frame, respond = true");
      print_msg(rx_data, sizeof(rx_data.payload));

      m_dest_addr = rx_data.address;
      respond = true;
    }
    else
    {
      ss.println();
      ss.println();
      ss.print("RECEIVED INVALID FRAME: ");
      print_array(rx_data.payload, sizeof(rx_data.payload), true);
      ss.println();
      ss.println();
    }
  }


  // tx data back to xbee
  if(respond)
  {
    // insert payloads
    m_tx_array[17] = m_tx_count[m_dest_addr];

    ss.print("Tx-ing to: ");
    ss.println(m_dest_addr);
    print_array(m_tx_array,sizeof(m_tx_array), true);
    ss.println();

    // use enum from transmit status
    uint8_t tx_ok = m_xbee.Transmit_data(m_tx_array,
                                         sizeof(m_tx_array),
                                         m_dest_addr);
    if(tx_ok == 1)
    {
      m_tx_count[m_dest_addr]++;
    }
    respond = false;
  }


  // clear the rx array
  m_xbee.Clear_array(rx_array, sizeof(rx_array));
}

//////////////////////////////////////////////////////////////////////

void print_array(uint8_t array[], uint8_t len, bool hex)
{
  if(hex)
  {
    for(int i = 0; i < len; i++)
    {
      ss.print(array[i],HEX);
      ss.print(", ");
    }
    ss.println();
  }
  else
  {
    for(int i = 0; i < len; i++)
    {
      ss.print(array[i]);
      ss.print(", ");
    }
    ss.println();
  }
}

//////////////////////////////////////////////////////////////////////

void print_msg(struct Msg_data msg, uint8_t len_payload)
{
  ss.print("Address: ");
  ss.println(msg.address, HEX);
  ss.print("Length: ");
  ss.println(msg.length, HEX);
  ss.print("Count: ");
  ss.println(msg.count, HEX);
  ss.print("Type_id: ");
  ss.println(msg.type_id, HEX);
  ss.print("Payload: ");
  print_array(msg.payload, len_payload, false);
}

