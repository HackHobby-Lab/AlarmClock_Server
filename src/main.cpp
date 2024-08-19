#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <FS.h>
#include <SPIFFS.h>

Preferences preferences;
WebServer server(80);

const char* apSSID = "AlarmClock";
const char* apPassword = "12345678";
const int resetThreshold = 5;
WiFiClient espClient;

// Define the static IP addresses
IPAddress local_IP(192, 168, 156, 176);    // Static IP address in STA mode
IPAddress gateway(192, 168, 1, 1);       // Gateway IP address
IPAddress subnet(255, 255, 255, 0);      // Subnet mask
IPAddress ap_IP(192, 168, 156, 176);         // Static IP address in AP mode




const int ledPin = 2;  // LED pin, change this according to your setup
bool ledState = false;

void toggleLED() {
  ledState = !ledState;
  digitalWrite(ledPin, ledState ? HIGH : LOW);
  server.send(200, "text/plain", ledState ? "LED is ON" : "LED is OFF");
}


void startWebServer() {
  server.on("/", HTTP_GET, []() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      Serial.println("Failed to open file");
      server.send(500, "text/plain", "Internal Server Error");
      return;
    }

    String htmlContent;
    while (file.available()) {
      htmlContent += file.readString();
    }
    file.close();

    server.send(200, "text/html", htmlContent);
  });

  server.on("/toggleLED", HTTP_GET, toggleLED);

  server.on("/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();
    String networks = "[";
    for (int i = 0; i < n; ++i) {
      if (i > 0) networks += ",";
      networks += "\"" + WiFi.SSID(i) + "\"";
    }
    networks += "]";
    server.send(200, "application/json", networks);
  });

  server.on("/wifiStatus", HTTP_GET, []() {
    String ssid = WiFi.SSID();
    String ip = WiFi.localIP().toString();
    String json = "{\"ssid\":\"" + ssid + "\", \"ip\":\"" + ip + "\"}";
    server.send(200, "application/json", json);
  });

  server.on("/setWiFi", HTTP_POST, []() {
    String newSSID = server.arg("ssid");
    String newPass = server.arg("pass");

    preferences.putString("ssid", newSSID);
    preferences.putString("password", newPass);

    server.send(200, "text/plain", "Settings saved, ESP will restart now.");
    delay(1000);
    ESP.restart();
  });

  server.begin();
  Serial.println("HTTP server started");
}

void startAPMode() {
  WiFi.softAPConfig(ap_IP, ap_IP, subnet); // Set static IP for AP mode
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point started with IP: ");
  Serial.println(WiFi.softAPIP());

  startWebServer();
}


void setup_wifi() {
  delay(10);
  Serial.println();
  preferences.begin("wifi", false);

  int resetCount = preferences.getInt("resetCount", 0);
  resetCount++;
  preferences.putInt("resetCount", resetCount);
  Serial.printf("Reset count: %d\n", resetCount);

  if (resetCount >= resetThreshold) {
    Serial.println("Reset count threshold reached, clearing preferences.");
    preferences.clear();  
    preferences.putInt("resetCount", 0); 
    delay(1000); 
    ESP.restart();
  }

  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");

  if (ssid != "") {
    WiFi.config(local_IP, gateway, subnet); // Set static IP for STA mode
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.printf("Connecting to WiFi SSID: %s \n", ssid.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      preferences.putInt("resetCount", 0); 

      // Start the web server in STA mode
      startWebServer();

    } else {
      Serial.println("Failed to connect to WiFi, starting AP mode.");
      startAPMode();
    }
  } else {
    startAPMode();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  preferences.begin("wifi", false);
  setup_wifi();
}

void loop() {
  server.handleClient();
}
