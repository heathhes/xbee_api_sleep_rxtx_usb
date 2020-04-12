#include <SoftwareSerial.h>
#include "LowPower.h"
//#include "setup_remote.h"
#include "millisDelay.h"
#include "version.h"
#include "Xbee_lib.h"
#include "Xbee_lib_defs.h"

#define RX_MSG_SIZE  21 // payload 5
#define TX_MSG_SIZE  23 // payload 5
#define LED_PIN   13
#define WAKE_PIN  3

Xbee_lib m_xbee(ID::XBEE_3); // id

millisDelay m_send_timer;
millisDelay m_sleep_timer;
millisDelay m_system_timer;

SoftwareSerial ss(7,8);  //(rx,tx)

bool m_sleep_now = true;
bool m_tx_now = true;
uint8_t m_tx_count = 0;
uint8_t rx_array[21] = {};
uint8_t tx_array[] = {0x7E, 0x00, 0x13, 0x10, 0x00,
                      ADDR_B1, ADDR_B2, ADDR_B3,
                      ADDR_B4, ADDR_B5, ADDR_B6,
                      ADDR_B7, ADDR_B8,
                      0xFF, 0xFE, 0x00, 0x00, 0x11,
                      0x22, 0x23, 0x24, 0x25, 0xD6};
                      
//////////////////////////////////////////////////////////////////////

void wakeUp() 
{  
  // called after interrupt (no delays or millis)
  // reset variables after wakeup
  m_tx_now = true;
  m_sleep_now = false;
}  

//////////////////////////////////////////////////////////////////////
 
void setup() 
{ 
  // allow time to switch to xbee mode on pcb
  delay(3000);

  Serial.begin(19200);
  ss.begin(19200);
  ss.print("xbee_api_sleep_txrx_usb_remote : ");
  ss.println(version);

  // pin definitions
  pinMode(WAKE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // delay before transmission is sent again (no response)
  m_send_timer.start(1000);

  // sleep after 5 seconds regardless if transmission/response status
  m_sleep_timer.start(5000);

  // slow the tx-ing and handling loop (rx serial always running);
  m_system_timer.start(100);
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
      m_sleep_now = true;
    }
    else
    {
      ss.print("RECEIVED INVALID FRAME: ");
      print_array(rx_data.payload, sizeof(rx_data.payload));
    }
  }


  // insert payloads
  tx_array[17] = m_tx_count;

  
  // transmit data, timer has timed out
  if(m_tx_now &&  m_send_timer.justFinished())
  {
    m_tx_count = m_xbee.Transmit_data(tx_array,
                                      sizeof(tx_array),
                                      ID::XBEE_1);

    // reset timer
    m_send_timer.repeat();
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
