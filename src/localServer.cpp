#include "localServer.h"
#include "Display.h"

// Define the global variables
Preferences preferences;
WebServer server(80);
HTTPClient http;

const char *apSSID = "AlarmClock";
const char *apPassword = "12345678";
const int resetThreshold = 5;
WiFiClient espClient;

// Define the static IP addresses
IPAddress local_IP(192, 168, 1, 148); // Static IP address in STA mode
IPAddress gateway(192, 168, 1, 1);    // Gateway IP address
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress ap_IP(192, 168, 1, 148);    // Static IP address in AP mode

const int ledPin = 2; // LED pin, change this according to your setup
bool ledState = false;

// Hue Emulator details
const char *hueEmulatorIP = "192.168.0.111";
String bridgeIP;
String apiUsername;
const int lightID_1 = 1; // Light ID for wakeup
const int lightID_2 = 2; // Light ID for sleep

void toggleLED()
{
  ledState = !ledState;
  digitalWrite(ledPin, ledState ? HIGH : LOW);
  analogWrite(ledPin, ledState ? 255 : 0);
  Serial.println(ledState ? "LED is ON" : "LED is OFF");
  server.send(200, "text/plain", ledState ? "LED is ON" : "LED is OFF");
}

void setBrightness()
{
  if (server.hasArg("value"))
  {
    int brightness = server.arg("value").toInt();
    analogWrite(ledPin, brightness); // Set LED brightness
    server.send(200, "text/plain", "Brightness set to " + String(brightness));
    Serial.println(brightness);
  }
  else
  {
    server.send(400, "text/plain", "Bad Request: No value provided");
  }
}

void setAlarm()
{
  if (server.hasArg("alarmTime"))
  {
    String alarmTime = server.arg("alarmTime");

    // Log the received alarm time
    Serial.println("Alarm Time: " + alarmTime);

    // Parse the alarm time
    int hour = alarmTime.substring(0, 2).toInt();
    int minute = alarmTime.substring(3, 5).toInt();
    int second = 10;

    // Determine if it's AM or PM based on 24-hour format
    bool isPM = false;
    String amPmString;

    if (hour >= 12)
    {
      isPM = true;
      amPmString = "PM";
    }
    else
    {
      amPmString = "AM";
    }

    Serial.print("Time: ");
    Serial.print(hour);
    Serial.print(":");
    Serial.print(minute);
    Serial.print(" ");
    Serial.println(amPmString);

    // Call the function to set the alarm in another file
    setAlarmTime(hour, minute, second, isPM);

    // Send a success response to the client
    server.send(200, "text/plain", "Alarm Time set to: " + alarmTime);
  }
  else
  {
    server.send(400, "text/plain", "Bad Request: Alarm Time not provided");
  }
}

void setAmPm()
{
  if (server.hasArg("amPmValue"))
  {
    Serial.println("AM/PM");
  }
}

void setDateTime()
{
  if (server.hasArg("date") && server.hasArg("time"))
  {
    String date = server.arg("date");
    String time = server.arg("time");

    // Log the received date and time
    Serial.println("Received Date: " + date);
    Serial.println("Received Time: " + time);

    // Parse the date (assumes format YYYY-MM-DD)
    int year = date.substring(0, 4).toInt();
    int month = date.substring(5, 7).toInt();
    int day = date.substring(8, 10).toInt();

    // Parse the time (assumes format HH:MM:SS)
    int hour = time.substring(0, 2).toInt();
    int minute = time.substring(3, 5).toInt();
    int second = time.substring(6, 8).toInt();

    // Call the time-setting function from another file
    setTimeFromValues(year, month, day, hour, minute, second);

    // Send a success response to the client
    server.send(200, "text/plain", "Date and Time set to: " + date + " " + time);
  }
  else
  {
    server.send(400, "text/plain", "Bad Request: Date or Time not provided");
  }
}

void changeVolume()
{
  String level = server.arg("volume");
  int volume = level.toInt();

  if (volume >= 0 && volume <= 100)
  {
    setDFPlayerVolume(volume); // Call function
    Serial.println("Volume set to: " + String(volume));
  }
  Serial.println(level);
  server.send(200, "text/plain", "Volume set to " + volume);
}

