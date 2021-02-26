//
// This is the firmware for the AirQuality sensor project.  Original
// code written by Aidan McHugh. 
//
// This code runs on an Adafruit Feather Huzzah with a small OLED display.
//   https://www.adafruit.com/product/2821 
//   https://www.adafruit.com/product/2900
//
// To setup the feather Huzzah see:
//   https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide
//
// The feather is connected to a PMS5003.
//   https://www.adafruit.com/product/3686
//
// Dependent libraries.  Can be found in dependency directory as zips.
//   PMS air quality client library - https://github.com/riverscn/pmsx003
//   WifiManager - https://github.com/tzapu/WiFiManager
//   AdaFruit GFX Library - https://github.com/adafruit/Adafruit-GFX-Library
//   AdaFruit SSD1306 Library - https://github.com/adafruit/Adafruit_SSD1306
//   PubSubClient for MQTT - https://github.com/knolleary/pubsubclient
// 
// Board: Adafruit Feather Huzzah 
// CPU Frequency: 80mhz
// Flash size: 4MB
// Upload speed: 115200

#include <Arduino.h>

// Include the various web pages served up by the web service
#include "MainWebPage.h"
#include "DatePage.h"
#include "RealTimePage.h"

// Library for air quality sensor
#include <pms.h>

// Libraries for featherwing display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Third party WifiManager
#include <WiFiManager.h>

// Libraries for ESP chip
#include <ESP8266WiFi.h>

// Library for MQTT
#include <PubSubClient.h>

// Over the air updates
#include <ArduinoOTA.h>

#define ESP_RX 13
#define ESP_TX 13

#define BUTTON_A  0
#define BUTTON_B 16
#define BUTTON_C  2

// Warning - these values are also hardcoded inside of the HTML pages 
// in the .h files within this directory.  If important then they can
// be templatized.
#define MQTT_SERVER "192.168.50.41"
#define MQTT_PORT 1883

#define DO_WEBSERVER
#define USE_MDNS

#ifdef DO_WEBSERVER
#include <ESP8266WebServer.h>
#endif
  
#if defined(USE_MDNS)
#include <ESP8266mDNS.h>
#endif

WiFiClient WIFI_CLIENT;

#ifdef DO_WEBSERVER
ESP8266WebServer server(80);
#endif

// Number of milliseconds to allow the air quality sensor to warm up
// before starting sampling.
#define WARMUP_MILLIS 10000

// Approximate number of milliseconds to wait before fetching more
// air quality readings.
#define NEXT_READING_MILLIS    500
#define NEXT_PUBLISHING_MILLIS 10000

PubSubClient mqtt_client;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
Pmsx003 pms(ESP_RX, ESP_TX);

#define AP_STATION_SSID "AirQuality"
#define AP_STATION_PASSWORD "greatpass"

const char *shortNames[Pmsx003::nValues_PmsDataNames]{
  // The sensor reports particulate counts for particles below a specific
  // size.  PM1.0 is the number of particulates seen during the sample
  // period whose size is 1 micron.  
  // The first set of three readings are for CF=1 
  // Which is 'indoor' readings.  
  "PM1.0",
	"PM2.5",
	"PM10",

  // The second set of three readings are for CF=ATM or 'outdoor' readings.
  // The air quality sensor outputs two sets of particular measurements with
  // different "correction factors".  In the manual it is stated that CF=ATM is
  // applicable under atmospheric (outdoor) environments.
	"PM1.0, CF=ATM",
	"PM2.5, CF=ATM",
	"PM10, CF=ATM",
  
  "P < 0.3 micron",
  "P < 0.5 micron",
  "P < 1.0 micron",
  "P < 2.5 micron",
  "P < 5.0 micron",
  "P < 10. micron",
  
  "Reserved",
};

void onFailedConnect(WiFiManager*);

void handleRoot() {
  server.send(200, "text/html", ROOTPAGEHTML);
}

void handleDateView() {
  server.send(200, "text/html", DATEPAGEHTML);
}

void handleRealTimeView() {
  server.send(200, "text/html", REALTIMEPAGEHTML);
}

