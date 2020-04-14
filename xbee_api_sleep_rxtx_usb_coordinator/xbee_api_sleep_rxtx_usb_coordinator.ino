#include "SoftwareSerial.h"
#include "millisDelay.h"
#include "version.h"
#include "Xbee_lib.h"

#define LED_PIN   13
#define WAKE_PIN   3

SoftwareSerial ss(7,8);  //(rx,tx)
Xbee_lib m_xbee(&ss);

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
  // allow time to switch to xbee mode on pcb
  delay(2000);

  Serial.begin(19200);
  Serial.println("**** SERIAL ****");

  ss.begin(19200);   //m_xbee.Begin(19200);
  ss.print("xbee_api_sleep_txrx_usb_coordinator : ");
  ss.println(version);

  // pin definitions
  pinMode(LED_PIN, OUTPUT);
  pinMode(WAKE_PIN, INPUT);

  // delay for handling wireless interface
  m_system_timer.start(25);

  // callback for when valid data received
  m_xbee.Set_callback(Message_received);
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
  while(Serial.available())
  {
    m_xbee.Process_byte(Serial.read());
  }
}

//////////////////////////////////////////////////////////////////////

uint8_t Message_received(const struct Msg_data data)
{
  ss.println("Message_received");
  m_xbee.Print_msg(data, sizeof(data.payload));

  m_tx_array[17] = m_tx_count[data.address];

  // use enum from transmit status
  uint8_t tx_ok = m_xbee.Transmit_data(m_tx_array,
                                       sizeof(m_tx_array),
                                       data.address);
  if(tx_ok == 1)
  {
    m_tx_count[data.address]++;
  }
}


