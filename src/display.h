#ifndef DISPLAY_H
#define DISPLAY_H

#include "DFRobotDFPlayerMini.h"
#include <RTClib.h>
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <EEPROM.h>
#include <stdlib.h>
#include "localServer.h"

extern int AlarmHH , AlarmMM, AlarmSS;

extern bool changeSceneBtn;
extern bool changeSceneBtnENC;

// Function prototypes
void setupPeripheral();
void updateEncoder();
void displayClock();
// void printDetail(uint8_t type, int value);
bool isValidTime(int hh, int mm);
void readEEPROM();
void writeEEPROM();
void eraseEEPROM();
void setAlarmFromMessage(String message);
void handleSerialInput();
void setTimeNow(int delta);
void changeSettingValue(int delta);
void handleRightButton();
void handleLeftButton();
void alarmSnooze();
void alarmStop();
void alarmPlay();
void handleAlarmButton();
void handleButtonPresses();
void toggleAlarm();
int GetStringWidth(const char *text, int font);
void setTimeFromValues(int year, int month, int day, int hour, int minute, int second);
void setAlarmTime(int hour, int minute, int second, bool isPM);
void setDFPlayerVolume(int volume);
void playSound(int track);

#endif