void setup(void) {
	Serial.begin(115200);
  
  pinMode(BUTTON_A, INPUT_PULLUP);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  
  // Clear display
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Connecting to wifi...");
  display.display();
  
  // Construct the wifiManager which will ensure we are connected to a 
  // wifi hotspot.  If this fails then it will construct an AP and 
  // block for configuration.
  WiFiManager wifiManager;
  wifiManager.setAPCallback(onFailedConnect);
  wifiManager.autoConnect(AP_STATION_SSID, AP_STATION_PASSWORD);

#if defined(USE_MDNS)
  // Start the mDNS responder for airquality.local
  if (!MDNS.begin("airquality")) {             
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
  }
#endif

  ArduinoOTA.setHostname("ESP8266-AirQuality");
  ArduinoOTA.onStart([]() {
      Serial.println("OTA: Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA: End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  Serial.println("Starting Air quality sensor...");
	pms.begin();
	pms.waitForData(Pmsx003::wakeupTime);
	pms.write(Pmsx003::cmdModeActive);
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connected to:");
  display.println(WiFi.localIP());
  display.printf("\nWarming up...");
  display.display();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/realtimeview", HTTP_GET, handleRealTimeView);
  server.on("/dateview", HTTP_GET, handleDateView);
  server.begin();
 
  connect_mqtt();
}

void onFailedConnect(WiFiManager *myManager) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("Configure required\n");
  display.printf("SSID: %s\n", AP_STATION_SSID);
  display.printf("Password: %s\n", AP_STATION_PASSWORD);
  display.display();
}

void connect_mqtt() {
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setClient(WIFI_CLIENT);
  Serial.println("Attempting to connect MQTT");
  mqtt_client.connect("airqualitysensor");
  if (!mqtt_client.connected()) {
    Serial.println("Failed to connect MQTT");
  } else {
    Serial.println("Connected to MQTT");
  }
}

bool isWarmup = true;
long startMillis = millis();
long nextReadMillis = millis();
long nextMqttPublishMillis = millis();
long lastMqttPublishMillis = millis();
long timesRead = 0;
long timesPublished = 0;

const auto n = Pmsx003::Reserved;

void potentially_report_pms_data(const Pmsx003::pmsData data[Pmsx003::Reserved]) {
  if (millis() > nextMqttPublishMillis) { 
    // Push data to MQTT if we are connected - otherwise attempt to reconnect.
    if (!mqtt_client.connected()) {
      connect_mqtt();
    } else { 
      char mqttMessage[100];

      sprintf(mqttMessage, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", 
          data[Pmsx003::PM1dot0CF1 ],
          data[Pmsx003::PM2dot5CF1],
          data[Pmsx003::PM10dot0CF1],
          data[Pmsx003::Particles0dot3],
          data[Pmsx003::Particles0dot5],
          data[Pmsx003::Particles1dot0],
          data[Pmsx003::Particles2dot5],
          data[Pmsx003::Particles5dot0],
          data[Pmsx003::Particles10],
          (int) (millis() - lastMqttPublishMillis)
          );

      mqtt_client.publish("aq", mqttMessage);
      timesPublished++;
      lastMqttPublishMillis = millis();
    }
    nextMqttPublishMillis += NEXT_PUBLISHING_MILLIS;
  }
}

char getSingleGlyph(int value) { 
  switch( value % 4) { 
    case 0: return '-';
    case 1: return '\\';
    case 2: return '|';
    case 3: return '/';
  }
  return '?';
}

void loop(void) {

  ArduinoOTA.handle();

  if (isWarmup) { 
    long elapsedMillis = millis() - startMillis;
    if (elapsedMillis > WARMUP_MILLIS) { 
      isWarmup = false;
    } 
    return;
  }

  // From here down we are ensured that we are no longer in the 
  // warmup state and we can proceed with business as usual.

  // See if we should fetch data in this loop.
  
  long startDataFetchMillis = millis();
  if (startDataFetchMillis >= nextReadMillis) { 

    Pmsx003::pmsData data[n];
    Pmsx003::PmsStatus status = pms.read(data, n);

    long dataFetchedMillis = millis();

    switch (status) {
      case Pmsx003::OK:
        {
          timesRead++;

          // Display the new data on the OLED.
          display.clearDisplay();
          display.setCursor(0,0);
          display.print(WiFi.localIP());
          if (mqtt_client.connected()) {
            display.println(" | OK");
          } else {
            display.println(" | BAD");
          }
        
          // A single character glyph is useful to show when the data has been
          // read and also published.  
          char r = getSingleGlyph(timesRead);
          char p = getSingleGlyph(timesPublished);
          display.printf( "             [%c]  [%c]\n", r,p);

          display.printf( "%-5d %s\n", data[Pmsx003::PM1dot0CF1], shortNames[Pmsx003::PM1dot0CF1]);
          display.printf( "%-5d %s\n", data[Pmsx003::PM10dot0CF1], shortNames[Pmsx003::PM10dot0CF1]);
          display.display();

          nextReadMillis = millis();
          long elapsedMillis = millis() - startDataFetchMillis;
          if (elapsedMillis < NEXT_READING_MILLIS) { 
            nextReadMillis += (NEXT_READING_MILLIS - elapsedMillis); 
          }
          potentially_report_pms_data(data);
          break;
        }
      case Pmsx003::noData:
        break;
      default:
        Serial.printf("---  PMS Error: %s\n",Pmsx003::errorMsg[status]);
    };
  }
  server.handleClient();
}


