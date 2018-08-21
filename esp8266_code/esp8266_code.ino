#include <ESP8266WiFi.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <SoftwareSerial.h>

#include <WiFiUdp.h>
#include "ArduinoOTA.h"    //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA


const byte numChars = 50;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

// variables to hold the parsed data
float pm01;
float pm25;
float pm10;
float voc;
float co2;
float temp;
float pressure;
float humidity;

boolean newData = false;
unsigned long timer;
// Wifi setup ;

const char* host = "api.thingspeak.com";
String apiKey = "L70906IUM4MHMP72";


void setup() {
        // put your setup code here, to run once:
        Serial.begin(9600);
        

        //WiFiManager
        //Local intialization. Once its business is done, there is no need to keep it around
        WiFiManager wifiManager;
        //reset saved settings
        //wifiManager.resetSettings();

        //set custom ip for portal
        //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

        //fetches ssid and pass from eeprom and tries to connect
        //if it does not connect it starts an access point with the specified name
        //here  "AutoConnectAP"
        //and goes into a blocking loop awaiting configuration
        wifiManager.autoConnect("AutoConnectAP");
        //or use this for auto generated name ESP + ChipID
        //wifiManager.autoConnect();


        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");

     ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready OTA8");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


       // pinMode(0, OUTPUT);
       // digitalWrite(0, LOW);
}

void loop() {
        // put your main code here, to run repeatedly:
        ArduinoOTA.handle();
        receiveDatateensy();
  
}



void establishTCP(){

        WiFiClient client;

        const int httpPort = 80;
        if (!client.connect(host, httpPort)) {
                Serial.println("connection failed");
                return;
        }
        // We now create a URI for the request
        String postStr = apiKey;
        postStr +="&field1=";
        postStr += String(temp);
        postStr +="&field2=";
        postStr += String(pressure);
        postStr +="&field3=";
        postStr += String(pm25);
        postStr +="&field4=";
        postStr += String(co2);
        postStr +="&field5=";
        postStr += String(pm10);
        postStr +="&field6=";
        postStr += String(pm01);
        postStr +="&field7=";
        postStr += String(voc);
        postStr +="&field8=";
        postStr += String(humidity);
        postStr += "\r\n\r\n";

        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(postStr.length());
        client.print("\n\n");
        client.print(postStr);

        client.stop();


}


void receiveDatateensy(){
        recvWithStartEndMarkers();
        if (newData == true) {
                strcpy(tempChars, receivedChars);
                // this temporary copy is necessary to protect the original data
                //   because strtok() used in parseData() replaces the commas with \0
                parseData();
                establishTCP();
                sendbacktoTeensy();
                newData = false;
        }
        digitalWrite(0, LOW);
}


void recvWithStartEndMarkers() {
        static boolean recvInProgress = false;
        static byte ndx = 0;
        char startMarker = '(';
        char endMarker = ')';
        char rc;

        while (Serial.available() > 0 && newData == false) {
                rc = Serial.read();

                if (recvInProgress == true) {
                        if (rc != endMarker) {
                                receivedChars[ndx] = rc;
                                ndx++;
                                if (ndx >= numChars) {
                                        ndx = numChars - 1;
                                }
                        }
                        else {
                                receivedChars[ndx] = '\0'; // terminate the string
                                recvInProgress = false;
                                ndx = 0;
                                newData = true;
                        }
                }

                else if (rc == startMarker) {
                        recvInProgress = true;
                }
        }
}


void parseData() {      // split the data into its parts

        char * strtokIndx; // this is used by strtok() as an index

        strtokIndx = strtok(tempChars,"/");
        pm01 = atof(strtokIndx);

        strtokIndx = strtok(NULL, "/"); // this continues where the previous call left off
        pm25 = atof(strtokIndx);   // convert this part to a float

        strtokIndx = strtok(NULL, "/");
        pm10 = atof(strtokIndx);   // convert this part to a float

        strtokIndx = strtok(NULL,"/");
        co2 = atof(strtokIndx); // convert this part to a float

        strtokIndx = strtok(NULL, "/"); // this continues where the previous call left off
        voc = atof(strtokIndx);   // convert this part to a float

        strtokIndx = strtok(NULL, "/");
        temp = atof(strtokIndx);   // convert this part to a float

        strtokIndx = strtok(NULL, "/"); // this continues where the previous call left off
        humidity = atof(strtokIndx);   // convert this part to a float

        strtokIndx = strtok(NULL, ")");
        pressure = atof(strtokIndx);   // convert this part to a float

}

void sendbacktoTeensy(){

  for(int i = 0 ; i <100;i++){
   Serial.print("."); 
   delay(10);
   }
  
  
}
