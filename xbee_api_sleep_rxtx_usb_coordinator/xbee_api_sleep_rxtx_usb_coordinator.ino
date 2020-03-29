
/*This code transmits a data payload through the xbee
  in API mode. It sleeps inbetween transmissions and is 
  wakened by the xbee sleep-off output on an interrupt pin
  
*/

#include "LowPower.h"
#include "setup.h"
#include "SoftwareSerial.h"
#include "millisDelay.h"
#include "version.h"

#define RX_MSG_SIZE  21 // payload 5
#define TX_MSG_SIZE  23 // payload 5
#define LED_PIN   13

millisDelay m_system_timer;

SoftwareSerial softSerial(7,8);  //(rx,tx)

uint8_t m_tx_count = 0;
uint8_t rx_array[21] = {};

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
  
  softSerial.begin(9600);
  Serial.begin(9600);
  Serial.println("**** SERIAL ****");
  softSerial.print("softSerial: xbee_api_sleep_txrx_usb_coordinator : ");
  softSerial.println(version);
  
  pinMode(LED_PIN, OUTPUT); 

  // delay for handling wireless interface
  m_system_timer.start(200);
  
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

  if(Serial.available())
  {  
    for(int i = 0; i < 21; i++)
    {
      rx_array[i] = Serial.read();    
    }
  } // delay required?
  
  if(rx_array[0] == 0x7E)
  {
    print_array(rx_array, sizeof(rx_array));
  }

  if(rx_array[0] == 0x7E)
  {
    respond = true;
  }


  // insert payloads
  tx_array[17] = m_tx_count;  

  
  // tx data back to xbee
  if(respond)
  {
    // get checksum
    tx_array[22] = get_checksum(tx_array,sizeof(tx_array));
    transmit_data(tx_array, sizeof(tx_array));
    respond = false;
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
