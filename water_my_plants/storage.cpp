// storage.cpp
#include "water_my_plants.h"

void saveWateringTimes() {
    int addr = 0;
    for (int i = 0; i < NUM_PUMPS; i++) {
        EEPROM.put(addr, plants[i].currentHistoryIndex);
        addr += sizeof(int);

        for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
            EEPROM.put(addr, plants[i].wateringHistory[j].timestamp);
            addr += sizeof(time_t);
            EEPROM.put(addr, plants[i].wateringHistory[j].amount);
            addr += sizeof(float);
        }
    }
    EEPROM.commit();
}

void loadWateringTimes() {
    int addr = 0;
    time_t currentTime;
    time(&currentTime);

    for (int i = 0; i < NUM_PUMPS; i++) {
        int loadedIndex;
        EEPROM.get(addr, loadedIndex);
        addr += sizeof(int);

        if (loadedIndex < 0 || loadedIndex >= WATERING_HISTORY_SIZE) {
            loadedIndex = 0;
        }
        plants[i].currentHistoryIndex = loadedIndex;

        bool validHistory = true;
        for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
            time_t timestamp;
            EEPROM.get(addr, timestamp);
            addr += sizeof(time_t);
            float amount;
            EEPROM.get(addr, amount);
            addr += sizeof(float);

            if (timestamp > currentTime || timestamp < 0 || amount < 0 || amount > 100) {
                validHistory = false;
                break;
            }

            plants[i].wateringHistory[j].timestamp = timestamp;
            plants[i].wateringHistory[j].amount = amount;
        }

        if (!validHistory) {
            plants[i].currentHistoryIndex = 0;
            for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
                plants[i].wateringHistory[j].timestamp = 0;
                plants[i].wateringHistory[j].amount = 0;
            }
        }
    }
}

void resetEEPROM() {
    int addr = 0;
    
    for (int i = 0; i < NUM_PUMPS; i++) {
        EEPROM.put(addr, 0);
        addr += sizeof(int);
        
        for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
            time_t zeroTime = 0;
            EEPROM.put(addr, zeroTime);
            addr += sizeof(time_t);
            
            float zeroAmount = 0.0;
            EEPROM.put(addr, zeroAmount);
            addr += sizeof(float);
        }
    }
    
    if (EEPROM.commit()) {
        Serial.println("EEPROM successfully reset");
    } else {
        Serial.println("EEPROM reset failed");
    }
}

void resetPlantHistory(int plantIndex) {
    if (plantIndex < 0 || plantIndex >= NUM_PUMPS) {
        Serial.println("Invalid plant index");
        return;
    }

    int addr = plantIndex * (sizeof(int) + WATERING_HISTORY_SIZE * (sizeof(time_t) + sizeof(float)));
    
    EEPROM.put(addr, 0);
    addr += sizeof(int);
    
    for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
        time_t zeroTime = 0;
        EEPROM.put(addr, zeroTime);
        addr += sizeof(time_t);
        
        float zeroAmount = 0.0;
        EEPROM.put(addr, zeroAmount);
        addr += sizeof(float);
    }
    
    plants[plantIndex].currentHistoryIndex = 0;
    for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
        plants[plantIndex].wateringHistory[j].timestamp = 0;
        plants[plantIndex].wateringHistory[j].amount = 0;
    }
    
    if (EEPROM.commit()) {
        Serial.printf("Successfully reset watering history for %s\n", plants[plantIndex].name);
    } else {
        Serial.printf("Failed to reset watering history for %s\n", plants[plantIndex].name);
    }
}