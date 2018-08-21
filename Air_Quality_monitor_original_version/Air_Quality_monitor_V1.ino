/*Teensy 3.2 and Plantower PM 2.5 monitor

 *
 *Format Logging in SD Card:
 *yyyy/mm/dd hh:mm:ss timeElapsed(s),tsi_pm1 tsi_pm2.5 tsi_pm10 pm1 pm2.5 pm10 CO2 temp(C) humidity(%)
 *buffer: the name of the file all collected data is logged into
 *boxLabel:  box label used in SD log
 */

const char* buffer = "teensy.txt";
String boxLabel;
String pmCali;

//Libraries
#include <Wire.h>
#include <SPI.h>         //for SD card
#include <RTClib.h>
//#include <RTC_DS3231.h>
#include <SD.h>
#include <SD_t3.h>
#include <SHT1X.h>       //temp, humidity sensor libray

//Define the variables:
RTC_DS3231 RTC;ze
int sampleSize;
int redPin = 8;
unsigned long timeElapsed;
unsigned long firstTime;
unsigned long logTime;
unsigned long prevTimeElapsed;

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

uint16_t TPM01ValueAvg;
uint16_t TPM2_5ValueAvg;
uint16_t TPM10ValueAvg;
uint16_t PM01ValueAvg;
uint16_t PM2_5ValueAvg;
uint16_t PM10ValueAvg;


//Temp & Hum Sensor:
int tempCerrors = 0;
int humErrors = 0;
float tempC = 0;
float tempCsum;
float tempCavg;

float humidity = 0;
float humiditySum;
float humidityAvg;

const int dataPin = 6;
const int clockPin = 5;

//Calibration:
float labTPM25, labPM25;   //lab calibrated output

//SD Logging:
const int chipSelect = 10;
File myFile;
String dataString = "";         // make a string for assembling the data to log:
String firstDataString = "";    // make a string for headers


//***SET UP***//
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  delay(1000);
  pinMode(redPin, OUTPUT);
  SPI.begin();

  //Initialize SD Card
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");     // keep flashing LED
    digitalWrite(redPin, HIGH);
    delay(300);
    digitalWrite(redPin, LOW);
    delay(300);
    return;
  }
  Serial.println("done.");


  //Initialize RTC
  Serial.print("Initializing RTC Chronodot...");
  digitalWrite(redPin, HIGH);

  Wire.begin();
  RTC.begin();

  DateTime now = RTC.now();// catch the time on RTC for now
  DateTime PCTime = DateTime(__DATE__, __TIME__); // Catch the time on PC for now

  // If any discrepencies , update with the time on PC
  // Manually change this code when the timezone is different uncomment the rtc.adjust(DateTime(__DATE__, __TIME__));
  // Upload it again to Arduino and check if the time is correct
  // Comment out rtc.adjust(DateTime(__DATE__, __TIME__)); lastly, Upload the entire code again
  if (now.unixtime() < PCTime.unixtime()) {
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }



  //Calibration:
  String sdCali = sdRead2("CALI.txt");
  //Serial.print("CALI ");
  //Serial.print(sdCali);       //uncomment to check all text in CALI.txt
  if (sdCali.length() != 0) {
    Calibration(sdCali);
  }
  else{
    zeroNO = 271;
    aeNO = 270;
    sensitivityNO = 0.352;
    zeroCO = 280;
    aeCO = 279;
    sensitivityCO = 0.243;
    zeroNO2 = 298;
    aeNO2 = 294;
    sensitivityNO2 = 0.456;
    zeroO3 = 412;
    aeO3 = 407;
    auxNO2O3 = 0.333;
    sensO3 = 0.222;
  }

  // SD logging string
  firstDataString = TimeString();
  String firstStr = boxLabel + "...New Logging Session..." + firstDataString;
  sdLog(buffer, firstStr);
  String infoStr = "yyyy/mm/dd, hh:mm:ss, timeElapsed, TPM1, TPM2.5, TPM10, PM1, PM2.5, PM10, CO2, TempC, Humidity, NO, NO2, O3, CO2raw, workNO2, auxNO2, workO3, auxO3, labNO, labNO2, labO3, labCO2, labTPM25, labPM25";
  sdLog(buffer, infoStr);

  //Timing
  firstTime = millis();
  prevTimeElapsed = 0;

  //mark the end of setup, blink several times
  digitalWrite(redPin, LOW);      //blink LED
  delay(1000);
  digitalWrite(redPin, HIGH);
}

