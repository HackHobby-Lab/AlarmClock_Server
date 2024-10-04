#include "display.h"

DFRobotDFPlayerMini myDFPlayer;
RTC_DS3231 rtc;

UBYTE *BlackImage;
#define FPSerial Serial1
#define EEPROM_SIZE 512

const char *mqtt_server = "broker.mqtt-dashboard.com";
const char *alarmOut = "outTopicc";
const char *soundSet = "soundSetting";

int brightness = 0;
const int encoderPinA = 32;
const int encoderPinB = 35;
const int encoderButtonPin = 33;
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
volatile long lastEncoderValue = 0;

int lastButtonState = LOW;
int currentTune = 0;

char alarmSet[20];

int timeHH = 12, timeMM = 0, timeSS = 0;
int timeYear, timeMonth, timeDay;
int AlarmHH = 12, AlarmMM = 0, AlarmSS = 0;
bool isPM = false;
int mode = 0;
bool alarmTriggered = false;

int lastMinute = -1;
int lastHour = -1;
int lastDay = -1;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

unsigned long previousMillis = 0;
const long interval = 60000;

const int alarmButtonPin = 4;
const int leftButtonPin = 12;
const int rightButtonPin = 34;

int settingMode = 0;
int settingValue = 0;
int timeSetting = 0;
unsigned long lastButtonPress = 0;
const unsigned long buttonHoldTime = 800;
long snoozeTime = 5 * 1000;
bool alarmStopFlag = false;
bool alarmSnoozeFlag = false;
bool setTime = false;
bool setTuneFlag = false;
bool wifiConnected = false;

bool changeSceneBtn = false;
bool changeSceneBtnENC = false;

unsigned long buttonHeldTime = 0;
const unsigned long holdDuration = 2000; // 2 seconds hold time
bool buttonPressed = false;

bool encoderPressed = false;
unsigned long encoderHeldTime = 0;
int lastButtonEncoder = LOW;

 bool sceneTriggered = false;  

void setupPeripheral()
{
  EEPROM.begin(EEPROM_SIZE);

  FPSerial.begin(9600, SERIAL_8N1, /*rx =*/17, /*tx =*/18);

  // Set button pins as inputs with internal pull-up resistors
  pinMode(alarmButtonPin, INPUT);
  pinMode(leftButtonPin, INPUT);
  pinMode(rightButtonPin, INPUT);
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  pinMode(encoderButtonPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(encoderPinA), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPinB), updateEncoder, CHANGE);

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (!myDFPlayer.begin(FPSerial, /*isACK = */ true, /*doReset = */ true))
  { // Use serial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    // while (true);
    delay(1000);
  }

  delay(2000); // Pause for 2 seconds
  rtc.begin();
  DEV_Module_Init();

  Serial.println("e-Paper Init and Clear...\r\n");
  EPD_5IN83_V2_Init();
  EPD_5IN83_V2_Clear();
  DEV_Delay_ms(500);
  UWORD Imagesize = ((EPD_5IN83_V2_WIDTH % 8 == 0) ? (EPD_5IN83_V2_WIDTH / 8) : (EPD_5IN83_V2_WIDTH / 8 + 1)) * EPD_5IN83_V2_HEIGHT;
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
  {
    printf("Failed to apply for black memory...\r\n");
    // while (1)
    //   ;
    delay(1000);
  }
  printf("Paint_NewImage\r\n");
  Paint_NewImage(BlackImage, EPD_5IN83_V2_WIDTH, EPD_5IN83_V2_HEIGHT, 180, WHITE);
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);

  // Check if RTC is connected
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    // while (1)
    //   ;
    delay(1000);
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, let's set the time!");
    // Comment out the next line after setting the date & time initially
    // This prevents the RTC from being reset
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("RTC Initialized.");
  Serial.println("Commands:");
  Serial.println("  set alarm - Enter alarm setting mode");
  Serial.println("  toggle alarm - Toggle alarm on/off");
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.setTimeOut(500); // Set serial communictaion time out 500ms

  //----Set volume----
  myDFPlayer.volume(20); // Set initial volume value (0~30).

  //----Set different EQ----
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);

  //----Set device we use SD as default----
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);

  //----Mp3 play----
  myDFPlayer.play(currentTune); // Play the first mp3

  Serial.println(myDFPlayer.readState());               // read mp3 state
  Serial.println(myDFPlayer.readVolume());              // read current volume
  Serial.println(myDFPlayer.readEQ());                  // read EQ setting
  Serial.println(myDFPlayer.readFileCounts());          // read all file counts in SD card
  Serial.println(myDFPlayer.readCurrentFileNumber());   // read current play file number
  Serial.println(myDFPlayer.readFileCountsInFolder(3)); // read file counts in folder SD:/03

  // Paint_DrawImage(Icons60_Table, 50, 50, 60,60);
  readEEPROM();
}

