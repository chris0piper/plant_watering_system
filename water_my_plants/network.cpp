// network.cpp
#include "water_my_plants.h"

bool syncTime() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, ntpServer1, ntpServer2);
    
    struct tm timeinfo;
    int retries = 0;
    const int maxRetries = 10;
    
    while (!getLocalTime(&timeinfo) && retries < maxRetries) {
        Serial.println("Waiting for time sync...");
        retries++;
        delay(1000);
    }
    
    if (retries < maxRetries) {
        char timeStr[30];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("Time synchronized: %s\n", timeStr);
        return true;
    }
    
    Serial.println("Time sync failed!");
    return false;
}

void setupWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        
        if (syncTime()) {
            struct tm timeinfo;
            if(getLocalTime(&timeinfo)){
                Serial.print("Current time: ");
                Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
            }
        } else {
            Serial.println("Failed to sync time after multiple attempts");
        }
    } else {
        Serial.println("\nWiFi connection failed");
    }
}