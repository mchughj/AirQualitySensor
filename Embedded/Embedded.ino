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

// Include the webpage
#include "webpage.h"

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

#define ESP_RX 13
#define ESP_TX 13

#define BUTTON_A  0
#define BUTTON_B 16
#define BUTTON_C  2

#define MQTT_SERVER "192.168.50.41"
#define MQTT_PORT 1883

#define DO_WEBSERVER
#ifdef DO_WEBSERVER
#include <ESP8266WebServer.h>
#endif
  
//#define USE_MDNS
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
#define NEXT_READING_MILLIS 2000

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

void handleRoot();
void handleNotFound();
void onFailedConnect(WiFiManager*);

void setup(void) {
	Serial.begin(115200);
	while (!Serial) {};
  
  pinMode(BUTTON_A, INPUT_PULLUP);
  
  Serial.println("Starting Air quality sensor...");
	pms.begin();
	pms.waitForData(Pmsx003::wakeupTime);
	pms.write(Pmsx003::cmdModeActive);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  
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

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connected to:");
  display.println(WiFi.localIP());
  display.printf("\nWarming up...");
  display.display();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/message", HTTP_POST, handleMessage);
  server.onNotFound(handleNotFound);
  server.begin();
}

void onFailedConnect(WiFiManager *myManager) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("Unable to connect.  Please configure.\n");
  display.printf("SSID: %s\n", AP_STATION_SSID);
  display.printf("Password: %s\n", AP_STATION_PASSWORD);
  display.display();
}

bool reconnect_mqtt() {
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setClient(WIFI_CLIENT);
  Serial.println("Attempting to connect MQTT");
  mqtt_client.connect("airqualitysensor");
  delay(500);
  if (!mqtt_client.connected()) {
    Serial.println("Failed to connect MQTT");
    return false;
  }
  Serial.println("Connected to MQTT");
  return true;
}

bool isWarmup = true;
long startMillis = millis();
long lastReadMillis = millis();
const auto n = Pmsx003::Reserved;
Pmsx003::pmsData lastData[n];

void loop(void) {

  if (isWarmup) { 
    long elapsedMillis = millis() - startMillis;
    if (elapsedMillis > WARMUP_MILLIS) { 
      lastReadMillis = millis();
      isWarmup = false;
    } 
    delay(100);
    return;
  }

  // From here down we are ensured that we are no longer in the 
  // warmup state and we can proceed with business as usual.

  long startDataFetchMillis = millis();

	Pmsx003::pmsData data[n];
	Pmsx003::PmsStatus status = pms.read(data, n);

  long dataFetchedMillis = millis();
  
	switch (status) {
		case Pmsx003::OK:
		{
      // Push to display
      display.clearDisplay();
      display.setCursor(0,0);
      display.print(WiFi.localIP());
      if (mqtt_client.connected()) {
        display.println(" | OK");
      } else {
        display.println(" | BAD");
      }
      display.println();
      display.printf( "%-5d %s\n", data[Pmsx003::PM1dot0CF1], shortNames[Pmsx003::PM1dot0CF1]);
      display.printf( "%-5d %s\n", data[Pmsx003::PM10dot0CF1], shortNames[Pmsx003::PM10dot0CF1]);
      display.display();
      
      // Copy the data
      memcpy(lastData, data, sizeof(data[0])*n);
      
      // Push MQTT data
      if (mqtt_client.connected() || reconnect_mqtt()) {
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
          (int) (millis() - lastReadMillis)
          );
          
        mqtt_client.publish("aq", mqttMessage);
      }

      long elapsedMillis = millis() - startDataFetchMillis;
      if (elapsedMillis < NEXT_READING_MILLIS) { 
#ifdef SHOW_TIME_ELAPSED
        Serial.printf("Total Elapsed: %d, PMS read: %d, sleeping for: %d\n", 
          elapsedMillis,
          (dataFetchedMillis - startDataFetchMillis),
          (NEXT_READING_MILLIS - elapsedMillis));
#endif
        delay(NEXT_READING_MILLIS - elapsedMillis);
      }
      lastReadMillis = dataFetchedMillis;
			break;
		}
		case Pmsx003::noData:
			break;
		default:
			Serial.printf("---  PMS Error: %s\n",Pmsx003::errorMsg[status]);
	};
  server.handleClient();
}

void handleRoot() {
  /*
  String msg = "<!DOCTYPE html><html><head><title>Air Quality Sensor</title><style>table,th,td{border:1px solid black;border-collapse:collapse;}</style></head><body><table>";
  for (size_t i = Pmsx003::PM1dot0CF1; i <= Pmsx003::Particles10; ++i) {
    msg = msg + "<tr><td>" + String(lastData[i]) + "</td><td>" + String(Pmsx003::dataNames[i]) + "</td><td>" + String(Pmsx003::metrics[i]) + "</td></tr>";
  }
  msg = msg + "</table>";
  //msg = msg + "<br /><form action=\"/message\" method=\"POST\"><input type=\"text\" name=\"msg\"><input type=\"submit\" value=\"Submit\"></form>";
  msg = msg + "<script>setTimeout(()=>{window.location.replace(\"/\");}, 5000);</script>";
  msg = msg + "</body></html>";
  server.send(200, "text/html", msg);
  */

  server.send(200, "text/html", PAGEHTML);
}

void handleMessage() {
  Serial.println("Got message request");
  String msg = server.arg("msg");
  Serial.print("Got message: \"");
  Serial.print(msg);
  Serial.println("\"");
  
  // Display the message given to us.
  // Clear display first
  display.clearDisplay();
  display.setCursor(0,0);
  // Now write the message all in one go.
  // The message may be cut off.
  display.print(msg);
  display.display();
  
  // Redirect back to the main page.
  server.sendHeader("Location","/");
  server.send(303);
}

// Send HTTP status 404 (Not Found) when there's no handler for the URI in the
// request
void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); 
}

