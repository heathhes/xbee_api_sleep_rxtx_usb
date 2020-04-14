#include <SoftwareSerial.h>
#include "LowPower.h"
#include "millisDelay.h"
#include "version.h"
#include "Xbee_lib.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define LED_PIN     13
#define WAKE_PIN     3
#define DS180_TEMP   4
#define VACUUM      A6
#define BATTERY     A7

SoftwareSerial ss(7,8);  //(rx,tx)
Xbee_lib m_xbee;

millisDelay m_send_timer;
millisDelay m_sleep_timer;
millisDelay m_system_timer;

OneWire oneWire(DS180_TEMP);
DallasTemperature sensor(&oneWire);

uint8_t m_parser_state = PARSE::SOM;
uint8_t m_parser_cnt = 0;
uint8_t m_msg_array[21] = {};
struct Msg_data m_msg_data;

bool m_sleep_now = true;
bool m_tx_now = true;
uint8_t m_tx_count = 0;
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
                        0x30, 0x31, 0x32, 0x33, 0x34,
                        0xD6};  // Checksum

//////////////////////////////////////////////////////////////////////
 
void setup() 
{ 
  // allow time to switch to xbee mode on pcb
  delay(2000);

  Serial.begin(19200);
  ss.begin(19200);
  ss.print("xbee_api_sleep_txrx_usb_remote : ");
  ss.println(version);

  // pin definitions
  pinMode(WAKE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // delay before transmission is sent again (no response)
  m_send_timer.start(2000);

  // sleep after 5 seconds regardless if transmission/response status
  m_sleep_timer.start(5000);

  // slow the tx-ing and rx-ing handling loop
  m_system_timer.start(25);

  // Start up the library for dallas temp
  sensor.begin();
}  

//////////////////////////////////////////////////////////////////////

void wakeUp()
{
  // called after interrupt (no delays or millis)
  // reset variables after wakeup
  m_tx_now = true;
  m_sleep_now = false;
}

//////////////////////////////////////////////////////////////////////
  
void loop() 
{ 
  // put micro to sleep
  if(m_sleep_now)
  {
    digitalWrite(LED_PIN, LOW);
    
    // attach external interrupt and then sleep
    attachInterrupt(digitalPinToInterrupt(WAKE_PIN), wakeUp, RISING);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

    //delay after wakeup
    delay(100);       
    digitalWrite(LED_PIN, HIGH);
    
    // Disable external pin interrupt on wake up pin.
    detachInterrupt(digitalPinToInterrupt(WAKE_PIN));

    m_sleep_timer.restart();
  }

  // after waking up, slow the system loop
  if(m_system_timer.justFinished())
  {
    m_system_timer.repeat();
    handle_wireless();
  }
  
  // no successful response, sleep anyway
  if(m_sleep_timer.justFinished())
  {
    ss.println("system timeout, going to sleep");
    m_sleep_now = true;
  }
 
} 

//////////////////////////////////////////////////////////////////////

void handle_wireless()
{ 
  bool new_rx = false;
  uint8_t x = 0;
  uint8_t rx_array[50] = {};

  while(Serial.available())
  {

    rx_array[x] = Serial.read();
    process_byte(rx_array[x]);
    //ss.print(rx_array[x], HEX);
    //ss.print(", ");

    x++;
    new_rx = true;
  }

  if(new_rx)
  {
    //ss.println("");
    //ss.print("Rx'd buffer: ");
    //print_array(rx_array, sizeof(rx_array), true);

//    struct Msg_data rx_data = m_xbee.Process_received(rx_array,
//                                                      sizeof(rx_array));
    struct Msg_data rx_data;

    if(rx_data.valid)
    {
      ss.println("Rx'd valid frame, sleep_now = true");
      print_msg(rx_data, sizeof(rx_data.payload));
      m_sleep_now = true;
    }
    else
    {
//      ss.println();
//      ss.println();
//      ss.print("RECEIVED INVALID FRAME: ");
//      print_array(rx_data.payload, sizeof(rx_data.payload), true);
//      ss.println();
//      ss.println();
    }
  }


  // build message, insert payloads
  m_tx_array[TX::PAYLOAD_CNT] = m_tx_count;
  m_tx_array[TX::PAYLOAD_ID] = 0xA1;

  analogRead(A2); // throw away
  uint16_t light = analogRead(A2);

  m_tx_array[TX::PAYLOAD_0] = light/4;
  m_tx_array[TX::PAYLOAD_0 + 1] = getDallasTemp();

  analogRead(A7);
  uint16_t battery = analogRead(A7);
  m_tx_array[TX::PAYLOAD_0 + 2] = battery/4;

  
  // transmit data, timer has timed out
  if(m_tx_now &&  m_send_timer.justFinished())
  {
    // use enum from transmit status
    uint8_t tx_ok = m_xbee.Transmit_data(m_tx_array,
                                         sizeof(m_tx_array),
                                         ID::XBEE_1);
    if(tx_ok == 1)
    {
      m_tx_count++;
    }

    // reset timer
    m_send_timer.repeat();
    m_tx_now = false;
  }

  // clear the rx array
  m_xbee.Clear_array(rx_array, sizeof(rx_array));

}

//////////////////////////////////////////////////////////////////////

void process_byte(uint8_t rx_byte)
{
  switch(m_parser_state)
  {
    case PARSE::SOM :
      ss.println("SOM: ");
      if(rx_byte == 0x7E)
      {
        reset_parser();
        ss.print("SOM 0x7E: ");
        ss.println(m_parser_cnt);

        m_msg_array[m_parser_cnt] = rx_byte;
        m_parser_state = PARSE::LENGTH;
        m_parser_cnt++;
      }
      break;

    case PARSE::LENGTH :
      ss.print("LENGTH: ");
      ss.println(m_parser_cnt);
      m_msg_array[m_parser_cnt] = rx_byte;
      if(m_parser_cnt == RX::LSB_LEN)
      {
        m_msg_data.length = rx_byte;
        m_parser_state = PARSE::FRAME_TYPE;
      }
      m_parser_cnt++;
      break;

    case PARSE::FRAME_TYPE :
      ss.print("FRAME_TYPE: ");
      ss.println(m_parser_cnt);
      m_msg_array[m_parser_cnt] = rx_byte;
      m_msg_data.frame_type = rx_byte;
      m_parser_state = PARSE::SOURCE;
      m_parser_cnt++;
      break;

    case PARSE::SOURCE :
      ss.print("SOURCE: ");
      ss.println(m_parser_cnt);
      m_msg_array[m_parser_cnt] = rx_byte;
      if(m_parser_cnt == RX::SRC_7)
      {
        m_msg_data.address = m_xbee.Get_address(rx_byte);
        ss.print("Address: ");
        ss.println(m_msg_data.address, HEX);
        m_parser_state = PARSE::UNUSED;
      }
      m_parser_cnt++;
      break;

    case PARSE::UNUSED :
      ss.print("UNUSED: ");
      ss.println(m_parser_cnt);
      m_msg_array[m_parser_cnt] = rx_byte;
      if(m_parser_cnt == RX::RESV_1 + 2)  // RESV 1, RESV2, TX_OPT
      {
        m_parser_state = PARSE::PAYLOAD_CNT;
      }
      m_parser_cnt++;
      break;

    case PARSE::PAYLOAD_CNT :
      ss.print("PAYLOAD CNT: ");
      ss.println(m_parser_cnt);
      m_msg_array[m_parser_cnt] = rx_byte;
      m_msg_data.payload_cnt = rx_byte;
      m_parser_state = PARSE::PAYLOAD_TYPE;
      m_parser_cnt++;
      break;

    case PARSE::PAYLOAD_TYPE :
      ss.print("PAYLOAD TYPE: ");
      ss.println(m_parser_cnt);
      m_msg_array[m_parser_cnt] = rx_byte;
      m_msg_data.payload_id = rx_byte;
      m_parser_state = PARSE::PAYLOAD;
      m_parser_cnt++;
      break;

    case PARSE::PAYLOAD :
      ss.print("PAYLOAD: ");
      ss.println(m_parser_cnt);
      static uint8_t j = 0;
      m_msg_array[m_parser_cnt] = rx_byte;
      m_msg_data.payload[j] = rx_byte;
      j++;
      if(m_parser_cnt == RX::PAYLOAD_2)
      {
        m_parser_state = PARSE::CHECKSUM;
        j = 0;
      }
      m_parser_cnt++;
      break;

    case PARSE::CHECKSUM :
    {
      ss.print("CHECKSUM: ");
      ss.println(m_parser_cnt);
      m_msg_array[m_parser_cnt] = rx_byte;
      ss.print("CHECKSUM array: ");
      print_array(m_msg_array, sizeof(m_msg_array), true);

      uint8_t cs = m_xbee.Get_checksum(m_msg_array, sizeof(m_msg_array));
      if(cs == m_msg_array[sizeof(m_msg_array) - 1])
      {
        m_msg_data.valid = true;
        print_msg(m_msg_data, sizeof(m_msg_data.payload));
      }
      else
      {
        ss.println("Invalid checksum");
      }
      ss.print("Parser count: ");
      ss.println(m_parser_cnt);
      reset_parser();
      break;
    }
  }
}

//////////////////////////////////////////////////////////////////////

void reset_parser()
{
  m_xbee.Clear_array(m_msg_array, sizeof(m_msg_array));
  m_parser_cnt = 0;
  m_parser_state = PARSE::SOM;
  clear_msg(m_msg_data);
}

//////////////////////////////////////////////////////////////////////

void clear_msg(struct Msg_data& msg)
{
  msg.valid = false;
  msg.frame_type = 0;
  msg.address = 0;
  msg.length = 0;
  msg.payload_cnt = 0;
  msg.payload_id = 0;
  msg.payload[3] = {};
};

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
  ss.print("Length: ");
  ss.println(msg.length, HEX);
  ss.print("Frame type: ");
  ss.println(msg.frame_type, HEX);
  ss.print("Address: ");
  ss.println(msg.address, HEX);
  ss.print("Count: ");
  ss.println(msg.payload_cnt, HEX);
  ss.print("Payload id: ");
  ss.println(msg.payload_id, HEX);
  ss.print("Payload: ");
  print_array(msg.payload, len_payload, false);
  ss.println();
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
