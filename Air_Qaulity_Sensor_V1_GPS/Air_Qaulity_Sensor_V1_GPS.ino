#include <SoftwareSerial.h>
//Serial connection for PM / Wifi
#include <SPI.h> 
#include <SD.h> 
#include <stdint.h> 
#include <SD_t3.h>
 // SPI for Micro SD card
#include "RTClib.h"
// For RTC 
#include <SparkFunBME280.h> 
#include <SparkFunCCS811.h> 
#include <TinyGPS++.h>
#define CCS811_ADDR 0x5B 
TinyGPSPlus gps;
//Default I2C Address for CCS811 sensor

const int lcdTx = 10;
const int gpsRx =  9;
const int wifiTx = 8;
const int wifiRx = 7;
const int pmRx = 0;
const int pmTx = 1;
const int airLEDgreen = 20;
const int airLEDyellow = 21;
const int airLEDred = 22;
const char * buffer = "teensy.txt";
File myFile;
String timeString;
String dataString;
String dataStringsd;
String dataStringlcd;
String year, month, day, second, hour, minute;

//PM Sensors variable
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
int pmErrors = 0;
uint8_t buf[24];

float tpm01Valueavg;
float tpm25Valueavg;
float tpm10Valueavg;
float pm01Valueavg;
float pm25Valueavg;
float pm10Valueavg;

//Temperature and Humidity variable

float tempC = 0;
float tempCSum;
float tempCAvg;

float humidity = 0;
float humiditySum;
float humidityAvg;

// CO2 variable 

uint16_t co2Con = 0;
uint16_t co2Consum;
uint16_t co2Conavg;

// VOC variable

uint16_t vocCon = 0;
uint16_t vocConsum;
uint16_t vocConavg;

//pressure and height

float pressure = 0;
float pressureSum = 0;
float pressureAvg = 0;

float height = 0;
float heightAvg = 0;
float heightSum = 0;

// AQI Index 

int AQI = 0;

RTC_DS3231 rtc;
CCS811 myCCS811(CCS811_ADDR);
BME280 myBME280;

SoftwareSerial pmSerial(0, 1); // PM for RX, TX
SoftwareSerial wifiSerial(7, 8); // WIFI for RX , TX
SoftwareSerial LCD = SoftwareSerial(gpsRx, lcdTx);

