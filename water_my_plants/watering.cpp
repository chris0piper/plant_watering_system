// watering.cpp
#include "water_my_plants.h"

void checkWateringNeeds() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time - skipping watering check");
        return;
    }
    
    time_t now = mktime(&timeinfo);
    
    for (int i = 0; i < NUM_PUMPS; i++) {
        Plant* plant = pumps[i].plant;
        if (plant->intervalMinutes == 0) continue;
        
        int lastIndex = (plant->currentHistoryIndex - 1 + WATERING_HISTORY_SIZE) % WATERING_HISTORY_SIZE;
        time_t lastWatered = plant->wateringHistory[lastIndex].timestamp;
        
        if (lastWatered == 0 || (now - lastWatered) >= (plant->intervalMinutes * 60)) {
            plant->needsWatering = true;
            pumps[i].runDuration = (unsigned long)(plant->ozPerWatering * MILLIS_PER_OZ);
        }
    }
}

void waterPlants() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time - skipping watering check");
        return;
    }
    
    time_t now = mktime(&timeinfo);
    unsigned long currentMillis = millis();
    bool needToSave = false;
    
    for (int i = 0; i < NUM_PUMPS; i++) {
        if (!pumps[i].isRunning && pumps[i].plant->needsWatering) {
            pumpOn(pumps[i]);
            pumps[i].isRunning = true;
            pumps[i].startTime = currentMillis;
            
            char timeStr[30];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
            Serial.printf("%s Starting to water %s\n", timeStr, pumps[i].plant->name);
        }
        else if (pumps[i].isRunning) {
            if (currentMillis - pumps[i].startTime >= pumps[i].runDuration) {
                pumpOff(pumps[i]);
                pumps[i].isRunning = false;
                
                int currentIndex = pumps[i].plant->currentHistoryIndex;
                pumps[i].plant->wateringHistory[currentIndex].timestamp = now;
                pumps[i].plant->wateringHistory[currentIndex].amount = pumps[i].plant->ozPerWatering;
                
                pumps[i].plant->currentHistoryIndex = (currentIndex + 1) % WATERING_HISTORY_SIZE;
                pumps[i].plant->needsWatering = false;
                needToSave = true;
                
                if(getLocalTime(&timeinfo)) {
                    char timeStr[30];
                    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
                    Serial.printf("%s Finished watering %s (%.1f oz)\n", 
                                timeStr, pumps[i].plant->name, 
                                pumps[i].plant->ozPerWatering);
                }
            }
        }
    }
    
    if (needToSave) {
        saveWateringTimes();
    }
}

void printPlantSchedules() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    
    char timeStr[30];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    Serial.println("\nCurrent Watering Schedules:");
    Serial.printf("Current time: %s\n", timeStr);
    
    for (int i = 0; i < NUM_PUMPS; i++) {
        if (pumps[i].plant->intervalMinutes > 0) {
            Serial.printf("Pump %d - %s: %.1foz every %d minutes\n",
                pumps[i].number,
                pumps[i].plant->name,
                pumps[i].plant->ozPerWatering,
                pumps[i].plant->intervalMinutes);
        }
    }
}

void pumpOn(Pump& pump) {
    digitalWrite(pump.in1, HIGH);
    digitalWrite(pump.in2, LOW);
}

void pumpOff(Pump& pump) {
    digitalWrite(pump.in1, LOW);
    digitalWrite(pump.in2, LOW);
}