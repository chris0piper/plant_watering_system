// plant_watering_system.ino
#include "water_my_plants.h"

void setup() {
    Serial.begin(115200);
    
    // Initialize pump pins and ensure they're OFF
    for (int i = 0; i < NUM_PUMPS; i++) {
        pinMode(pumps[i].in1, OUTPUT);
        pinMode(pumps[i].in2, OUTPUT);
        digitalWrite(pumps[i].in1, LOW);
        digitalWrite(pumps[i].in2, LOW);
        pumpOff(pumps[i]);
    }
    
    delay(1000);  // Give the system time to stabilize
    
    // WiFi setup with more robust connection handling
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    delay(1000);
    
    // Increase WiFi power
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    
    setupWiFi();
    
    if (WiFi.status() != WL_CONNECTED || !syncTime()) {
        Serial.println("Initial setup failed - restarting...");
        delay(1000);
        ESP.restart();
    }
    
    // Initialize EEPROM
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("Failed to initialize EEPROM");
        return;
    }
    
    loadWateringTimes();

    // Set up the web server
    setupWebServer();
    
    Serial.println("Plant Watering System Initialized");
    printPlantSchedules();
}

void loop() {
    static unsigned long lastWiFiCheck = 0;
    static unsigned long lastTimeSync = 0;
    unsigned long currentMillis = millis();
    
    // Check WiFi every 5 minutes
    if (currentMillis - lastWiFiCheck >= 300000) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected - reconnecting...");
            setupWiFi();
        }
        lastWiFiCheck = currentMillis;
    }
    
    // Sync time every hour
    if (currentMillis - lastTimeSync >= 3600000) {
        if (!syncTime()) {
            Serial.println("Time sync failed - resetting...");
            ESP.restart();
        }
        lastTimeSync = currentMillis;
    }
    
    checkWateringNeeds();
    waterPlants();
    delay(1000);
}