void loop() {
  bool getFlag = getPMValues();    //Store PM values into global variables
  if (getFlag) {                   //getFlag=1 if checksum proves PM values correct, else getFlag=0
    sampleSize++;
    TimePrint();                   //Print time to serial monitor
    PMPrint();                     //Print PM values to serial monitor
    getSHT1X();                    //Collect temperature and humidity and print on serial monitor
    TimeElapseCalculation();       //Collect and print time elapsed on serial monitor
    Serial.print("\n");

    if (timeElapsed > 60000) {       //Take Avg Every Minute
      AverageCalculation();
      LabCalibration();
      dataString = TimeString() + String(millis() / 1000) + ", ";

      dataString += String(TPM01ValueAvg) + ", " + String(TPM2_5ValueAvg) + ", " + String(TPM10ValueAvg);
      dataString += ", " + String(PM01ValueAvg) + ", " + String(PM2_5ValueAvg) + ", " + String(PM10ValueAvg);
      dataString += ", " + String(CO2concAvg, 2) + ", " + String(tempCavg, 2) + ", " + String(humidityAvg, 2);
      dataString += ", " + String(NOavg) + ", " + String(NO2avg) + ", " + String(O3avg) + ", " + String(CO2rawAvg);
      dataString += ", " + String(workavgNO2) + ", " + String(auxavgNO2) + ", " + String(workavgO3) + ", " + String(auxavgO3);
      dataString += ", " + String(labNO) + ", " + String(labNO2) + ", " + String(labO3) + ", " + String(labCO2) + ", " + String(labTPM25) + ", " + String(labPM25);

      Serial.println("--------MINUTE AVERAGE below------------");
      Serial.println(dataString);
      Serial.println("----------------------------------------");

      //check if log is successful:
      int SDmarker = sdLog(buffer, dataString);
      while (!SDmarker) {
        Serial.println("Card failed, or not present");  // keep flashing LED
        digitalWrite(redPin, HIGH);
        delay(300);
        digitalWrite(redPin, LOW);
        delay(300);
      }
      //Reset firstTime to the last logTime value
      firstTime = logTime;
      ReSet(true);
    }
  }
  delay(100);
}

//***To Calculate Time Elapsed***//

void TimeElapseCalculation() {
  logTime = millis();
  timeElapsed = logTime - firstTime;
  Serial.print("Time elapsed from last datalog: ");
  Serial.println(timeElapsed);

  long intervalTime = timeElapsed - prevTimeElapsed;
  Serial.print("Time from last measurement: ");
  Serial.println(intervalTime);
  prevTimeElapsed = timeElapsed;
}

//***To Print Date and Time***//

void TimePrint() {
  DateTime now = RTC.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  String sec = String(now.second());
  if (sec.length() < 2) {
    sec = "0"+sec;
  }
  Serial.print(sec);
  Serial.print(" ");
}


//***To Get Value from Temperature and Humidity Sensor***//
void getSHT1X() {
  SHT1x sht1x(dataPin, clockPin);
  tempC = sht1x.readTemperatureC();
  if (tempC < 0 || tempC > 50) {
    tempCerrors++;
  }
  else {
    tempCsum += tempC;
  }
  humidity = sht1x.readHumidity();
  if (humidity < 0 || humidity > 100) {
    humErrors++;
  }
  else {
    humiditySum += humidity;
  }
  Serial.print(" Temp = ");
  Serial.print(tempC);
  Serial.print(" ");
  Serial.print("C");
  Serial.print(" Humidity = ");
  Serial.print(humidity);
  Serial.print("%");
  Serial.print("\n");
}


