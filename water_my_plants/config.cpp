// config.cpp
#include "config.h"
#include "water_my_plants.h"

// WiFi credentials
const char* ssid = "ConnectoPatronum";
const char* password = "AccioInternet";

// Time server settings
const char* ntpServer = "pool.ntp.org";
const char* ntpServer1 = "time.google.com";
const char* ntpServer2 = "time.cloudflare.com";
const long gmtOffset_sec = -18000;    // EST is UTC-5 * 3600 = -18000
const int daylightOffset_sec = 3600;  // 1 hour DST
const int MINUTES_PER_DAY = 1440;  // 24 hours * 60 minutes

// Plant definitions
Plant plants[] = {
    {{"Prickly Pear"}, 3.0, 14 * MINUTES_PER_DAY, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {{"Rosemary"}, 3.0, 4 * MINUTES_PER_DAY, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {{"Fittonia"}, 1.5, 1 * MINUTES_PER_DAY, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {{"Thyme"}, 3.0, 3 * MINUTES_PER_DAY, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {{"Myrtle"}, 3.0, 2.5 * MINUTES_PER_DAY, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {{"No Plant"}, 0.0, 100 * MINUTES_PER_DAY, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {{"Lavender"}, 3.0, 4 * MINUTES_PER_DAY, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {{"Mint Plant"}, 3.0, 2 * MINUTES_PER_DAY, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false}
};

// Pump definitions
Pump pumps[] = {
    {33, 23, 1, &plants[0], false, 0, 0},
    {21, 22, 2, &plants[1], false, 0, 0},
    {16, 4,  3, &plants[2], false, 0, 0},
    {15, 2,  4, &plants[3], false, 0, 0},
    {14, 27, 5, &plants[4], false, 0, 0},
    {25, 26, 6, &plants[5], false, 0, 0},
    {19, 18, 7, &plants[6], false, 0, 0},
    {17, 5,  8, &plants[7], false, 0, 0}
};

// EEPROM size calculation

const int NUM_PUMPS = sizeof(pumps) / sizeof(pumps[0]);
const int SIZE_PER_PLANT = 
    32 +                   // name[32]
    sizeof(float) +        // ozPerWatering
    sizeof(int) +          // intervalMinutes
    sizeof(int) +          // currentHistoryIndex
    sizeof(bool) +         // needsWatering
    3 +                    // padding to align to 4 bytes
    (WATERING_HISTORY_SIZE * (sizeof(time_t) + sizeof(float))); // wateringHistory

const int EEPROM_SIZE = sizeof(uint32_t) + NUM_PUMPS * SIZE_PER_PLANT;
