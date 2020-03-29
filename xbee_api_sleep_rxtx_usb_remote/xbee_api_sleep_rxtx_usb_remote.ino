#include <SoftwareSerial.h>
#include "LowPower.h"
#include "setup.h"
#include "SimpleTimer.h"
#include "millisDelay.h"
#include "version.h"


#define RX_MSG_SIZE  21 // payload 5
#define TX_MSG_SIZE  23 // payload 5
#define LED_PIN   13
#define WAKE_PIN  3


millisDelay m_send_timer;
millisDelay m_sleep_timer;
millisDelay m_system_timer;

SoftwareSerial softSerial(7,8);  //(rx,tx)

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
  // Disable external pin interrupt on wake up pin.
  // detachInterrupt(digitalPinToInterrupt(WAKE_PIN));
  
  // called after interrupt (no delays or millis)
  // reset variables after wakeup
  m_tx_now = true;
  m_sleep_now = false;
}  

//////////////////////////////////////////////////////////////////////
 
void setup() 
{ 
  delay(3000);
  
  Serial.begin(9600);   
  Serial.println("**** SERIAL ****");
  softSerial.begin(9600);    
  softSerial.print("xbee_api_sleep_txrx_usb_remote : ");
  softSerial.println(version);


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

  // slow loop system loop
  if(m_system_timer.justFinished())
  {
    m_system_timer.repeat();
    handle_wireless();
  }
  
  // no successful response, sleep anyway
  if(m_sleep_timer.justFinished())
  {
    softSerial.println("system timeout, going to sleep");
    m_sleep_now = true;
  }
 
} 

//////////////////////////////////////////////////////////////////////

void handle_wireless()
{ 
  // get response from coordinator
  if(Serial.available())
  {
    for(int i = 0; i < 21; i++)
    {
      rx_array[i] = Serial.read();    
    }
  }


  // insert payloads
  tx_array[17] = m_tx_count;

  
  // transmit data, timer has timed out
  if(m_tx_now &&  m_send_timer.justFinished())
  {
    // get checksum for transmission
    tx_array[22] = get_checksum(tx_array, sizeof(tx_array));
    transmit_data(tx_array, sizeof(tx_array));

    // reset timer
    m_send_timer.repeat();
  }


  // print response array if new data
  if(rx_array[0] == 0x7E)
  {
    print_array(rx_array, sizeof(rx_array));
  }


  // validate response
  uint8_t rx_check = get_checksum(rx_array, sizeof(rx_array));
  if((rx_array[0] == 0x7E) && (rx_array[20] == rx_check))
  {
     softSerial.println("response rx-d, going to sleep");
     
    // received response, sleep 
    // do_stuff_with_response_function();  
    m_sleep_now = true; 
  }


  // clear the rx array
  clear_array(rx_array, sizeof(rx_array));

}

//////////////////////////////////////////////////////////////////////

uint8_t get_checksum(uint8_t array[], uint8_t len)
{
  long sum = 0;
  for(int i = 3; i < (len - 1); i++)
  {
    sum += array[i]; 
  } 
  uint8_t check_sum = 0xFF - (sum & 0xFF);
  
  return check_sum; 
}

//////////////////////////////////////////////////////////////////////

void clear_array(uint8_t array[], uint8_t len)
{
  // clear the rx array
  for(int i = 0; i < len; i++)
  {
    array[i] = 0;
  }
}

//////////////////////////////////////////////////////////////////////

void print_array(uint8_t array[], uint8_t len)
{
  for(int i = 0; i < len; i++)
  {
    softSerial.print(array[i],HEX);
    softSerial.print(", ");
  } 
  softSerial.println();
}

//////////////////////////////////////////////////////////////////////

void transmit_data(uint8_t array[], uint8_t len)
{
  softSerial.println("tx-ing");
  delay(50);
  Serial.write(array, len);
  delay(50);
  m_tx_count++;
}
