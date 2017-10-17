#include <SPI.h>
#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "DHT.h"

#define DHTPIN D4     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors

#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11
#define BMP_CS 10

Adafruit_BMP280 bme; // I2C
DHT dht(DHTPIN, DHTTYPE);

// Global Variables
// =====================================================================

char ssid[] = "Test";           // SSID of your home WiFi
char pass[] = "test1234";            // password of your home WiFi

String dataArray = "";

unsigned long askTimer = 0;

// Wifi Config
// =====================================================================

IPAddress server(192, 168, 43, 215);    // the fix IP address of the server
WiFiClient client;

// SETUP
// =====================================================================

void setup() {
  dht.begin();
  Serial.begin(115200);               // only for debug
  WiFi.begin(ssid, pass);             // connects to the WiFi router

  if (!bme.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
}

// LOOP
// =====================================================================

void loop () {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  float p = bme.readPressure();

  String Pres = String (p);
  String Hum = String (h);
  String Temp = String(t);

  dataArray = "{first;" + Temp + ";" + Hum + ";" + Pres + ";" "}";    // Create StringObject for the server

  //Serial.print(dataArray);

  // Connect to server
  // =====================================================================


  client.connect(server, 80);   // Connection to the server
  Serial.println(".");
  client.println(dataArray);  // sends the message to the server
  String answer = client.readStringUntil('\r');   // receives the answer from the sever
  //Serial.println("from server: " + answer);
  client.flush();
  delay(2000);                  // client will trigger the communication after two seconds
}
