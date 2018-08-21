#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <SoftwareSerial.h>

//PM Sensors:
uint16_t pm10 = 0;
uint16_t pm25 = 0;
uint16_t pm100 = 0;
uint16_t tpm10 = 0;
uint16_t tpm25 = 0;
uint16_t tpm100 = 0;

uint16_t tpm10Sum;
uint16_t tpm25Sum;
uint16_t tpm100Sum;
uint16_t pm10Sum;
uint16_t pm25Sum;
uint16_t pm100Sum;
int PMerrors = 0;
uint8_t buf[24];
boolean newData = false;


uint8_t count = 0;
int countreadStream = 0;
int countincorrectStream =0;
int countcorrectStream = 0;

SoftwareSerial mySerial(0, 1);


void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  //Serial.println("hello world");
  // put your setup code here, to run once:

}

void loop() {

  
    
    //getPMValues();
    //Serial.println(pm25);
  getStream();
  //Serial.println(pm25)
  
  /*for(int i =0 ; i < 24 ; i ++){
     Serial.println(buf[i]); 
  }*/
  Serial.println(tpm25);

  //Serial.println(pm10);
  //Serial.println(pm100);
  //Serial.println(pm25);
  //Serial.println("here");

}


void getStream(){
 int idx = 0;
 int dataSum = 0;
 memset(buf, 0, 24);
 elapsedMillis waiting;
 while(waiting < 1000){
  if(mySerial.available()){
    buf[idx++] = mySerial.read();
      if(idx > 23){
        break;
      }
      if(buf[0] != 66 && buf[1] != 77){
        break;  
      }
    waiting = 0;
    }
  }
  for( int i = 0 ; i <24;i++){
     dataSum = dataSum + buf[i]; 
  }
 
  if(dataSum != ((buf[22]<<8) + buf[23] + buf[22] + buf[23])){
     Serial.println("DataSum doesn't match !");
  }
  else{
    pm25 = ( buf[12] << 8 ) | buf[13];
    pm10 = ( buf[10] << 8 ) | buf[11];
    pm100 = ( buf[14] << 8 ) | buf[15];
    tpm10 = ( buf[4] << 8 ) | buf[5];
    tpm25 = ( buf[6] << 8 ) | buf[7];
    tpm100 = ( buf[8] << 8 ) | buf[9];

    tpm10Sum += tpm10;  tpm25Sum += tpm25;  tpm100Sum += tpm100;
    pm10Sum += pm10; pm25Sum += pm25; pm100Sum += pm100;

  }
}

