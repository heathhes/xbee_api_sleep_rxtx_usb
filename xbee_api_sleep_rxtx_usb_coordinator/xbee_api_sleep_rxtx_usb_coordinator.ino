#include "SoftwareSerial.h"
#include "millisDelay.h"
#include "version.h"
#include "Xbee_lib.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define LED_PIN    13
#define WAKE_PIN    3
#define DS180_TEMP  4

SoftwareSerial ss(7,8);  // (rx,tx)
Xbee_lib m_xbee(&ss);

millisDelay m_system_timer;

OneWire oneWire(DS180_TEMP);
DallasTemperature sensor(&oneWire);

uint8_t m_tx_count[6] = {}; // number of xbees in network

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

  // Start up the library for dallas temp
  sensor.begin();
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

uint8_t Message_received(const struct Msg_data rx_data)
{
  ss.println("Message_received");
  m_xbee.Print_msg(rx_data);

  // build message, insert payloads
  struct Msg_data tx_msg;
  tx_msg.length = 23;
  tx_msg.frame_type = 0x10;
  tx_msg.address = rx_data.address;
  tx_msg.payload_cnt = m_tx_count[rx_data.address];
  tx_msg.payload_id = 0xD1;

  // payload
  tx_msg.payload[0] = 0x99;
  tx_msg.payload[1] = 0x88;
  tx_msg.payload[2] = 0x77;

  // use enum from transmit status
  uint8_t tx_ok = m_xbee.Transmit_data(tx_msg);
  if(tx_ok == 1)
  {
    m_tx_count[rx_data.address]++;
  }
}

//////////////////////////////////////////////////////////////////////

uint8_t getDallasTemp()
{
  delay(10);
  sensor.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensor.getTempCByIndex(0);
  float tempF = (sensor.getTempCByIndex(0) * 9.0 / 5.0) + 32;

  if(tempF < 0)
  {
    tempF = 0;
  }
  uint8_t temp2dac = tempC * 4; //can only transmit byte so save resolution

  return tempF;
}