void setWakeupSound()
{
  String sound = server.arg("sound");
  Serial.println("Selected Wake-up Sound: " + sound);
  server.send(200, "text/plain", "Wake-up sound set to " + sound);
}

// Function to handle the preview of the wake-up sound
void previewWakeupSound()
{
  if (server.hasArg("sound"))
  {
    String track = server.arg("sound");
    int sound = 0;
    if (track == "alarm1")
    {
      sound = 1;
    }
    if (track == "alarm2")
    {
      sound = 2;
    }
    if (track == "alarm3")
    {
      sound = 3;
    }

    playSound(sound); // Preview sound using function
    Serial.println("Previewing sound track: " + String(sound));
    Serial.println("Previewing Wake-up Sound: " + track);

    server.send(200, "text/plain", "Previewing sound " + sound);
  }
  else
  {
    server.send(400, "text/plain", "Bad Request: 'sound' argument missing");
  }
}

void setSleepSound()
{
  String sound = server.arg("sound");
  Serial.println("Selected Sleep Sound: " + sound);
  server.send(200, "text/plain", "Sleep sound set to " + sound);
}

// Function to handle the preview of the Sleep sound
void previewSleepSound()
{
  if (server.hasArg("sound"))
  {
    String sound = server.arg("sound");
    Serial.println("Previewing Sleep Sound: " + sound);

    server.send(200, "text/plain", "Previewing sound " + sound);
  }
  else
  {
    server.send(400, "text/plain", "Bad Request: 'sound' argument missing");
  }
}

// Function to handle searching for the Philips Hue Bridge using mDNS
void handleSearchForBridge()
{
  int n = MDNS.queryService("_http", "tcp"); // Search for services named "hue" over TCP

  if (n == 0)
  {
    Serial.println("No Philips Hue Bridge found");
    server.send(404, "text/plain", "No Philips Hue Bridge found");
  }
  else
  {
    // Assume the first result is the desired Philips Hue Bridge
    bridgeIP = MDNS.IP(0).toString();
    Serial.println("Philips Hue Bridge found at IP: " + bridgeIP);
    server.send(200, "text/plain", bridgeIP);
  }
}

void handleAddManually()
{
  if (server.hasArg("plain"))
  {
    String requestBody = server.arg("plain");

    // Extract IP from requestBody (assuming JSON format)
    int ipIndex = requestBody.indexOf("ip\":\"") + 5;
    if (ipIndex != -1)
    {
      int ipEndIndex = requestBody.indexOf("\"", ipIndex);
      bridgeIP = requestBody.substring(ipIndex, ipEndIndex);

      // Save or use the bridge IP (replace with actual storage or usage logic)
      Serial.println("Bridge IP added manually: " + bridgeIP);

      // Request the username from the bridge, retry for 30 seconds
      if (WiFi.status() == WL_CONNECTED)
      {
        HTTPClient http;
        String url = "http://" + bridgeIP + "/api";
        http.addHeader("Content-Type", "application/json");

        unsigned long startTime = millis();
        bool usernameFound = false;

        while (millis() - startTime < 30000) // Retry for 30 seconds
        {
          http.begin(url);
          int httpResponseCode = http.POST("{\"devicetype\":\"hue#AlarmClock\"}");

          if (httpResponseCode == 200)
          {
            String response = http.getString();
            Serial.println("Response from bridge: " + response);
            Serial.println("Retrying");

            // Allocate a JsonDocument
            StaticJsonDocument<200> doc;

            // Deserialize the JSON document
            DeserializationError error = deserializeJson(doc, response);

            // Check if parsing succeeds
            if (!error)
            {
              // Extract the username
              const char *username = doc[0]["success"]["username"];
              if (username != nullptr && strlen(username) > 0)
              {
                apiUsername = username;
                Serial.print("Username: ");
                Serial.println(username);
                server.send(200, "text/plain", "Bridge added successfully. Username: " + apiUsername);
                usernameFound = true;
                break; // Exit the loop since we found the username
              }
            }
          }

          // Delay before retrying to avoid hammering the API too fast
          delay(100);
        }

        http.end();

        if (!usernameFound)
        {
          server.send(400, "text/plain", "Failed to get username after 30 seconds. Make sure to press the link button on the bridge.");
        }
      }
      else
      {
        server.send(400, "text/plain", "WiFi not connected");
      }
    }
    else
    {
      server.send(400, "text/plain", "Invalid request format");
    }
  }
  else
  {
    server.send(400, "text/plain", "No data received");
  }
}

