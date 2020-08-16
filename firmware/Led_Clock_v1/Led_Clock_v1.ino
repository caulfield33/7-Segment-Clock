#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <FastLED.h>
#include <FS.h>
#include <ArduinoOTA.h>


// ==================== Web Server ==========================

AsyncWebServer server(80);
DNSServer dns;

void notFound(AsyncWebServerRequest *request) {
  Serial.println("Server not fount");
  request->send(SPIFFS, "/index.html", String(), false, initProcessor);
}

// ==========================================================

// ====================== WIFI Setup ========================

void configModeCallback (AsyncWiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());

}

AsyncWiFiManager wifiManager(&server, &dns);

// ==========================================================

// ==================== FastLED Setup =======================

#define NUM_LEDS 30
#define DATA_PIN D2
CRGB leds[NUM_LEDS];

// ==========================================================

// ========================== OTA ===========================

#define SENSORNAME "7segmentClock"
#define OTApassword "7segmentClock"
int OTAport = 8266;

// ==========================================================

// ========================= Vars ===========================

int clock_state = 1;
int clock_digit_update_mode = 0;
int time_zone = 1;
// ==========================================================

void setup() {
  Serial.begin(115200);

  // ========================== FS ===========================

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
  } else {
    Serial.println("failed to mount FS");
  }

  // ======================== End FS =========================


  // ==================== FastLED Setup =======================

  FastLED.addLeds<WS2812B, DATA_PIN>(leds, NUM_LEDS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

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

  // ========================== OTA ===========================
  //OTA SETUP
  ArduinoOTA.setPort(OTAport);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(SENSORNAME);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)OTApassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
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

  // ========================== End OTA =======================

  // ==================== Web Server ==========================

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.println("Server index");

    request->send(SPIFFS, "/index.html", String(), false, initProcessor);
  });
  server.onNotFound(notFound);
  server.begin();
  Serial.println("HTTP server started");
  // =================== End Web Server ========================

}

String initProcessor(const String& var) {
  Serial.println("Processor var: " + var);
  if (var == "DATA") {
    String json = "{";

    json += "\"state\":" + String(clock_state) + ",";
    json += "\"timeZone\":" + String(time_zone) + ",";
    json += "\"mode\":" + String(clock_digit_update_mode);

    json += "}";
    Serial.println("Processor data: " + json);

    return json;
  }

  return "";
};

void online() {
  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    Serial.print("WIFI Disconnected. Attempting reconnection.");
    if (!wifiManager.autoConnect()) {
      Serial.println("failed to connect and hit timeout");
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(1000);
    }
  }
  ArduinoOTA.handle();
  //  server.handleClient();
}

void loop() {
  // ==================== Web Server ==========================

  // =================== End Web Server ========================


  online();
}
