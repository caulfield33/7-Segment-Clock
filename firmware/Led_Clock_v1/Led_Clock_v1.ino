#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <FS.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>

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
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

AsyncWiFiManager wifiManager(&server, &dns);

// ==========================================================

// ==================== FastLED Setup =======================

#define NUM_LEDS 30
#define DATA_PIN D2
CRGB leds[NUM_LEDS];
CHSV color = CHSV(0, 0, 0);
CHSV stored_color;
CHSV new_color = CHSV(128, 255, 128);

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
bool is24 = true;
unsigned long ntp_begin;
int digits[9][7] = {
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1}
};
int dots[2] = {1, 1};

// ==========================================================

// ========================== Time ===========================

time_t getNtpTime () {
  Serial.print("Synchronize NTP ...");
  ntp_begin = millis();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) < 1500000000) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" OK");

  return time(nullptr) + time_zone;
}

void setup() {
  Serial.begin(115200);
  // ========================== Time ===========================

  time_t nowUtc = getNtpTime();
  setSyncProvider(getNtpTime);
  setSyncInterval(3600);

  // ==========================================================

  // ========================== FS ===========================

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");

    if (SPIFFS.exists("/config.json")) {}
  } else {
    Serial.println("failed to mount FS");
  }

  // ==========================================================


  // ==================== FastLED Setup =======================

  FastLED.addLeds<WS2812B, DATA_PIN>(leds, NUM_LEDS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // ==========================================================

  // ====================== WIFI Setup ========================


  wifiManager.autoConnect("Led Clock Setup");

  //reset settings - for testing
  //wifiManager.resetSettings();

  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    ESP.reset();
    delay(1000);
  }
  // ==========================================================

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

  // ==========================================================

  // ==================== Web Server ==========================

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.println("Server index");

    request->send(SPIFFS, "/index.html", String(), false, initProcessor);
  });
  server.onNotFound(notFound);
  server.begin();
  Serial.println("HTTP server started");
  // ==========================================================

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
      ESP.reset();
      delay(1000);
    }
  }
  ArduinoOTA.handle();
}

void loop() {
  displayTime();

  online();
}

void displayTime() {

  //  fill_solid(leds, NUM_LEDS, CRGB::Black);

  int display_hour = is24 ? hour() : hourFormat12();
  int display_minute = minute();

  int hour1 = display_hour / 10;
  int hour2 = display_hour % 10;


  int minute1 = (display_minute < 10) ? 0 : display_minute / 10;
  int minute2 = display_minute % 10;

  shopTimeOnClock(hour1, hour2, minute1, minute2);

  //  FastLED.show();
}

void shopTimeOnClock(int hour1, int hour2, int minute1, int minute2) {
  Serial.print(hour1);
  Serial.print(hour2);
  Serial.print(":");
  Serial.print(minute1);
  Serial.print(minute2);
  Serial.println("");
}
