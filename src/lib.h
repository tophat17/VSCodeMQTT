#if defined(ESP8266)
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#error "This isn't an ESP8266 or ESP32!"
#endif

#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <PubSubClient.h>

#include <string>
#include <memory>
using namespace std;