void setup() {
  Serial.begin(9600); // For Serial monitor 
  pmSerial.begin(9600); // For PM sensor
  wifiSerial.begin(9600); // For wifi
  SPI.begin();
  Wire.begin();

  // Define LED ports
  pinMode(airLEDgreen, OUTPUT);
  pinMode(airLEDyellow, OUTPUT);
  pinMode(airLEDred, OUTPUT);

  // LCD start up
  pinMode(lcdTx, OUTPUT);
  digitalWrite(lcdTx, HIGH);
  LCD.begin(9600);
  delay(1000);
  LCD.write(17);

  // RTC start up 

  if (!rtc.begin()) {
    Serial.println("Can't fine RTC");
    //Clear screen
    LCD.write(12);
    delay(5);
    //Print out statement, display for 2 seconds
    LCD.print("Can't find RTC");
    LCD.write(13);
    delay(2000);
    //Clear screen
    LCD.write(12);
    while (1);
  } else {
    Serial.println("RTC initialized successfully");
    LCD.write(12);
    delay(5);
    LCD.print("RTC initialized successfully");
    LCD.write(13);
    delay(2000);
    LCD.write(12);
  }
  DateTime now = rtc.now(); // Catch the time on RTC for now
  DateTime pcTime = DateTime(__DATE__, __TIME__); // Catch the time on PC for now

  // If any discrepencies , update with the time on PC 
  // Manually change this code when the timezone is different uncomment the rtc.adjust(DateTime(__DATE__, __TIME__));
  // Upload it again to Arduino and check if the time is correct
  // Comment out rtc.adjust(DateTime(__DATE__, __TIME__)); lastly, Upload the entire code again
  if (now.unixtime() < pcTime.unixtime()) {
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  rtc.begin();

  // Set SS / CS pin as 15 , SD.Begin(XX)-> XX is also SS pin number
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  // SD card reader start up (Due to the bad reader, max clock speed can only be 72 MHz)

  if (SD.begin(15) == false) {
    Serial.println("SD card didn't initialized");
    LCD.write(12);
    delay(5);
    LCD.print("SD card didn't initialized");
    LCD.write(13);
    delay(2000);
    LCD.write(12);
    delay(5);
  } else {
    Serial.println("SD card initialized successfully");
    LCD.write(12);
    delay(5);
    LCD.print("SD card initialized sucessfully");
    LCD.write(13);
    delay(2000);
    LCD.write(12);
    delay(5);
  }

  //This begins the CCS811 sensor and prints error status of .begin()
  CCS811Core::status returnCode = myCCS811.begin();
  Serial.print("CCS811 begin exited with: ");
  LCD.write(12);
  delay(5);
  LCD.print("CCS811 begin exited with ");
  printDriverError(returnCode); //Pass the error code to a function to print the results
  Serial.println();

  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;

  //Initialize BME280
  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;
  myBME280.settings.runMode = 3; //Normal mode
  myBME280.settings.tStandby = 0;
  myBME280.settings.filter = 4;
  myBME280.settings.tempOverSample = 5;
  myBME280.settings.pressOverSample = 5;
  myBME280.settings.humidOverSample = 5;

  //Calling .begin() causes the settings to be loaded
  delay(10); //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
  myBME280.begin();
  myBME280.setReferencePressure(101200);


  //WIFI Pin set up 
  //RESET
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  //GPIO 0 next version PCB , pin change to 4
  // Comment out because Pin 5 is TX1 
  //pinMode(5, OUTPUT);
  //digitalWrite(5, LOW);
  //GPIO 2
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  delay(1000);

}

//Timer for SD logging
int sampleSize = 0;
int sampleSizepm = 0;
elapsedMillis sinceStartup;


void loop() {
  
  //analogWrite(airLEDgreen, 0);
  //analogWrite(airLEDyellow, 0);
  //analogWrite(airLEDred, 0);
  
  getTHCV();
  elapsedMillis pmTimer;
  while(pmTimer < 750){
    getPmvalues();
    
     getGPS();
  }
  pmTimer = 0;


  //sampleSize++;

  if (sinceStartup > 60000) {
    
    calculateAvg();
    DateTime now = rtc.now();
    year = String(now.year(), DEC);
    //Convert from Now.year() long to Decimal String object
    month = String(now.month(), DEC);
    day = String(now.day(), DEC);
    hour = String(now.hour(), DEC);
    minute = String(now.minute(), DEC);
    second = String(now.second(), DEC);
    timeString = year + "/" + month + "/" + day + " " + hour + ":" + minute + ":" + second + " ";
    dataString = String(tpm25Valueavg) + " " + String(pm25Valueavg) + " " + String(tempCAvg) + " " + String(humidityAvg) + " " + String(pressureAvg) + " " + String(co2Conavg) + " " + String(vocConavg);
    dataStringlcd = String(int(tpm25Valueavg)) + " " + String(int(pm25Valueavg)) + " " + String(tempCAvg) + " " + String(humidityAvg) + " " + String(pressureAvg) + " " + String(co2Conavg) + " " + String(vocConavg);
    dataStringsd = String(pm01Valueavg) + " " + String(tpm01Valueavg) + " " + String(pm25Valueavg) + " " + String(tpm25Valueavg) + " " + String(pm10Valueavg) + " " + String(tpm10Valueavg) + " " + String(tempCAvg) + " " + String(humidityAvg) + " " + String(pressureAvg) + " " + String(co2Conavg) + " " + String(vocConavg);
    sdLog(buffer, timeString + dataStringsd);
    Serial.println(timeString + dataStringsd);

    /*LCD.write(12);
    delay(5);
    elapsedMillis lcdTimer;
    LCD.print(timeString);  
    while(lcdTimer < 2000){
    }
    lcdTimer = 0;*/
    LCD.write(13);
    LCD.write(12);
    delay(5);
    LCD.print(dataStringlcd);
    reset();

    calculateAQI();
    aqiLED(AQI);

  } else {

  }

}


void getGPS(){


  while(LCD.available() > 0 ){
      byte gpsData = LCD.read();
      Serial.write(gpsData);
      /*gps.encode(LCD.read());
      Serial.print("Latitude= "); 
      Serial.print(gps.location.lat(), 6);
      Serial.print(" Longitude= "); 
      Serial.println(gps.location.lng(), 6);
      Serial.println(" GPS Satelitle No=");
      Serial.println(gps.satellites.value());*/
  }
    
    
}
  
  
  
  
  
  


void getPmvalues() {
  int idx = 0;
  memset(buf, 0, 24);
  while (pmSerial.available() > 24) {
  
    while (pmSerial.available()) {
      buf[idx++] = pmSerial.read();
      if (idx > 23){
        break;
      }

    }
    //blinkYellowred();
    
   // Serial.println(checkValue(buf, 24));
    if(buf[0] != 66 && buf[1] != 77){
    //blinkYellow();
    break;  
    }
    
    
    //Serial.println(checkValue(buf,24));
    if (checkValue(buf, 24)) {

      pm25 = ( buf[12] << 8 ) | buf[13];
      pm10 = ( buf[10] << 8 ) | buf[11];
      pm100 = ( buf[14] << 8 ) | buf[15];
      tpm10 = ( buf[4] << 8 ) | buf[5];
      tpm25 = ( buf[6] << 8 ) | buf[7];
      tpm100 = ( buf[8] << 8 ) | buf[9];

      tpm10Sum += tpm10;  tpm25Sum += tpm25;  tpm100Sum += tpm100;
      pm10Sum += pm10; pm25Sum += pm25; pm100Sum += pm100;
      //Serial.println(pm25Sum);
      sampleSizepm ++;
      //Serial.println(sampleSizepm);
    } else {
     // blinkRed();
      break;

    }

  }

}

void getTHCV() {

  if (myCCS811.dataAvailable()) {
    //Calling this function updates the global tVOC and eCO2 variables

    myCCS811.readAlgorithmResults();
    //printInfoSerial fetches the values of tVOC and eCO2
    co2Con = myCCS811.getCO2();
    vocCon = myCCS811.getTVOC();
    tempC = myBME280.readTempC();
    pressure = myBME280.readFloatPressure();
    height = myBME280.readFloatAltitudeMeters();
    humidity = myBME280.readFloatHumidity();

    //Serial.println(pressure);

    //This sends the temperature data to the CCS811
    myCCS811.setEnvironmentalData(humidity, tempC);

    humiditySum += humidity;
    tempCSum += tempC;
    co2Consum += co2Con;
    vocConsum += vocCon;
    heightSum += height;
    pressureSum += pressure;

    sampleSize ++;

  } else if (myCCS811.checkForStatusError()) {
    //If the CCS811 found an internal error, print it.
    printSensorError();

  }

}

void calculateAvg() {

  tpm01Valueavg = tpm10Sum / sampleSizepm;
  tpm25Valueavg = tpm25Sum / sampleSizepm;
  tpm10Valueavg = tpm100Sum / sampleSizepm;
  pm01Valueavg = pm10Sum / sampleSizepm;
  pm25Valueavg = pm25Sum / sampleSizepm;
  pm10Valueavg = pm100Sum / sampleSizepm;

  co2Conavg = co2Consum / sampleSize;
  tempCAvg = tempCSum / sampleSize;
  humidityAvg = humiditySum / sampleSize;
  vocConavg = vocConsum / sampleSize;
  heightAvg = heightSum / sampleSize;
  pressureAvg = (pressureSum * 0.01) / sampleSize;

}

void reset() {
  co2Consum = 0;
  tempCSum = 0;
  humiditySum = 0;

  co2Conavg = 0;
  tempCAvg = 0;
  humidityAvg = 0;

  sampleSize = 0;
  sampleSizepm = 0;

  tpm10Sum = 0;
  tpm25Sum = 0;
  tpm100Sum = 0;
  pm10Sum = 0;
  pm25Sum = 0;
  pm100Sum = 0;

  vocConavg = 0;
  vocConsum = 0;
  vocCon = 0;

  tpm01Valueavg = 0;
  tpm25Valueavg = 0;
  tpm10Valueavg = 0;
  pm01Valueavg = 0;
  pm25Valueavg = 0;
  pm10Valueavg = 0;

  heightSum = 0;
  heightAvg = 0;
  height = 0;

  pressureSum = 0;
  pressureAvg = 0;
  pressure = 0;

  sinceStartup = 0;


}

void sdLog(const char * fileName, String stringToWrite) {
  File myFile = SD.open(fileName, FILE_WRITE);
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to ");
    Serial.print(fileName);
    Serial.print("...");
    myFile.println(stringToWrite);
    // close the file:
    myFile.close();
    Serial.println("done.");
    digitalWrite(13, HIGH);
    delay(300);
    digitalWrite(13, LOW);
    delay(300);
  } else {
    // if the file didn't open, print an error:
    Serial.print("error opening ");
    Serial.println(fileName);
  }
}

