#include <SoftwareSerial.h>
#include "LowPower.h"
#include "setup.h"

#define LED_PIN   13
#define WAKE_PIN  3


//-----address of destination-----
                            //for broadcast
int8_t ADDR_B1 = 0x00;      //0x00
int8_t ADDR_B2 = 0x13;      //0x00
int8_t ADDR_B3 = 0xA2;      //0x00
int8_t ADDR_B4 = 0x00;      //0x00
int8_t ADDR_B5 = 0x41;      //0x00
int8_t ADDR_B6 = 0x25;      //0x00
int8_t ADDR_B7 = 0xA4;      //0xFF
int8_t ADDR_B8 = 0x79;      //0xFF


SoftwareSerial softSerial(7,8);  //(rx,tx)

bool sleep_now = true;
bool tx_data = true;
uint8_t rx_array[21] = {};
uint8_t tx_array[] = {0x7E,0x00,0x13,0x10,0x00,
                   ADDR_B1,ADDR_B2,ADDR_B3,
                   ADDR_B4,ADDR_B5,ADDR_B6,
                   ADDR_B7,ADDR_B8,
                      0xFF,0xFE,0x00,0x00,0x11,
                      0x22,0x23,0x24,0x25,0xB6};

void wakeUp() 
{  
  //called after interrupt // no delays or millis
}  
  
void setup() 
{ 
  delay(3000);
  Serial.begin(9600);   
  Serial.println("Serial: xbee_api_sleep_txrx_usb_remote"); 
  softSerial.begin(9600);    
  softSerial.println("softSerial: xbee_api_sleep_txrx_usb_remote");
  pinMode(WAKE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
}  
  
void loop() 
{ 
  digitalWrite(LED_PIN, LOW);
  if(sleep_now)
  {
    // attach external interrupt and then sleep
    attachInterrupt(digitalPinToInterrupt(WAKE_PIN), wakeUp, RISING);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
  digitalWrite(LED_PIN, HIGH); 
  tx_data = true; 
  
  // Disable external pin interrupt on wake up pin.
  detachInterrupt(digitalPinToInterrupt(WAKE_PIN));  
  delay(100);     //delay after wakeup
  mainFunction(); 
} 

void mainFunction()
{ 

  sleep_now = false;
  //-----create checksum byte for transmission-----
  int8_t check_sum = getTxCheckSum();
  tx_array[22] = check_sum;
  
  //-----transmit meta-data and payload-----
  if(tx_data)
  {
    Serial.write(tx_array, 23);
    Serial.flush();
    //delay(50);
    tx_data = false; 
  }


  // get response from coordinator
  while(Serial.available())
  {
    for(int i = 0; i < 21; i++)
    {
      rx_array[i] = Serial.read();    
    }
  }  
  delay(100);

  if(rx_array[0] == 0x7E)
  {
    for(int i = 0; i < 21; i++)
    {
      softSerial.print(rx_array[i],HEX);
      softSerial.print(", ");
    } 
    softSerial.println(); 
  }

//  if(rx_array[20] == 0x66)
//  {
//    sleep_now = true;
//  }
  
  for(int i = 0; i < 21; i++)
  {
    rx_array[i] = 0;
  }   
}

////////////////////////////////////////
//Get the check sum of recieved message
int8_t getTxCheckSum(){
  long sum = 0;
  for(int i = 3; i < (sizeof(tx_array) - 1); i++){
    sum += tx_array[i]; 
  } 
  int8_t check_sum = 0xFF - (sum & 0xFF);
  return check_sum; 
}
