#include "localServer.h"

// Define the global variables
Preferences preferences;
WebServer server(80);
HTTPClient http;

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

String savedSettings;

// Hue Emulator details
const char* hueEmulatorIP = "192.168.0.111";
String bridgeIP;
String apiUsername;
const int lightID_1 = 1;  // Light ID for wakeup
const int lightID_2 = 2;  // Light ID for sleep

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
  String volume = server.arg("volume");
  Serial.println(volume);
  server.send(200, "text/plain", "Volume set to " + volume);
}

void setWakeupSound() {
  String sound = server.arg("sound");
  Serial.println("Selected Wake-up Sound: " + sound);
  server.send(200, "text/plain", "Wake-up sound set to " + sound);
}

// Function to handle the preview of the wake-up sound
void previewWakeupSound() {
  if (server.hasArg("sound")) {
    String sound = server.arg("sound");
    Serial.println("Previewing Wake-up Sound: " + sound);

    // Here you would add the code to actually play the sound using your hardware
    // For example, playSound(sound);  // This function would be defined to play the sound

    server.send(200, "text/plain", "Previewing sound " + sound);
  } else {
    server.send(400, "text/plain", "Bad Request: 'sound' argument missing");
  }
}

void setSleepSound() {
  String sound = server.arg("sound");
  Serial.println("Selected Sleep Sound: " + sound);
  server.send(200, "text/plain", "Sleep sound set to " + sound);
}

// Function to handle the preview of the Sleep sound
void previewSleepSound() {
  if (server.hasArg("sound")) {
    String sound = server.arg("sound");
    Serial.println("Previewing Sleep Sound: " + sound);

    // Here you would add the code to actually play the sound using your hardware
    // For example, playSound(sound);  // This function would be defined to play the sound

    server.send(200, "text/plain", "Previewing sound " + sound);
  } else {
    server.send(400, "text/plain", "Bad Request: 'sound' argument missing");
  }
}

// Function to handle searching for the Philips Hue Bridge using mDNS
void handleSearchForBridge() {
  int n = MDNS.queryService("_mi-connect", "tcp"); // Search for services named "hue" over TCP
  
  if (n == 0) {
    Serial.println("No Philips Hue Bridge found");
    server.send(404, "text/plain", "No Philips Hue Bridge found");
  } else {
    // Assume the first result is the desired Philips Hue Bridge
    bridgeIP = MDNS.IP(0).toString();
    Serial.println("Philips Hue Bridge found at IP: " + bridgeIP);
    server.send(200, "text/plain", bridgeIP);
  }
}

void handleAddManually() {
  if (server.hasArg("plain")) {
    String requestBody = server.arg("plain");
    
    // Extract IP from requestBody (assuming JSON format)
    int ipIndex = requestBody.indexOf("ip\":\"") + 5;
    if (ipIndex != -1) {
      int ipEndIndex = requestBody.indexOf("\"", ipIndex);
      bridgeIP = requestBody.substring(ipIndex, ipEndIndex);
      
      // Save or use the bridge IP (replace with actual storage or usage logic)
      Serial.println("Bridge IP added manually: " + bridgeIP);
      
      // Request the username from the bridge
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = "http://" + bridgeIP + "/api";
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST("{\"devicetype\":\"hue#AlarmClock\"}");
        
        if (httpResponseCode == 200) {
          String response = http.getString();
          Serial.println("Response from bridge: " + response);
          
          // Extract username from response (assuming JSON format)
          int usernameStart = response.indexOf("username\":\"") + 11;
          if (usernameStart != -1) {
            int usernameEnd = response.indexOf("\"", usernameStart);
             apiUsername = response.substring(usernameStart, usernameEnd);
            Serial.println("Username: " + apiUsername);
            
            // Respond to the client with the username
            server.send(200, "text/plain", "Bridge added successfully. Username: " + apiUsername);
          } else {
            server.send(400, "text/plain", "Username not found in response");
          }
        } else {
          Serial.println("Error getting response from bridge: " + String(httpResponseCode));
          server.send(400, "text/plain", "Failed to get username from bridge");
        }
        
        http.end();
      } else {
        server.send(400, "text/plain", "WiFi not connected");
      }
    } else {
      server.send(400, "text/plain", "Invalid request format");
    }
  } else {
    server.send(400, "text/plain", "No data received");
  }
}

// Function to send the light state to the Hue Emulator
void setLightState(int lightID, bool turnOn) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = "http://" + String(bridgeIP) + "/api/" + String(apiUsername) + "/lights/" + String(lightID) + "/state";
    
    Serial.println("URL: " + url);
    
    // Prepare the HTTPClient
    http.begin(url); // Use HTTP URL
    http.addHeader("Content-Type", "application/json");
    
    // Construct the payload
    String payload = turnOn ? "{\"on\": true}" : "{\"on\": false}";
    Serial.println("Payload: " + payload);

    // Send HTTP PUT request
    int httpResponseCode = http.PUT(payload);
    
    // Check the response
    Serial.println("HTTP Response Code: " + String(httpResponseCode));
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error Response Code: " + String(httpResponseCode));
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected. Status: " + String(WiFi.status()));
  }
}

// Function to handle saving settings
void handleSaveSettings() {
  if (server.hasArg("plain")) {
    String requestBody = server.arg("plain");
    
    // Parse JSON data
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, requestBody);
    
    if (error) {
      server.send(400, "text/plain", "Invalid JSON format");
      return;
    }
    
    // Extract settings from the JSON object
    bool hueToggle = doc["hueToggle"];
    String wakeUpScene = doc["wakeUpScene"].as<String>();
    int fadeStart = doc["fadeStart"];
    String customButton = doc["customButton"].as<String>();
    String customScene = doc["customScene"].as<String>();
    int fadeInCustom = doc["fadeInCustom"];

    // Example: Save these settings in a global variable or EEPROM
    savedSettings = requestBody; // You can also use EEPROM.write() or another storage method

    Serial.println("Settings saved:");
    Serial.println(savedSettings);
    
    // Send success response
    server.send(200, "text/plain", "Settings saved successfully");
  } else {
    server.send(400, "text/plain", "No data received");
  }
}

// Function to handle resetting settings
void handleResetSettings() {
  // Example of resetting to default values (you can define your own defaults)
  savedSettings = "";
  
  Serial.println("Settings reset to defaults.");
  
  // Send success response
  server.send(200, "text/plain", "Settings reset successfully");
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
  server.on("/setWakeupSound", setWakeupSound);
  server.on("/previewWakeupSound", HTTP_POST, previewWakeupSound);
  server.on("/setSleepSound", setSleepSound); 
  server.on("/previewSleepSound", HTTP_POST, previewSleepSound);
  server.on("/searchForBridge", HTTP_POST, handleSearchForBridge);
  server.on("/addManually", HTTP_POST, handleAddManually);
  server.on("/saveSettings", HTTP_POST, handleSaveSettings);
  server.on("/resetSettings", HTTP_POST, handleResetSettings);

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