void calculateAQI() {
  float pm25Low = 35.4;
  float pm25Medium = 150.4;
  float pm10Low = 154;
  float pm10Medium = 354;

  if ( pm25Valueavg < pm25Low && pm25Valueavg < pm10Low) {
    AQI = 5;

  } else if (pm25Valueavg > pm25Low && pm25Low < pm25Medium && pm10Valueavg > pm10Low && pm25Valueavg < pm10Medium) {
    AQI = 10;

  } else if (pm25Valueavg > pm25Medium && pm25Valueavg > pm10Medium ) {
    AQI = 15;

  }

}

void aqiLED(int AQI) {

  if (AQI == 5) {
    analogWrite(airLEDgreen, 255);
    analogWrite(airLEDyellow, 0);
    analogWrite(airLEDred, 0);
  } else if (AQI == 10) {
    analogWrite(airLEDyellow, 255);
    analogWrite(airLEDgreen, 0);
    analogWrite(airLEDred, 0);
  } else {
    analogWrite(airLEDred, 255);
    analogWrite(airLEDgreen, 0);
    analogWrite(airLEDyellow, 0);
  }

}

void printDriverError(CCS811Core::status errorCode) {
  switch (errorCode) {
  case CCS811Core::SENSOR_SUCCESS:
    Serial.print("SUCCESS");
    LCD.print("SUCCESS");
    //LCD.write(13);
    delay(2000);
    LCD.write(12);
    delay(5);
    break;
  case CCS811Core::SENSOR_ID_ERROR:
    Serial.print("ID_ERROR");
    LCD.print("ID_ERROR");
    //LCD.write(13);
    delay(2000);
    LCD.write(12);
    delay(5);
    break;
  case CCS811Core::SENSOR_I2C_ERROR:
    Serial.print("I2C_ERROR");
    LCD.print("I2C_ERROR");
    //LCD.write(13);
    delay(2000);
    LCD.write(12);
    delay(5);
    break;
  case CCS811Core::SENSOR_INTERNAL_ERROR:
    Serial.print("INTERNAL_ERROR");
    LCD.print("INTERNAL_ERROR");
    //LCD.write(13);
    delay(2000);
    LCD.write(12);
    delay(5);
    break;
  case CCS811Core::SENSOR_GENERIC_ERROR:
    Serial.print("GENERIC_ERROR");
    LCD.print("GENERIC_ERROR");
    //LCD.write(13);
    delay(2000);
    LCD.write(12);
    delay(5);
    break;
  default:
    Serial.print("Unspecified error.");
    LCD.print("Unspecified error.");
    //LCD.write(13);
    delay(2000);
    LCD.write(12);
    delay(5);
  }
}

