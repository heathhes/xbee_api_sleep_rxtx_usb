
/*This code transmits a data payload through the xbee
  in API mode. It sleeps inbetween transmissions and is 
  wakened by the xbee sleep-off output on an interrupt pin
  
*/

#include "LowPower.h"
#include "setup.h"
#include "SoftwareSerial.h"
#include "version.h"
#define LED_PIN   13
#define MSG_SIZE  21

SoftwareSerial softSerial(7,8);  //(rx,tx)

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
}  

  
//////////////////////////////////////////////////////////////////////

void loop() 
{ 

  bool respond = false;
  while(Serial.available())
  {
    for(int i = 0; i < 21; i++)
    {
      rx_array[i] = Serial.read();    
    }
  }
  delay(50); // absolutely must have 50ms+

  
  if(rx_array[0] == 0x7E)
  {
    for(int i = 0; i < 21; i++)
    {
      softSerial.print(rx_array[i],HEX);
      softSerial.print(", ");
    } 
    softSerial.println();
    respond = true;
  }

  // tx data back to xbee
  if(respond)
  {
    //-----create checksum byte for transmission-----
    int8_t check_sum = getTxCheckSum();
    tx_array[22] = check_sum;
    delay(500);
    Serial.write(tx_array, 23);
    Serial.flush();
    delay(50);
    respond = false;
  }

  
  for(int i = 0; i < 21; i++)
  {
    rx_array[i] = 0;
  }

}

//////////////////////////////////////////////////////////////////////

int8_t getTxCheckSum(){
  long sum = 0;
  for(int i = 3; i < (sizeof(tx_array) - 1); i++){
    sum += tx_array[i]; 
  } 
  int8_t check_sum = 0xFF - (sum & 0xFF);
  return check_sum; 
}
 
