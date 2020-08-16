#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"
#include <ArduinoJson.h> 
#include <FastLED.h>

// ====================== WIFI Setup ========================

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());

}

WiFiManager wifiManager;

// ==================== End WIFI Setup ======================

// ==================== FastLED Setup =======================

#define NUM_LEDS 30
#define DATA_PIN D2

// =================== End FastLED Setup ====================

// ========================= Vars ===========================

int clock_state = 1;
int clock_digit_update_mode = 0;

// ======================= End Vars =========================

// ==================== Web Server ==========================

ESP8266WebServer server(80);

// =================== End Web Server ========================

void setup() {
  Serial.begin(115200);

  // ==================== FastLED Setup =======================
  
  FastLED.addLeds<WS2812B, DATA_PIN>(leds, NUM_LEDS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);

// =================== End FastLED Setup ====================

  // ====================== WIFI Setup ========================
  

  wifiManager.autoConnect("Led Clock Setup");

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  // ==================== End WIFI Setup ======================
  

  // ==================== Web Server ==========================
  server.serveStatic("/", SPIFFS, "/index.html")
  
  // =================== End Web Server ========================

}


void connection_check() {
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
}

void loop() {
  // ==================== Web Server ==========================
  WiFiClient client = server.available();
  rest.handle(client);
  // =================== End Web Server ========================


  connection_check();
}
