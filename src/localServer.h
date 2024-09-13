#ifndef LOCALSERVER
#define LOCALSERVER

#include <WebServer.h>
#include <Preferences.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

extern Preferences preferences;
extern WebServer server;

extern const char* apSSID;
extern const char* apPassword;
extern const int resetThreshold;
extern WiFiClient espClient;

// Define the static IP addresses
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress ap_IP;

extern const int ledPin;
extern bool ledState;

void toggleLED();
void startWebServer();
void startAPMode(); 
void fetchScenesAndSendToClient();
#endif // LOCALSERVER
