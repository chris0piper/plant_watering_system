// plant_watering_system.h
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <EEPROM.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <AsyncJson.h>


// Constants
#define WATERING_HISTORY_SIZE 5
#define OZ_PER_MINUTE (12.0 / 4.0)
#define MILLIS_PER_OZ ((4L * 60L * 1000L) / 12L)

// Structures
struct WateringEvent {
    time_t timestamp;
    float amount;
};

struct Plant {
    char name[32];  // Fixed size array instead of const char*
    float ozPerWatering;     
    int intervalMinutes;     
    WateringEvent wateringHistory[WATERING_HISTORY_SIZE];
    int currentHistoryIndex;  
    bool needsWatering;      
};

struct Pump {
    int in1;
    int in2;
    int number;
    Plant* plant;  
    bool isRunning;
    unsigned long startTime;
    unsigned long runDuration;
};

// Function declarations
void setupWiFi();
bool syncTime();
void checkWateringNeeds();
void waterPlants();
void printPlantSchedules();
void saveWateringTimes();
void loadWateringTimes();
void resetEEPROM();
void resetPlantHistory(int plantIndex);
void pumpOn(Pump& pump);
void pumpOff(Pump& pump);
void setupWebServer();
String getPlantDataJson();

// External variable declarations
extern const char* ssid;
extern const char* password;
extern const char* ntpServer;
extern const char* ntpServer1;
extern const char* ntpServer2;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;
extern Plant plants[];
extern Pump pumps[];
extern const int NUM_PUMPS;
extern const int EEPROM_SIZE;
extern AsyncWebServer server;

