#include "LowPower.h"
#include "setup.h"
#include "SoftwareSerial.h"
#include "millisDelay.h"
#include "version.h"
#include "Xbee_lib.h"

#define RX_MSG_SIZE  21 // payload 5
#define TX_MSG_SIZE  23 // payload 5
#define LED_PIN   13

Xbee_lib m_xbee(ID::XBEE_1); // id

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
  Serial.begin(19200);
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
    m_tx_count = m_xbee.Transmit_data(tx_array, sizeof(tx_array), ID::XBEE_3);
    respond = false;
  }


  // clear the rx array
  m_xbee.Clear_array(rx_array, sizeof(rx_array));

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