//***To Get Value from PM Sensor***//
bool getPMValues() {
  int idx;
  bool hasPm25Value = false;
  int timeout = 200;
  String printout = "";
  bool flag = false;
  while (!hasPm25Value) {
    idx = 0;
    memset(buf, 0, 24);
    while (Serial1.available()) {
      buf[idx++] = Serial1.read();
    }

    if (buf[0] == 0x42 && buf[1] == 0x4d) {
      pm25 = ( buf[12] << 8 ) | buf[13]; 
      // Shift left 8 bits means multiply by 256 and add buf [13].
      // The data stream has 4 bits to indicate the PM value. First two bits are buf[12]. The last two bits are buf[13]
      pm10 = ( buf[10] << 8 ) | buf[11];
      pm100 = ( buf[14] << 8 ) | buf[15];
      tpm10 = ( buf[4] << 8 ) | buf[5];
      tpm25 = ( buf[6] << 8 ) | buf[7];
      tpm100 = ( buf[8] << 8 ) | buf[9];

      if (checkValue(buf, 24)) {
        flag = true;
        tpm10Sum += tpm10;  tpm25Sum += tpm25;  tpm100Sum += tpm100;
        pm10Sum += pm10; pm25Sum += pm25; pm100Sum += pm100;
        //Serial.println(PMerrors);
      } else {
        PMerrors++;
      }
      hasPm25Value = true;

      // //Debbugging Tool: Print out 24 bytes PM data (See communication protocol for details)
      //      for (int j = 0; j < 24; j++) {
      //        printout = printout + " " + buf[j];
      //      }
      //      Serial.print(printout);
      //      Serial.print("\n");
      //      Serial.print("\n");
    }
    timeout--;
    if (timeout < 0) {
      break;
    }
  }
  return flag;
}

void PMPrint() {
  Serial.print("pm1.0= ");
  Serial.print(pm10);
  Serial.print(" ug/m3 ");
  Serial.print("pm2.5= ");
  Serial.print(pm25);
  Serial.print(" ug/m3 ");
  Serial.print("pm100= ");
  Serial.print(pm100);
  Serial.print(" ug/m3 ");
  Serial.print("tpm1.0= ");
  Serial.print(tpm10);
  Serial.print(" ug/m3 ");
  Serial.print("tpm2.5= ");
  Serial.print(tpm25);
  Serial.print(" ug/m3 ");
  Serial.print("tpm100= ");
  Serial.print(tpm100);
  Serial.print(" ug/m3 \n");
}

// ***Checksum for PM values ***//
int checkValue(uint8_t thebuf[24], int leng)
{
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

// ***Log values into SD Card ***//
int sdLog(const char* fileName, String stringToWrite) {
  int marker;
  File myFile = SD.open(fileName, FILE_WRITE);
  for (int x = 0; x < 5; x++) {
    digitalWrite(redPin, LOW);
    delay(50);
    digitalWrite(redPin, HIGH);
    delay(50);
  }

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print(" to ");
    Serial.print(fileName);
    Serial.print("...");
    myFile.println(stringToWrite);
    myFile.close();
    Serial.println("done.");
    digitalWrite(redPin, HIGH);
    marker = 1;
  } else {
    Serial.print("error opening ");
    Serial.println(fileName);
    while (true) {
      digitalWrite(redPin, HIGH);
      delay(300);
      digitalWrite(redPin, LOW);
      delay(300);
    }
    marker = 0;
  }
  return marker;
}

//**Module to form datastring which will be logged into SD card**//
String TimeString() {
  DateTime now = RTC.now();
  dataString = String(now.year(), DEC);
  dataString += "/";
  dataString += String(now.month(), DEC);
  dataString += "/";
  dataString += String(now.day(), DEC);
  dataString += ", ";
  dataString += String(now.hour(), DEC);
  dataString += ":";
  dataString += String(now.minute(), DEC);
  dataString += ":";
  String tempSec = String(now.second(),DEC);
  if (tempSec.length() == 1) {
  	dataString += "0";
  }
  dataString += tempSec;
  dataString += ", ";

  return dataString;
}

