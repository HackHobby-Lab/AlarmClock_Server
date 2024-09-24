// #include <Arduino.h>
#include <WiFi.h>
#include "localServer.h"
#include "Display.h"

void setup_wifi() {
  delay(10);
  setupPeripheral(); // only for testing
  // EEPROM.begin(512); // only for testing
  // rtc.begin(); // only for testing
  Serial.println();
  preferences.begin("wifi", false); // REVISIT: Already initialized

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
    // WiFi.config(local_IP, gateway, subnet); // Set static IP for STA mode
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.printf("Connecting to WiFi SSID: %s \n", ssid.c_str());
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < wifiTimeout) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\nConnected to WiFi\n");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      preferences.putInt("resetCount", 0); 

      // Start the web server in STA mode
      startWebServer();

    } else {
      Serial.println("Failed to connect to WiFi, Restarting ESP32...");
      delay(1000); 
      ESP.restart();
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

  // REVISIT: ADD CHECKS WHEN TO START mDNS RESPONDER??
  // Start mDNS responder
  if (!MDNS.begin("esp32")) {
    Serial.println("Error starting mDNS");
    return;
  }
  Serial.println("mDNS responder started");

}

void loop() {
  server.handleClient();
  displayClock(); // only for testing
}