void printSensorError() {
  uint8_t error = myCCS811.getErrorRegister();

  if (error == 0xFF) //comm error
  {
    Serial.println("Failed to get ERROR_ID register.");
  } else {
    Serial.print("Error: ");
    if (error & 1 << 5) Serial.print("HeaterSupply");
    if (error & 1 << 4) Serial.print("HeaterFault");
    if (error & 1 << 3) Serial.print("MaxResistance");
    if (error & 1 << 2) Serial.print("MeasModeInvalid");
    if (error & 1 << 1) Serial.print("ReadRegInvalid");
    if (error & 1 << 0) Serial.print("MsgInvalid");
    Serial.println();
  }
}

/*void blinkRed(){
      elapsedMillis blinkredtimer;
      while(blinkredtimer < 500){
      analogWrite(airLEDred, 255);
      }
      blinkredtimer = 0;  
  
}


void blinkYellow(){
      elapsedMillis blinkyellowtimer;
      while(blinkyellowtimer < 500){
      analogWrite(airLEDyellow, 255);
      }
      blinkyellowtimer = 0;  
}


void blinkYellowred(){
      elapsedMillis blinkyellowredtimer;
      while(blinkyellowredtimer < 500){
      analogWrite(airLEDyellow, 255);
      analogWrite(airLEDred, 255);
      }
      blinkyellowredtimer = 0;  
}
/*/


// ***Checksum for PM values ***//
int checkValue(uint8_t thebuf[24], int leng) {
  char receiveflag = 0;
  int receiveSum = 0;
  int i = 0;

  for (i = 0; i < leng; i++) {
    receiveSum = receiveSum + thebuf[i];
  }

  if (receiveSum == ((thebuf[leng - 2] << 8) + thebuf[leng - 1] + thebuf[leng - 2] + thebuf[leng - 1])) //checksum the serial data
  {
    receiveSum = 0;
    receiveflag = 1;
  }
  return receiveflag;
}
