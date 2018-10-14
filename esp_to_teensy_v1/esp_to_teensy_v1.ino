#include "SoftwareSerial.h"

#define ESP_RESET 17
#define ESP_Flash 7
#define ESP_TX  9
#define ESP_RX 10
#define GPIO2 14
#define ESP_Serial Serial2

void programmode ();

void setup() {
  // put your setup code here, to run once:
   pinMode(ESP_RESET, OUTPUT);
   pinMode(ESP_Flash, OUTPUT); 
   pinMode(ESP_TX, INPUT); 
   pinMode(ESP_RX, OUTPUT); 
   delay(100);
  


   Serial.begin(115200);
   ESP_Serial.begin(115200);
   
   
  

}

void programmode(){
     digitalWrite(ESP_RESET, LOW);
   delay(1000);
   digitalWrite(ESP_Flash, LOW);
   delay(1000);
   digitalWrite(GPIO2, HIGH);
   delay(1000);
   digitalWrite(ESP_RESET, HIGH);
  
  
  
}



void serialEvent(){
  
  
  while(Serial.available()){
    ESP_Serial.write(Serial.read());
  }
  while(ESP_Serial.available()){
    Serial.write(ESP_Serial.read());
  }
  
  
}


void loop() {


}
