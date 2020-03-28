#include <SoftwareSerial.h>
#include "LowPower.h"
#include "setup.h"
#include "SimpleTimer.h"
#include "XBee_lib.h"
#include "millisDelay.h"
#include "version.h"

#define LED_PIN   13
#define WAKE_PIN  3
typedef unsigned char uchar;



XBee_lib m_xbee;
millisDelay m_send_timer;

SoftwareSerial softSerial(7,8);  //(rx,tx)

bool m_sleep_now = true;
bool m_tx_now = true;
uchar m_tx_count = 0;
uchar rx_array[21] = {};
uchar tx_array[] = {0x7E, 0x00, 0x13, 0x10, 0x00,
                    ADDR_B1, ADDR_B2, ADDR_B3,
                    ADDR_B4, ADDR_B5, ADDR_B6,
                    ADDR_B7, ADDR_B8,
                    0xFF, 0xFE, 0x00, 0x00, 0x11,
                    0x22, 0x23, 0x24, 0x25, 0xD6};
                      
////////////////////////////////////////////////////////////

void wakeUp() 
{  
  // called after interrupt (no delays or millis)
  // reset variables after wakeup
  m_tx_now = true;
  m_tx_count = 0;
  m_sleep_now = false;
}  

////////////////////////////////////////////////////////////
 
void setup() 
{ 
  delay(3000);
  
  Serial.begin(9600);   
  Serial.println("**** SERIAL ****: xbee_api_sleep_txrx_usb_remote");
  Serial.println(version);
  softSerial.begin(9600);    
  softSerial.print("xbee_api_sleep_txrx_usb_remote : ");
  softSerial.println(version);
  
  pinMode(WAKE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // delay before transmission is sent again (no response)
  m_send_timer.start(1000);
}  

////////////////////////////////////////////////////////////
  
void loop() 
{ 
  digitalWrite(LED_PIN, LOW);
  if(m_sleep_now)
  {
    // attach external interrupt and then sleep
    attachInterrupt(digitalPinToInterrupt(WAKE_PIN), wakeUp, RISING);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
  digitalWrite(LED_PIN, HIGH); 
  
  // Disable external pin interrupt on wake up pin.
  detachInterrupt(digitalPinToInterrupt(WAKE_PIN));  
  delay(100);     //delay after wakeup
  mainFunction(); 
} 

////////////////////////////////////////////////////////////

void mainFunction()
{ 
  
  // get checksum for transmission
  tx_array[22] = get_checksum(tx_array, sizeof(tx_array));
  
  // transmit data, timer has timed out
  if(m_tx_now)// && m_send_timer.justFinished())
  {
    transmit_data(tx_array, sizeof(tx_array));

    // reset timer
    //m_send_timer.repeat();
  }


  // get response from coordinator
  while(Serial.available())
  {
    for(int i = 0; i < 21; i++)
    {
      rx_array[i] = Serial.read();    
    }
  }
  delay(25);

  // print response array if new data
  if(rx_array[0] == 0x7E)
  {
    // reset timer so tx isn't immediate after received message
    //m_send_timer.repeat();
    
    print_array(rx_array, sizeof(rx_array));
  }

  // validate response
  uchar rx_check = get_checksum(rx_array, sizeof(rx_array));
  if((rx_array[0] == 0x7E) && (rx_array[20] == rx_check))
  {
    // received response, sleep 
    // do_stuff_with_response_function();  
    m_sleep_now = true; 
  }

  // clear the rx array
  clear_array(rx_array, sizeof(rx_array));  
}

////////////////////////////////////////////////////////////

int8_t get_checksum(uchar array[], uchar len)
{
  long sum = 0;
  for(int i = 3; i < (len - 1); i++)
  {
    sum += array[i]; 
  } 
  int8_t check_sum = 0xFF - (sum & 0xFF);
  
  return check_sum; 
}

////////////////////////////////////////////////////////////

void clear_array(uchar array[], uchar len)
{
  // clear the rx array
  for(int i = 0; i < len; i++)
  {
    array[i] = 0;
  }
}

////////////////////////////////////////////////////////////

void print_array(uchar array[], uchar len)
{
  for(int i = 0; i < len; i++)
  {
    softSerial.print(array[i],HEX);
    softSerial.print(", ");
  } 
  softSerial.println();
}

////////////////////////////////////////////////////////////

void transmit_data(uchar array[], uchar len)
{
  Serial.write(array, len);
  Serial.flush();
  delay(10);
  m_tx_count++;
}