// Function to send the light state to the Hue Emulator
void setLightState(int lightID, bool turnOn)
{
  if (WiFi.status() == WL_CONNECTED)
  {
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
    if (httpResponseCode > 0)
    {
      String response = http.getString();
      Serial.println("Response: " + response);
    }
    else
    {
      Serial.println("Error Response Code: " + String(httpResponseCode));
    }

    http.end();
  }
  else
  {
    Serial.println("WiFi not connected. Status: " + String(WiFi.status()));
  }
}

// Function to handle scene submission
void handleSubmitScenes()
{
  if (server.hasArg("plain"))
  {
    // Parse the received JSON payload
    String body = server.arg("plain");
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
      Serial.println("Failed to parse JSON");
      server.send(400, "application/json", "{\"status\":\"failed to parse JSON\"}");
      return;
    }

    // Extract wakeScene and sleepScene from the JSON object
    const char *wakeScene = doc["wakeScene"];
    const char *sleepScene = doc["sleepScene"];

    // // Print the scenes to the serial monitor
    // Serial.print("Selected wake-up scene: ");
    // Serial.println(wakeScene);
    // Serial.print("Selected sleep scene: ");
    // Serial.println(sleepScene);

    // Handle scenes based on user's selection
    if (strcmp(wakeScene, "Good morning scene") == 0)
    {
      Serial.println("Turning on Light 1 for Good Morning scene.");
      setLightState(lightID_1, true); // Turn on Light 1
      // setLightState(lightID_2, false); // Ensure Light 2 is off
    }
    if (strcmp(sleepScene, "Good night scene") == 0)
    {
      Serial.println("Turning on Light 2 for Good Night scene.");
      setLightState(lightID_2, true); // Turn on Light 2
      // setLightState(lightID_1, false); // Ensure Light 1 is off
    }
    else if (strcmp(sleepScene, "Night light scene") == 0)
    {
      Serial.println("Turning off both lights.");
      setLightState(lightID_1, false); // Turn off Light 1
      setLightState(lightID_2, false); // Turn off Light 2
    }

    // Respond to the client
    server.send(200, "application/json", "{\"status\":\"scenes received\"}");
  }
  else
  {
    server.send(400, "application/json", "{\"status\":\"no body received\"}");
  }
}

void startWebServer()
{
  server.on("/", HTTP_GET, []()
            {
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

    server.send(200, "text/html", htmlContent); });
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
  server.on("/submitScenes", HTTP_POST, handleSubmitScenes);

  // server.on("/toggleLED", HTTP_GET, toggleLED);

  server.on("/scan", HTTP_GET, []()
            {
    int n = WiFi.scanNetworks();
    String networks = "[";
    for (int i = 0; i < n; ++i) {
      if (i > 0) networks += ",";
      networks += "\"" + WiFi.SSID(i) + "\"";
    }
    networks += "]";
    server.send(200, "application/json", networks); });

  server.on("/wifiStatus", HTTP_GET, []()
            {
    String ssid = WiFi.SSID();
    String ip = WiFi.localIP().toString();
    String json = "{\"ssid\":\"" + ssid + "\", \"ip\":\"" + ip + "\"}";
    server.send(200, "application/json", json); });

  server.on("/setWiFi", HTTP_POST, []()
            {
    String newSSID = server.arg("ssid");
    String newPass = server.arg("pass");

    preferences.putString("ssid", newSSID);
    preferences.putString("password", newPass);

    server.send(200, "text/plain", "Settings saved, ESP will restart now.");
    delay(1000);
    ESP.restart(); });

  server.begin();
  Serial.println("HTTP server started");
}

void startAPMode()
{
  WiFi.softAPConfig(ap_IP, ap_IP, subnet); // Set static IP for AP mode
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point started with IP: ");
  Serial.println(WiFi.softAPIP());

  startWebServer();
}
