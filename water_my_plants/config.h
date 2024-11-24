// config.h
#pragma once
#include "water_my_plants.h"

// Declare all variables as extern
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
extern const int SIZE_PER_PLANT;