// *** To Reset Sum & Average After Loggin ***//
void ReSet(boolean PMReSet) {
  CO2concSum = 0;
  tempCsum = 0;
  humiditySum = 0;
  NOsum = 0;
  NO2sum = 0;
  O3sum = 0;
  CO2rawSum = 0;


  tempCavg = 0;
  humidityAvg = 0;
  sampleSize = 0;
  humErrors = 0;
  tempCerrors = 0;

  tpm10Sum = 0;
  tpm25Sum = 0;
  tpm100Sum = 0;
  pm10Sum = 0;
  pm25Sum = 0;
  pm100Sum = 0;

  TPM01ValueAvg = 0;
  TPM2_5ValueAvg = 0;
  TPM10ValueAvg = 0;
  PM01ValueAvg = 0;
  PM2_5ValueAvg = 0;
  PM10ValueAvg = 0;

  PMerrors = 0;
}

// *** Calculate Average Every Minute ***//
void AverageCalculation() {
  Serial.print("A minute has elapsed...");

  TPM01ValueAvg = tpm10Sum / sampleSize;
  TPM2_5ValueAvg = tpm25Sum / sampleSize;
  TPM10ValueAvg = tpm100Sum / sampleSize;
  PM01ValueAvg = pm10Sum / sampleSize;
  PM2_5ValueAvg = pm25Sum / sampleSize;
  PM10ValueAvg = pm100Sum / sampleSize;

  CO2concAvg = CO2concSum / sampleSize;
  CO2rawAvg = CO2rawSum / sampleSize;
  tempCavg = tempCsum / sampleSize;
  humidityAvg = humiditySum / sampleSize;

  NOavg = NOsum / sampleSize;
  NO2avg = NO2sum / sampleSize;
  O3avg = O3sum / sampleSize;

  CO2rawAvg = CO2rawSum / sampleSize;
  workavgNO2 = worksumNO2 / sampleSize;
  auxavgNO2 = auxsumNO2 / sampleSize;
  workavgO3 = worksumO3 / sampleSize;
  auxavgO3 = auxsumO3 / sampleSize;

  Serial.println("Calculated avgs...");
}

// *** Used To Read Txt file in SD Card and Output Content as String***//

String sdRead(const char* fileName) {
  File myFile = SD.open(fileName, FILE_READ);
  String data, data2, sdOut;

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print(" from ");
    Serial.print(fileName);
    Serial.println("...");

    while (myFile.available()) {
      data = myFile.read();
      data2 += data;
    }
    myFile.close();
    digitalWrite(redPin, HIGH);
  } else {
    Serial.print("error opening ");
    Serial.println(fileName);
    while (true) {
      digitalWrite(redPin, HIGH);
      delay(300);
      digitalWrite(redPin, LOW);
      delay(300);
    }
  }
  return data2;
}

// *** Function to restart SD card, not used in the main loop***//

void restartSD() {
  //Initialize SD Card
  //  SPI.begin();
  //  Serial.print("Restarting SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");  // keep flashing LED
    while (true) {
      digitalWrite(redPin, HIGH);
      delay(300);
      digitalWrite(redPin, LOW);
      delay(300);
    }
    return;
  }
  Serial.println("done.");
  // SDoff = 0;
}

// *** Read Factory Calibrations from CALI.txt and calibrate alphasensor***//

