#include "localServer.h"

// Define the global variables
Preferences preferences;
WebServer server(80);

const char* apSSID = "AlarmClock";
const char* apPassword = "12345678";
const int resetThreshold = 5;
WiFiClient espClient;

// Define the static IP addresses
IPAddress local_IP(192, 168, 1, 148);    // Static IP address in STA mode
IPAddress gateway(192, 168, 1, 1);         // Gateway IP address
IPAddress subnet(255, 255, 255, 0);        // Subnet mask
IPAddress ap_IP(192, 168, 1, 148);       // Static IP address in AP mode

const int ledPin = 2;  // LED pin, change this according to your setup
bool ledState = false;

void toggleLED() {
  ledState = !ledState;
  digitalWrite(ledPin, ledState ? HIGH : LOW);
  analogWrite(ledPin, ledState? 255 : 0);
  Serial.println(ledState ? "LED is ON" : "LED is OFF");
  server.send(200, "text/plain", ledState ? "LED is ON" : "LED is OFF");
}
void setBrightness() {
  if (server.hasArg("value")) {
    int brightness = server.arg("value").toInt();
    analogWrite(ledPin, brightness);  // Set LED brightness
    server.send(200, "text/plain", "Brightness set to " + String(brightness));
    Serial.println(brightness);
  } else {
    server.send(400, "text/plain", "Bad Request: No value provided");
  }
}

void setAlarm(){
  if(server.hasArg("alarmTime")){
    String alarmTime = server.arg("alarmTime");

    Serial.println("Alarm Time: " + alarmTime);

    server.send(200, "text/plain", "Alarm Time set to: " + alarmTime);
  }
  else {
    server.send(400, "text/plain", "Bad Request: Alarm Date not Provided");
  }
}
void setAmPm(){
  if(server.hasArg("amPmValue")){
    Serial.println("AM/PM");
  }
}

void setDateTime() {
  if (server.hasArg("date") && server.hasArg("time")) {
    String date = server.arg("date");
    String time = server.arg("time");
    // String ampm = server.hasArg("ampm") ? server.arg("ampm") : ""; // Use this if you're handling AM/PM
    // String ampm = server.hasArg("ampm") ? server.arg("ampm") : "";
    // String amData = server.hasArg("data") ? server.arg("time") : "" ; // amData
    // String pmData = server.hasArg("time") ? server.arg("time") : "";  //pmData
    // Log the received date and time
    Serial.println("Received Date: " + date);
    Serial.println("Received Time: " + time);
    // Serial.println("AM/PM: " + ampm); // Uncomment if handling AM/PM

    // Send a success response to the client
    server.send(200, "text/plain", "Date and Time set to: " + date + " " + time /* + " " + ampm */);
  } else {
    server.send(400, "text/plain", "Bad Request: Date or Time not provided");
  }
}


void changeVolume(){
   if (server.hasArg("data")) {
    // int volume = server.arg("data").toInt();
    // //analogWrite(ledPin, volume);  // Set LED brightness
    // server.send(200, "text/plain", "Volume set to " + String(volume));
    Serial.println("volume");
  // } else {
  //   server.send(400, "text/plain", "Bad Request: No value provided");
  }

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
  // Define web routes
  server.on("/toggleLED", toggleLED);
  server.on("/setBrightness", setBrightness);
  server.on("/setDateTime", setDateTime);
  server.on("/addAlarm", setAlarm);
  server.on("/amPmValue", setAmPm);
  server.on("/changeVolume", changeVolume);

  // server.on("/toggleLED", HTTP_GET, toggleLED);

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