void displayClock()
{

  DateTime now = rtc.now();
  int alarmHour24 = AlarmHH % 12; // Get the hour in 12-hour format (0-11)
  if (isPM)
  {
    alarmHour24 += 12; // Convert PM hours to 24-hour format
  }
  else if (alarmHour24 == 0)
  {
    alarmHour24 = 12; // Handle midnight case for 12-hour clock
  }

  DateTime alarm(alarmHour24, AlarmMM, AlarmSS); // Corrected variable name
  // Serial.println("main");
  if (encoderValue != lastEncoderValue)
  {
    if (encoderValue > lastEncoderValue)
    {
      turnOnLED();
      if (settingMode > 0)
      {
        changeSettingValue(-1);
      }
      if (setTuneFlag)
      {
        myDFPlayer.volumeUp();
      }
      if (timeSetting > 0)
      {
        setTimeNow(-1);
      }
    }
    else
    {
       turnOffLED();
      if (settingMode > 0)
      {
        changeSettingValue(1);
      }
      if (setTuneFlag)
      {
        myDFPlayer.volumeDown();
      }
      if (timeSetting > 0)
      {
        setTimeNow(1);
      }
    }
  }
  lastEncoderValue = encoderValue;
 
  unsigned long currentMillis = millis();

  if (digitalRead(encoderButtonPin) == LOW) {
    if (!encoderPressed) {
      Serial.println("Encoder Button Pressed");
      encoderPressed = true;
      lastButtonEncoder = currentMillis;  // Record the time when the button is first pressed
    }

    // Check if customButtonOne is true and the button has been held long enough
    if (customButtonOne == true && currentMillis - lastButtonEncoder >= holdDuration) {
      Serial.println("Changing scene on button Encoder");
      changeSceneBtnENC = true;
      customBtnScene();  // Call your custom function
    }

  } else {
    // Button is released, reset variables
    if (encoderPressed) {
      Serial.println("Encoder Button Released");
    }
    encoderPressed = false;
    changeSceneBtnENC = false;
    lastButtonEncoder = currentMillis;  // Reset the lastButtonEncoder time
  }

  
  if (setTuneFlag)
  {
    int buttonState = digitalRead(encoderButtonPin);
    if (buttonState == LOW && lastButtonState == HIGH)
    {
      delay(50); // Debounce delay
      if (digitalRead(encoderButtonPin) == LOW)
      {
        currentTune++;
        // myDFPlayer.play(currentTune);
        Serial.print(F("Playing tune: "));
        Serial.println(currentTune);
        // myDFPlayer.start();
        // myDFPlayer.loop(currentTune);

        if (currentTune > 7)
        {
          currentTune = 0;
        }
      }

      
    }

    lastButtonState = buttonState;
  }

  // Get current time from RTC

  bool nowPM = now.hour() >= 12;
  int nowHour12 = (now.hour() % 12);
  if (nowHour12 == 0)
    nowHour12 = 12; // Adjust for 12-hour format
if(enabledAlarm == true){
  // Check if it's time to trigger the alarm
  if (now.hour() == AlarmHH && now.minute() == AlarmMM)
  {
    alarmTriggered = true;              // Set alarm trigger flag
    Serial.println("Alarm triggered!"); //    tone(buzzerPin, 2000); // Play a tone on the buzzer


  }
  else{
    alarmTriggered = false; 
  }

   

    if (now.hour() == AlarmHH && now.minute() == AlarmMM - fadeInBefore.toInt()) {
      if (!sceneTriggered && atAlarmTrigger == true && atAlarmStop == false) {
        Serial.println("Sending Scene Trigger");
        alarmTriggerScene();
        sceneTriggered = true;  // Set the flag to true after the scene is triggered
      }
    } else {
      // Reset the flag once the time moves past the alarm time
      sceneTriggered = false;
    }


      // Display current time in 12-hour format or alarm triggered message
    if (alarmTriggered == true)
    {
      Serial.println("Alarm triggered!");

      alarmPlay();

      // toggleAlarm();
    }
    else{
      alarmStop();
    }
}
  // } else {
  //   Serial.print(now.year(), DEC);
  //   Serial.print('/');
  //   Serial.print(now.month(), DEC);
  //   Serial.print('/');
  //   Serial.print(now.day(), DEC);
  //   Serial.print(" ");
  //   Serial.print(nowHour12);
  //   Serial.print(':');
  //   Serial.print(now.minute(), DEC);
  //   Serial.print(':');
  //   Serial.print(now.second(), DEC);
  //   Serial.print(nowPM ? " PM" : " AM");
  //   Serial.println();
  //   if (nowPM) {
  //     Serial.println("PM");
  //   } else {
  //     Serial.println("AM");
  //   }
  // }
  // Calculate the difference
  if (now < alarm)
  {
    TimeSpan timeLeft = alarm - now;
    Serial.print("Time until alarm: ");
    Serial.print(timeLeft.hours());
    Serial.print(" hours, ");
    Serial.print(timeLeft.minutes());
    Serial.print(" minutes, ");
    Serial.print(timeLeft.seconds());
    Serial.println(" seconds");
  }
  else if (now > alarm)
  {
    // Serial.println("Alarm time has passed or is now.");
  }
  else
  {
    Serial.println("Alarm time is now.");
  }

  if (alarmSnoozeFlag == true && alarmStopFlag != true)
  {
    Serial.println("In main loop alarm snooze");
    unsigned long currentTime = millis();
    if (((currentTime - previousMillis) > snoozeTime) && (alarmStopFlag != true))
    {
      alarmPlay();
      alarmTriggered = true;
      Serial.println("Aalarm goes off again after Snooze--------------");
    }
    previousMillis = currentTime;
  }

  if (now.day() != lastDay)
  {
    lastDay = now.day();
    Serial.print("------------------------------------------------");
    Serial.println(now.day());
    Serial.print("------------------------------------------------");
    // Paint_Clear(BLACK);
    // Paint_Clear(WHITE);
    EPD_5IN83_V2_Clear();
    Serial.println("Clearing Screen----------------------------------------++++++++++++++++++++++++++++++");
    char weekDayString[20];
    snprintf(weekDayString, sizeof(weekDayString), "%01s", daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.println("String for Week Day prepared");
    int stringWidth = GetStringWidth(weekDayString, 46); // Get the width of the string
    int centerX = EPD_5IN83B_V2_WIDTH / 2;               // Center of the screen X-axis
    int startX = centerX - (stringWidth / 2);            // Calculate the starting X position to center the text
    Paint_ClearWindows(0, 10, 648, 80, BLACK);
    Paint_ClearWindows(0, 10, 648, 80, WHITE);
    Paint_DrawString_EN(
        startX,
        10,
        weekDayString,
        &Font60,
        BLACK,
        WHITE);
    char dateDayString[20];
    snprintf(dateDayString, sizeof(dateDayString), "%01d/%01d/%01d",
             // daysOfTheWeek[now.dayOfTheWeek()],
             now.month(),
             now.day(),
             now.year());
    Serial.println("String for date Day is Ready");
    Paint_ClearWindows(0, 85, 648, 162, BLACK);
    Paint_ClearWindows(0, 85, 648, 162, WHITE);
    Paint_DrawRectangle(0, 90, 648, 160, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    int stringWidth2 = GetStringWidth(dateDayString, 36); // Get the width of the string
    int centerX2 = EPD_5IN83B_V2_WIDTH / 2;               // Center of the screen X-axis
    int startX2 = centerX2 - (stringWidth2 / 2);          // Calculate the starting X position to center the text
    Paint_DrawString_EN(
        startX2, // X position (centered horizontally)
        100,     // Y position (centered vertically)
        dateDayString,
        &Font50, // Font used
        WHITE,   // Text color
        BLACK    // Background color
    );
    EPD_5IN83_V2_Display(BlackImage);
  }
  if (now.hour() != lastHour)
  {
    lastHour = now.hour();
    Serial.print("------------------------------------------------");
    Serial.println(now.hour());
    Serial.print("------------------------------------------------");
    EPD_5IN83_V2_Clear();
    DEV_Delay_ms(500);
  }
  if ((now.minute() != lastMinute) || setTime == true)
  {
    lastMinute = now.minute();
    Serial.print("------------------------------------------------");
    Serial.println(now.minute());
    Serial.print("------------------------------------------------");

    // EPD_5IN83B_V2_Clear();
    char timeString[20];
    snprintf(timeString, sizeof(timeString), "%02d:%02d%01s",
             now.hour() % 12 == 0 ? 12 : now.hour() % 12,
             now.minute(),
             now.hour() >= 12 ? "PM" : "AM");
    Serial.println("String prepared");
    int stringWidth = GetStringWidth(timeString, 86); // Get the width of the string
    int centerX = EPD_5IN83B_V2_WIDTH / 2;            // Center of the screen X-axis
    int startX = centerX - (stringWidth / 2);         // Calculate the starting X position to center the text
    Paint_ClearWindows(0, 190, 648, 290, BLACK);
    Paint_ClearWindows(0, 190, 648, 290, WHITE);
    Paint_DrawString_EN(
        startX,
        190,
        timeString,
        &Font100,
        BLACK,
        WHITE);
    EPD_5IN83_V2_Display(BlackImage);
    setTime = false;
  }

  // Check for button presses
  handleButtonPresses();

  // Check for serial input
  handleSerialInput();
  // delay(100);
  //  printf("Goto Sleep...\r\n");
  //  EPD_5IN83_V2_Sleep();
  //  free(BlackImage);
  //  BlackImage = NULL;
  if (myDFPlayer.available())
  {
    // printDetail(myDFPlayer.readType(), myDFPlayer.read());  //Print the detail message from DFPlayer to handle different errors and states.
  }

  delay(100); // Delay before updating the display again
}

int GetStringWidth(const char *text, int font)
{
  // Implement this function based on your library
  // For example purposes, it's a stub here
  return strlen(text) * font; // Example calculation
}

void toggleAlarm()
{
  if (alarmTriggered || alarmEnabled)
  {
    alarmEnabled = false;
    alarmTriggered = false;
    Serial.println("Alarm deactivated.");
  }
  else
  {
    mode = 1 - mode;
    if (mode == 1)
    {
      Serial.println("Alarm activated.");
    }
    else
    {
      Serial.println("Alarm deactivated.");
    }
  }
}


void handleButtonPresses()
{
  // Check for alarm button press
  if (digitalRead(alarmButtonPin) == HIGH)
  {
    Serial.println("Alarm Button pressed");
    handleAlarmButton();
    delay(200); // Debounce delay
  }

  // Check for left button press
  if (digitalRead(leftButtonPin) == HIGH)
  {
    Serial.println("left Button pressed");

    handleLeftButton();
    delay(200); // Debounce delay
  }

  unsigned long currentTime = millis();
if (digitalRead(rightButtonPin) == HIGH) {
  if (!buttonPressed) {
    Serial.println("right Button pressed");
    buttonPressed = true;
    lastButtonPress = currentTime;  // Record the time when the button is pressed
  }

  // Check if customButtonTwo is true and the button has been held long enough
  if (customButtonTwo == true && currentTime - lastButtonPress >= holdDuration) {
    changeSceneBtn = true;
    customBtnScene2(); // Call your custom function
    Serial.println("Changing scene on button right");
  }

} else {
  // Button is released, reset variables
  if (buttonPressed) {
    Serial.println("right Button released");
  }
  buttonPressed = false;
  changeSceneBtn = false;
  lastButtonPress = currentTime;  // Reset the lastButtonPress time
}

// Optional debounce delay
delay(200);

}

void handleAlarmButton()
{
  // technique used to switch between two states or values
  // mode is 0 initially
  unsigned long currentTime = millis();
  if ((currentTime - lastButtonPress) >= buttonHoldTime)
  {

    // Button pressed, move to previous setting
    settingMode++;
    if (settingMode == 0)
    {
      Serial.println("Exiting alarm setting mode.");
    }
  }
  if (settingMode == 8)
  {
    // Serial.println("Alarm set.");
    settingMode = 0;
  }

  lastButtonPress = currentTime;

  if (alarmTriggered == true)
  {
    if ((currentTime - lastButtonPress) >= 800)
    {

      Serial.println("Alarm deactivated.");
      // noTone(buzzerPin); // Stop the buzzer tone
      alarmTriggered = false; // Reset the alarm trigger flag
      alarmStop();
      //eraseEEPROM();
    }
    lastButtonPress = currentTime;
  }
}

void alarmPlay()
{
  myDFPlayer.play(currentTune);
  Serial.print(F("Playing tune: "));
  Serial.println(currentTune);
  myDFPlayer.start();
  myDFPlayer.loop(currentTune);
  // Paint_Clear(WHITE);
  //  Paint_Clear(BLACK);
  // Serial.println("Clearing Screen----------------------------------------++++++++++++++++++++++++++++++");
  // Paint_DrawString_EN(
  //     100,
  //     300,
  //     "Wake Up",
  //     &Font80,
  //     WHITE,
  //     BLACK);
  // EPD_5IN83_V2_Display(BlackImage);
}

void alarmStop()
{
  if (atAlarmTrigger == false && atAlarmStop == true)
  {
    alarmTriggerScene();
  };
  alarmStopFlag = true;
  alarmSnoozeFlag = false;
  myDFPlayer.stop();
  // Paint_ClearWindows(100, 300, 648, 430, BLACK);
  // Paint_ClearWindows(100, 300, 648, 430, WHITE);
  // EPD_5IN83_V2_Display(BlackImage);
}
void alarmSnooze()
{
  alarmSnoozeFlag = true;
  Serial.println("Aalarm Snoozed");
  myDFPlayer.stop();
  Paint_ClearWindows(100, 300, 648, 430, BLACK);
  Paint_ClearWindows(100, 300, 648, 430, WHITE);
  EPD_5IN83_V2_Display(BlackImage);
}
void handleLeftButton()
{

  unsigned long currentTime = millis();
  if (currentTime - lastButtonPress >= buttonHoldTime)
  {
    timeSetting++;
    if (timeSetting == 0)
    {
      Serial.println("Exiting alarm setting mode.");
    }
    // setTime = true;
  }
  if (timeSetting > 9)
  {
    // Serial.println("Alarm set.");
    timeSetting = 0;
    timeSetting = false;
  }
  lastButtonPress = currentTime;

  if (alarmTriggered == true)
  {
    Serial.println("It's first if in alarm snooze");
    unsigned long currentTime = millis();
    if (currentTime - lastButtonPress >= buttonHoldTime)
    {
      alarmSnooze();
    }
    lastButtonPress = currentTime;
  }
}

void handleRightButton()
{

  if (alarmTriggered == true)
  {
    Serial.println("It's first if in alarm stop");
    unsigned long currentTime = millis();
    if (currentTime - lastButtonPress >= buttonHoldTime)
    {
      alarmStop();
      // eraseEEPROM();
      alarmTriggered == false;
    }
    lastButtonPress = currentTime;
  }
}
// to handle the increment/decrement of the setting value based on the current setting mode.
void changeSettingValue(int delta)
{
  switch (settingMode)
  {
    //Serial.println(settingMode);
  case 1: // Hour
    settingValue = (settingValue + 12 + delta) % 12;
    if (settingValue == 0)
      settingValue = 12;
    AlarmHH = settingValue;
    Serial.print("Hour: ");
    Serial.println(settingValue);

    break;
  case 2: // Minute
    settingValue = (settingValue + 60 + delta) % 60;
    AlarmMM = settingValue;
    Serial.print("Minute: ");
    Serial.println(settingValue);

    break;
  case 3: // Second
    isPM = !isPM;
    Serial.print("AM/PM: ");
    Serial.println(isPM ? "PM" : "AM");

    break;
  case 4:
    setTuneFlag = true;
    Serial.println("Set Tune");
    break;
  case 5: // AM/PM

    Serial.print("Alarm set to ");
    Serial.print(AlarmHH);
    Serial.print(":");
    Serial.print(AlarmMM);
    Serial.print(":");
    Serial.print(AlarmSS);
    Serial.println(isPM ? " PM" : " AM");

    writeEEPROM();

    // settingMode = 0;
    break;
  case 6:

    break;
  case 7:
    settingMode = 0;
    break;
  }

  // if (alarmTriggered == true){
  //     alarmStop();
  //     alarmTriggered == false;
  // }
}
void setTimeNow(int delta)
{
  int adjustedHH = timeHH;
  switch (timeSetting)
  {
  case 1: // Hour
    settingValue = (settingValue + 12 + delta) % 12;
    if (settingValue == 0)
      settingValue = 12;
    timeHH = settingValue;
    Serial.print("Hour: ");
    Serial.println(settingValue);
    break;
  case 2: // Minute
    settingValue = (settingValue + 60 + delta) % 60;
    timeMM = settingValue;
    Serial.print("Minute: ");
    Serial.println(settingValue);
    break;
  case 3: // AM/PM
    isPM = !isPM;
    Serial.print("AM/PM: ");
    Serial.println(isPM ? "PM" : "AM");
    break;
  case 4: // Finalize time setting
    if (isPM && adjustedHH != 12)
      adjustedHH += 12;
    if (!isPM && adjustedHH == 12)
      adjustedHH = 0;
    // rtc.adjust(DateTime(2024, 7, 29, adjustedHH, timeMM, timeSS)); // Set time to YYYY, MM, DD, HH, MM, SS
    Serial.print("Time ");
    Serial.print(timeHH);
    Serial.print(":");
    Serial.print(timeMM);
    Serial.print(":");
    Serial.print(timeSS);
    Serial.println(isPM ? " PM" : " AM");
    break;
  case 5: // Year
    settingValue = (settingValue + 10000 + delta) % 10000;
    timeYear = settingValue;
    Serial.print("Year: ");
    Serial.println(settingValue);
    break;
  case 6: // Month
    settingValue = (settingValue + 12 + delta) % 12;
    if (settingValue == 0)
    {
      settingValue = 12;
    }
    timeMonth = settingValue;
    Serial.print("Month: ");
    Serial.println(settingValue);
    break;
  case 7: // Day
    settingValue = (settingValue + 31 + delta) % 31;
    if (settingValue == 0)
    {
      settingValue = 31;
    }
    timeDay = settingValue;
    Serial.print("Day: ");
    Serial.println(settingValue);
    break;
  case 8: // Finalize date and time setting
    if (isPM && adjustedHH != 12)
      adjustedHH += 12;
    if (!isPM && adjustedHH == 12)
      adjustedHH = 0;
    rtc.adjust(DateTime(timeYear, timeMonth, timeDay, adjustedHH, timeMM, timeSS)); // Set date and time to YYYY, MM, DD, HH, MM, SS
    Serial.print("Date & Time ");
    Serial.print(timeYear);
    Serial.print("-");
    Serial.print(timeMonth);
    Serial.print("-");
    Serial.print(timeDay);
    Serial.print(" ");
    Serial.print(timeHH);
    Serial.print(":");
    Serial.print(timeMM);
    Serial.print(":");
    Serial.print(timeSS);
    Serial.println(isPM ? " PM" : " AM");
    break;
  case 9: // Exit setting mode
    setTime = true;
    break;
  }
}

void handleSerialInput()
{
  if (Serial.available())
  {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.startsWith("set alarm"))
    {
      int hh, mm, ss;
      char period[3];
      sscanf(input.c_str(), "set alarm %d %d  %s", &hh, &mm, period);
      bool pm = (strcmp(period, "PM") == 0 || strcmp(period, "pm") == 0);
      if (isValidTime(hh, mm) && (pm || strcmp(period, "AM") == 0 || strcmp(period, "am") == 0))
      {
        AlarmHH = hh;
        AlarmMM = mm;
        AlarmSS = 0;
        isPM = pm;
        Serial.print("Alarm set to ");
        Serial.print(AlarmHH);
        Serial.print(":");
        Serial.print(AlarmMM);
        Serial.print(":");
        Serial.print(AlarmSS);
        Serial.println(isPM ? " PM" : " AM");
      }
      else
      {
        Serial.println("Invalid time. Please use HH MM AM/PM format.");
      }
    }
    else if (input.equals("toggle alarm"))
    {
      if (alarmTriggered)
      {
        alarmTriggered = false; // Reset the alarm trigger flag
        Serial.println("Alarm deactivated.");
      }
      else
      {
        mode = 1 - mode; // Toggle alarm mode
        if (mode == 1)
        {
          Serial.println("Alarm activated.");
        }
        else
        {
          Serial.println("Alarm deactivated.");
        }
      }
    }
    else
    {
      Serial.println("Unknown command.");
    }
  }
}

void setAlarmFromMessage(String message)
{
  int hh, mm, tune;
  char period[3];
  if (sscanf(message.c_str(), "Set Alarm %d:%d %s tune is %d ", &hh, &mm, period, &tune) == 4)
  {
    bool pm = (strcmp(period, "PM") == 0 || strcmp(period, "pm") == 0);
    if (isValidTime(hh, mm) && (pm || strcmp(period, "AM") == 0 || strcmp(period, "am") == 0))
    {
      AlarmHH = hh;
      AlarmMM = mm;
      AlarmSS = 10;
      isPM = pm;
      Serial.print("Alarm set to ");
      Serial.print(AlarmHH);
      Serial.print(":");
      Serial.print(AlarmMM);
      Serial.print(":");
      Serial.print(AlarmSS);
      Serial.println(isPM ? " PM" : " AM");
      currentTune = tune;
      Serial.print("Current Tune set by MQTT is :");
      Serial.println(tune);
    }
    else
    {
      Serial.println("Invalid time. Please use HH:MM AM/PM format.");
    }
  }
  else
  {
    Serial.println("Invalid command format. Use 'Set Alarm HH:MM AM/PM'.");
  }
  if (sscanf(message.c_str(), "Set sound %d", &tune) == 2)
  {
    currentTune = tune;
    Serial.print("Current Tune set by MQTT is :");
    Serial.println(tune);
  }
}

void eraseEEPROM()
{
  for (int i = 0; i < 512; i++)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  delay(500);
}

void writeEEPROM()
{
  EEPROM.write(0, AlarmHH);
  EEPROM.write(1, AlarmMM);
  EEPROM.write(2, AlarmSS);
  EEPROM.write(3, isPM);
  EEPROM.commit();
  Serial.println("Commiting the eeprom");
}

void readEEPROM()
{
  AlarmHH = EEPROM.read(0);
  AlarmMM = EEPROM.read(1);
  AlarmSS = EEPROM.read(2);
  isPM = EEPROM.read(3);
  Serial.println("Reading from eeprom");
  Serial.print("Alarm set to ");
  Serial.print(AlarmHH);
  Serial.print(":");
  Serial.print(AlarmMM);
  Serial.print(":");
  Serial.print(AlarmSS);
  Serial.println(isPM ? " PM" : " AM");
}

bool isValidTime(int hh, int mm)
{
  return (hh >= 1 && hh <= 12) && (mm >= 0 && mm < 60);
}

void updateEncoder()
{
  int MSB = digitalRead(encoderPinA); // MSB = most significant bit
  int LSB = digitalRead(encoderPinB); // LSB = least significant bit

  int encoded = (MSB << 1) | LSB;         // converting the 2 pin value to single number
  int sum = (lastEncoded << 2) | encoded; // adding it to the previous encoded value

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderValue++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderValue--;

  lastEncoded = encoded; // store this value for next time
}

// void printDetail(uint8_t type, int value) {
//   switch (type) {
//     case TimeOut:
//       Serial.println(F("Time Out!"));
//       break;
//     case WrongStack:
//       Serial.println(F("Stack Wrong!"));
//       break;
//     case DFPlayerCardInserted:
//       Serial.println(F("Card Inserted!"));
//       break;
//     case DFPlayerCardRemoved:
//       Serial.println(F("Card Removed!"));
//       break;
//     case DFPlayerCardOnline:
//       Serial.println(F("Card Online!"));
//       break;
//     case DFPlayerUSBInserted:
//       Serial.println("USB Inserted!");
//       break;
//     case DFPlayerUSBRemoved:
//       Serial.println("USB Removed!");
//       break;
//     case DFPlayerPlayFinished:
//       Serial.print(F("Number:"));
//       Serial.print(value);
//       Serial.println(F(" Play Finished!"));
//       break;
//     case DFPlayerError:
//       Serial.print(F("DFPlayerError:"));
//       switch (value) {
//         case Busy:
//           Serial.println(F("Card not found"));
//           break;
//         case Sleeping:
//           Serial.println(F("Sleeping"));
//           break;
//         case SerialWrongStack:
//           Serial.println(F("Get Wrong Stack"));
//           break;
//         case CheckSumNotMatch:
//           Serial.println(F("Check Sum Not Match"));
//           break;
//         case FileIndexOut:
//           Serial.println(F("File Index Out of Bound"));
//           break;
//         case FileMismatch:
//           Serial.println(F("Cannot Find File"));
//           break;
//         case Advertise:
//           Serial.println(F("In Advertise"));
//           break;
//         default:
//           break;
//       }
//       break;
//     default:
//       break;
//   }
// }

void setTimeFromValues(int year, int month, int day, int hour, int minute, int second)
{
  // Set the RTC to the new date and time
  rtc.adjust(DateTime(year, month, day, hour, minute, second));

  // Print the new date and time for verification
  Serial.print("Time set to: ");
  Serial.print(year);
  Serial.print("-");
  Serial.print(month);
  Serial.print("-");
  Serial.print(day);
  Serial.print(" ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);
}

void setAlarmTime(int hour, int minute, int second, bool alarmIsPM)
{
  // Set the alarm time variables
  AlarmHH = hour;
  AlarmMM = minute;
  AlarmSS = second;
  isPM = alarmIsPM;

  // Log the alarm time for debugging
  Serial.print("Alarm set to: ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.print(second);
  Serial.println(alarmIsPM ? " PM" : " AM");
  writeEEPROM();
}

void setDFPlayerVolume(int volume)
{
  if (volume >= 0 && volume <= 100)
  {
    myDFPlayer.volume(volume);
    Serial.println("Volume Setting Done");
  }
}

void playSound(int track)
{
  Serial.println("Playing Track");
  myDFPlayer.play(track); // Play track based on the passed number

  myDFPlayer.start();
  // myDFPlayer.loop(track);
  Serial.println("Playing Track");
}
