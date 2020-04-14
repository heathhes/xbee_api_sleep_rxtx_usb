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
Xbee_lib m_xbee(&ss);

millisDelay m_send_timer;
millisDelay m_sleep_timer;
millisDelay m_system_timer;

OneWire oneWire(DS180_TEMP);
DallasTemperature sensor(&oneWire);

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
  Serial.println("**** SERIAL ****");

  ss.begin(19200);   //m_xbee.Begin(19200);
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

  // callback for when valid data received
  m_xbee.Set_callback(Message_received);
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
  while(Serial.available())
  {
    m_xbee.Process_byte(Serial.read());
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
}

//////////////////////////////////////////////////////////////////////

uint8_t Message_received(const struct Msg_data data)
{
  ss.println("Message_received");
  m_xbee.Print_msg(data, sizeof(data.payload));

  // received valid response, do stuff and then sleep
  m_sleep_now = true;
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
