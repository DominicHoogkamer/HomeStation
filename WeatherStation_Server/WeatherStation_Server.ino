#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include "MQ135.h"
#include <Wire.h>
#include <SPI.h>
#include "SH1106.h"
#include "SH1106Ui.h"

#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
HTTPClient http;

// Global Variables
// =====================================================================

const char *ssid = "Test";
const char *password = "test1234";

String httpPostUrl = "http://iot-open-server.herokuapp.com/data";
String apiToken = "e6b884fdef63344db8bd5f4a";

String hum = "";
String temp = "";
String airP = "";

// Wifi Config
// =====================================================================

WiFiServer server(80);

IPAddress ip(192, 168, 43, 215);            // IP address of the server
IPAddress gateway(192, 168, 0, 1);        // gateway of your network
IPAddress subnet(255, 255, 255, 0);       // subnet mask of your network

unsigned long clientTimer = 0;

// SETUP
// =====================================================================

void setup() {
  Serial.begin(115200);   // Start monitor for debugging
  dht.begin();            // Start dht sensor
  connectToWifiNetwork(); // Function to make connection with Wifi
}

// LOOP
// =====================================================================

void loop() {

  MQ135 gasSensor = MQ135(A0);
  float airquality = gasSensor.getPPM();

  // Check if Wifi Client Exist
  // =====================================================================

  WiFiClient client = server.available();
  if (client) {
    if (client.connected()) {
      digitalWrite(LED_BUILTIN, LOW);  // to show the communication only (inverted logic)
      Serial.println(".");            // Dot Dot try connecting
      String request = client.readStringUntil('\r');    // receives the message from the client
      //Serial.print("From client: "); Serial.println(request);

      temp = getValue(request , ';' , 1);       // Set temp to incoming data
      hum = getValue(request , ';' , 2);        // Set hum to incoming data
      airP = getValue(request , ';' , 3);       // Set AirPressure to incoming data

      client.flush();
      //client.println("Hi client! No, I am listening.\r"); // sends the answer to the client
    }
    client.stop();                // tarminates the connection with the client
    clientTimer = millis();
  }

  // Send Data to Server
  // =====================================================================

  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& jsonRoot = jsonBuffer.createObject();

  jsonRoot["token"] = apiToken;

  JsonArray& data = jsonRoot.createNestedArray("data");
  //
  JsonObject& humidityObject = jsonBuffer.createObject();
  humidityObject["key"] = "humidity";
  humidityObject["value"] = hum;
  //
  JsonObject& temperatureObject = jsonBuffer.createObject();
  temperatureObject["key"] = "temperature";
  temperatureObject["value"] = temp;

  JsonObject& airQualityObject = jsonBuffer.createObject();
  airQualityObject["key"] = "airq";
  airQualityObject["value"] = airquality;

  JsonObject& airPressureObject = jsonBuffer.createObject();
  airPressureObject["key"] = "airp";
  airPressureObject["value"] = airP;

  data.add(humidityObject);
  data.add(temperatureObject);
  data.add(airQualityObject);
  data.add(airPressureObject);

  String dataToSend;
  jsonRoot.printTo(dataToSend);
  Serial.println(dataToSend);
  postData(dataToSend);

  if (millis() - clientTimer > 30000) {    // stops and restarts the WiFi server after 30 sec
    WiFi.disconnect();                     // idle time
    delay(5000);
    server_start(1);
  }
  delay(1000); // Testing purpose
}


void server_start(byte restart) {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  server.begin();
  delay(500);
  clientTimer = millis();
}

void connectToWifiNetwork() {
  Serial.println ( "Trying to establish WiFi connection" );
  WiFi.begin ( ssid, password );
  Serial.println ( "" );

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );
  server.begin();
}

void postData(String stringToPost) {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(httpPostUrl);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(stringToPost);
    String payload = http.getString();

    Serial.println(httpCode);
    Serial.println(payload);
    http.end();
  } else {
    Serial.println("Wifi connection failed, retrying.");
    connectToWifiNetwork();
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

