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

millisDelay m_wireless_timer;
millisDelay m_usb_timer;
millisDelay m_remote_status_timer;

uint8_t m_usb_state = 0;
uint8_t m_usb_count = 0;
struct Msg_data m_rx_usb_msg;
uint8_t m_tx_addr = 0;
uint8_t j = 0;

struct Msg_data m_tx_msg[5];
struct Msg_data m_rx_msg[5];

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
  m_wireless_timer.start(1);

  // delay for handling usb interface
  m_usb_timer.start(1);

  // delay for reporting status
  m_remote_status_timer.start(5000);

  // callback for when valid data received
  m_xbee.Set_callback(Message_received);

  // Start up the library for dallas temp
  sensor.begin();
}

//////////////////////////////////////////////////////////////////////

void loop()
{
  if(m_wireless_timer.justFinished())
  {
    m_wireless_timer.repeat();
    handle_wireless();
  }

  if(m_usb_timer.justFinished())
  {
    m_usb_timer.repeat();
    handle_usb();
  }

  if(m_remote_status_timer.justFinished())
  {
    m_remote_status_timer.repeat();
    handle_remote_status();
  }
}


void handle_remote_status()
{
  ss.println("Handle remote status");
  for(int i = 0; i < 5; i++)
  {
    if(m_rx_msg[i].valid)
    {
      ss.print("Remote Address: ");
      ss.println(m_rx_msg[i].address);
      ss.print("Light[%]: ");
      uint8_t light = map(m_rx_msg[i].payload[0], 0, 255, 0, 100);
      ss.println(light);
      ss.print("Temperature[F]: ");
      ss.println(m_rx_msg[i].payload[1]);
      ss.print("Battery[V]: ");
      float battery = m_rx_msg[i].payload[2] * 8 * 0.003157; // 0.0032258 @ 3.3V
      ss.println(battery);
      ss.println();
    }
  }
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//////////////////////////////////////////////////////////////////////

void handle_usb()
{
  while(ss.available())
  {
    process_usb_byte(ss.read()); // s3o3611
  }
}

void process_usb_byte(uint8_t rx_byte)
{
  switch(m_usb_state)
  {
    case 0:
      if(rx_byte == 's')
      {
        reset_usb();
        m_usb_state = 1;
      }
    break;

    case 1 :
      m_tx_addr = (rx_byte - '0');
      m_rx_usb_msg.address = m_tx_addr;
      m_usb_state = 2;
      break;

    case 2 :
      m_rx_usb_msg.payload_id = rx_byte;
      m_usb_state = 3;
      break;

    case 3 :
      m_rx_usb_msg.payload_len = (rx_byte - '0');
      m_usb_state = 4;
      break;

    case 4 :
      m_rx_usb_msg.payload[j] = (rx_byte - '0');
      j++;
      if(j == 3)
      {
        m_usb_state = 5;
      }
      break;

    case 5 :
      update_tx_msg(m_rx_usb_msg);
      reset_usb();
      break;

    default :
      reset_usb();
  }
}

void update_tx_msg(const struct Msg_data usb_data)
{
  ss.println("USB message rx'd");
  m_xbee.Print_msg(usb_data, false);
  m_tx_msg[usb_data.address] = usb_data;
}

void reset_usb()
{
  m_tx_addr = 0;
  m_usb_count = 0;
  m_usb_state = 0;
  j = 0;
  m_xbee.Clear_msg(m_rx_usb_msg);
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

  switch(rx_data.payload_id)
  {
    case CMD_ID::NO_ACK :
      ss.println("Rx'd No Ack");
      // do nothing, no response
      break;

    case CMD_ID::ACK :
      ss.println("Rx'd Ack");
      // build response
      break;

    case CMD_ID::IO_IN :
      ss.println("Rx'd IO in");

      // update rx data container
      for(int i = 0; i < sizeof(rx_data.payload); i++)
      {
        m_rx_msg[rx_data.address] = rx_data;
      }
      //build response

      break;

    default :
      ss.print("Unknown CMD::ID: ");
      ss.println(rx_data.payload_id);
  }


  // build message, insert payloads
  struct Msg_data tx_msg;
  tx_msg.length = 23;
  tx_msg.frame_type = 0x10;
  tx_msg.address = rx_data.address;
  tx_msg.payload_len = m_tx_msg[tx_msg.address].payload_len;
  tx_msg.payload_cnt = m_tx_count[rx_data.address];
  tx_msg.payload_id = m_tx_msg[tx_msg.address].payload_id;;

  // payload
  tx_msg.payload[0] = m_tx_msg[tx_msg.address].payload[0];
  tx_msg.payload[1] = m_tx_msg[tx_msg.address].payload[1];
  tx_msg.payload[2] = m_tx_msg[tx_msg.address].payload[2];

  // use enum from transmit status
  uint8_t tx_ok = m_xbee.Transmit_data(tx_msg);
  if(tx_ok == 1)
  {
    m_tx_count[rx_data.address]++;
  }

  // if only meant to send this once
  if(true)
  {
    m_xbee.Clear_msg(m_tx_msg[tx_msg.address]);
  }
}

//////////////////////////////////////////////////////////////////////

uint8_t getDallasTemp()
{
  delay(10);
  sensor.requestTemperatures(); // Send the command to get temperatures
  float tempF = (sensor.getTempCByIndex(0) * 9.0 / 5.0) + 32;

  if(tempF < 0)
  {
    tempF = 0;
  }

  return tempF;
}