void Calibration(String raw) {
  //Select boxLabel:
  int j = 0;
  while (raw.substring(j, j + 1) != "\n") {
    j++;
  }
  boxLabel = raw.substring(0, j);
  alphaCali = boxLabel.substring(6, 8);
  pmCali = boxLabel.substring(9, 11);
  CO2Cali = boxLabel.substring(12, 14);
  String rest = raw.substring(j + 1);

  // //Uncomment to debug boxlabel tags:
  // Serial.println("boxLabel "+boxLabel);
  //Serial.println("alphaCali "+alphaCali);
  //Serial.println("pmCali "+pmCali);
  //Serial.println("CO2Cali "+CO2Cali);

  //Obtain each line & select correct line:

  String curr;
  String line;
  bool found = false;
  unsigned int i = 0;
  while (!found) {
    curr = rest.substring(i, i + 1);
    if (curr == "B") {
      if (rest.substring(i + 3, i + 5) == alphaCali) {
        line = rest.substring(i, i + 69);
        found = true;
      }
    }
    i++;
    if (i > rest.length()) {
      Serial.println("Calibration not found");
      break;
    }
  }

  String myData[14];
  int index = 0;
  int start = 0;;
  for (int i = 0; i < 69; i++) {
    curr = line.substring(i, i + 1);
    if (curr == " ") {
      myData[index] = line.substring(start, i);
      index++;
      start = i + 1;
    }
  }
  myData[index] = line.substring(start);

  //  for (int i = 1; i < 14; i++) {
  //    Serial.print(myData[i].toFloat(), 3);
  //    Serial.print("\n");
  //  }

  zeroNO = myData[1].toFloat();
  aeNO = myData[2].toFloat();
  sensitivityNO = myData[3].toFloat();
  zeroCO = myData[4].toFloat();
  aeCO = myData[5].toFloat();
  sensitivityCO = myData[6].toFloat();
  zeroNO2 = myData[7].toFloat();
  aeNO2 = myData[8].toFloat();
  sensitivityNO2 = myData[9].toFloat();
  zeroO3 = myData[10].toFloat();
  aeO3 = myData[11].toFloat();
  auxNO2O3 = myData[12].toFloat();
  sensO3 = myData[13].toFloat();

}

// *** Used To Read Calibrations from LAB.txt and Generate Lab Calibrated Data *** //

void LabCalibration() {
  String labCali = sdRead2("LABX.txt");
  String myData[18];
  int start = 0;
  int index = 0;
  //Serial.println(labCali);

  if (labCali.length() == 0) {
    labNO = NOavg;
    labNO2 = NO2avg;
    labO3 = O3avg;
    labCO2 = CO2concAvg;
    labTPM25 = TPM2_5ValueAvg;
    labPM25 = PM2_5ValueAvg;
    return;
  }

  for (unsigned int i = 0; i < labCali.length(); i++) {
    if ((labCali.substring(i, i + 1) == " ") | (labCali.substring(i, i + 1) == "\n")) {
      myData[index] = labCali.substring(start, i);
      start = i + 1;
      index++;
    }
  }
  myData[index] = labCali.substring(start);

  //  for (int i=0;i<18;i++) {
  //    Serial.print(" mydata: "+myData[i]);
  //  }

  labNO = (1 / myData[1].toFloat()) * (NOavg - myData[2].toFloat());
  labNO2 = (1 / myData[4].toFloat()) * (NO2avg - myData[5].toFloat());
  labO3 = (1 / myData[7].toFloat()) * (O3avg - myData[8].toFloat());
  labCO2 = (1 / myData[10].toFloat()) * (CO2concAvg - myData[11].toFloat());
  labTPM25 = (1 / myData[13].toFloat()) * (TPM2_5ValueAvg - myData[14].toFloat());
  labPM25 = (1 / myData[16].toFloat()) * (PM2_5ValueAvg - myData[17].toFloat());

  //Serial.println("NO " + String(labNO) + " NO2 " + String(labNO2) + " O3 " + String(labO3) + " CO2 " + String(labCO2) + " TPM2.5 " + String(labTPM25) + " PM2.5 " + String(labPM25));
}

String sdRead2(const char* fileName) {
  File myFile = SD.open(fileName, FILE_READ);
  String data, data2, sdOut;

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print(" from ");
    Serial.print(fileName);
    Serial.println("...");

    while (myFile.available()) {
      data = myFile.read();
      data2 += data;
    }
    myFile.close();
    digitalWrite(redPin, HIGH);
  } else {
    Serial.print("error opening ");
    Serial.println(fileName);
    data2 = "";
  }
  return data2;
}
