/*
Based on Environment_Calculations.ino

This code shows how to record data from the BME280 environmental sensor
and perform various calculations.

GNU General Public License

- Added Wifi and ThinkSpeak support

21/02/2018 - Gavin Lyons

Connecting the BME280 Sensor:
Sensor              ->  Board
-----------------------------
Vin (Voltage In)    ->  3.3V
Gnd (Ground)        ->  Gnd
SDA (Serial Data)   ->  D2 ESP8266 / A4 on Uno/Pro-Mini, 20 on Mega2560/Due, 2 Leonardo/Pro-Micro
SCK (Serial Clock)  ->  D1 ESP8266 / A5 on Uno/Pro-Mini, 21 on Mega2560/Due, 3 Leonardo/Pro-Micro

 */

#include <EnvironmentCalculations.h>
#include <BME280I2C.h>
#include <Wire.h>
#include <ESP8266WiFi.h>

#define SERIAL_BAUD 115200


BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

const char* ssid     = "ssid";
const char* password = "yourpassword";


const char* host = "api.thingspeak.com";
const char* thingspeak_key = "yourthingspeakkey";
// sleep for this many seconds
const int sleepSeconds = 600;
//////////////////////////////////////////////////////////////////

void turnOff(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, 1);
}

//////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(SERIAL_BAUD);
  // disable all output to save power
  turnOff(0);
  turnOff(2);
  turnOff(12);
  turnOff(13);
  turnOff(14);
  turnOff(15);
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  

  while(!Serial) {} // Wait

  Wire.begin();

  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  // bme.chipID(); // Deprecated. See chipModel().
  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }
   printBME280Data(&Serial);
}

//////////////////////////////////////////////////////////////////
void loop()
{
    
}

//////////////////////////////////////////////////////////////////
void printBME280Data
(
   Stream* client
)
{
   float temp(NAN), hum(NAN), pres(NAN);

   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_Pa);

   bme.read(pres, temp, hum, tempUnit, presUnit);

   client->print("Temp: ");
   client->print(temp);
   client->print("°"+ String(tempUnit == BME280::TempUnit_Celsius ? "C" :"F"));
   client->print("\t\tHumidity: ");
   client->print(hum);
   client->print("% RH");
   client->print("\t\tPressure: ");
   client->print(pres);
   client->print(" Pa");

   EnvironmentCalculations::AltitudeUnit envAltUnit  =  EnvironmentCalculations::AltitudeUnit_Meters;
   EnvironmentCalculations::TempUnit     envTempUnit =  EnvironmentCalculations::TempUnit_Celsius;

   float altitude = EnvironmentCalculations::Altitude(pres, envAltUnit);
   float dewPoint = EnvironmentCalculations::DewPoint(temp, hum, envTempUnit);
   float seaLevel = EnvironmentCalculations::EquivalentSeaLevelPressure(altitude, temp, pres);
   // seaLevel = EnvironmentCalculations::SealevelAlitude(altitude, temp, pres); // Deprecated. See EquivalentSeaLevelPressure().

   client->print("\t\tAltitude: ");
   client->print(altitude);
   client->print((envAltUnit == EnvironmentCalculations::AltitudeUnit_Meters ? "m" : "ft"));
   client->print("\t\tDew point: ");
   client->print(dewPoint);
   client->print("°"+ String(envTempUnit == EnvironmentCalculations::TempUnit_Celsius ? "C" :"F"));
   client->print("\t\tEquivalent Sea Level Pressure: ");
   client->print(seaLevel);
   client->println(" Pa");

    // Use WiFiClient class to create TCP connections
    WiFiClient wifiClient;
    const int httpPort = 80;
    if (!wifiClient.connect(host, httpPort)) {
      Serial.println("Connection failed to ThinkSpeak!");
      return;
    }  

    String voltage = ""; //String(system_get_free_heap_size());
    
    String url = "/update?key=";
    url += thingspeak_key;
    url += "&field1=";
    url += temp;
    url += "&field2=";
    url += String(hum);
    url += "&field3=";
    url += voltage;
     url += "&field4=";
    url += pres;
    
    Serial.print("Requesting URL: ");
    Serial.println(url);
    
    // This will send the request to the server
    wifiClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");
     delay(10);
     
    // Read all the lines of the reply from server and print them to Serial
    while(wifiClient.available()){
      String line = wifiClient.readStringUntil('\r');
      Serial.print(line);
    }
  
    Serial.println();
    Serial.println("closing connection. going to sleep...");             
  
 // convert to microseconds
  ESP.deepSleep(sleepSeconds * 1000000);
    
}

