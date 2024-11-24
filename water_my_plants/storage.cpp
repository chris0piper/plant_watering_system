// storage.cpp
#include "water_my_plants.h"

// Define a magic number to verify EEPROM data validity
#define EEPROM_MAGIC_NUMBER 0xABCD1234

void saveWateringTimes() {
    int addr = 0;

    // Write magic number
    uint32_t magicNumber = EEPROM_MAGIC_NUMBER;
    EEPROM.put(addr, magicNumber);
    addr += sizeof(uint32_t);

    for (int i = 0; i < NUM_PUMPS; i++) {
        // Save name[32]
        EEPROM.put(addr, plants[i].name);
        addr += 32;

        // Save ozPerWatering
        EEPROM.put(addr, plants[i].ozPerWatering);
        addr += sizeof(float);

        // Save intervalMinutes
        EEPROM.put(addr, plants[i].intervalMinutes);
        addr += sizeof(int);

        // Save currentHistoryIndex
        EEPROM.put(addr, plants[i].currentHistoryIndex);
        addr += sizeof(int);

        // Save needsWatering
        EEPROM.put(addr, plants[i].needsWatering);
        addr += sizeof(bool);

        // Padding to align to 4 bytes
        addr += 3;  // Skip 3 bytes for alignment

        // Save wateringHistory
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

    // Read magic number to verify data validity
    uint32_t magicNumber;
    EEPROM.get(addr, magicNumber);
    addr += sizeof(uint32_t);

    if (magicNumber != EEPROM_MAGIC_NUMBER) {
        Serial.println("No valid data in EEPROM, using default plant settings");
        return;
    }

    for (int i = 0; i < NUM_PUMPS; i++) {
        // Load name[32]
        char loadedName[32];
        EEPROM.get(addr, loadedName);
        addr += 32;
        strncpy(plants[i].name, loadedName, 32);

        // Load ozPerWatering
        EEPROM.get(addr, plants[i].ozPerWatering);
        addr += sizeof(float);

        // Load intervalMinutes
        EEPROM.get(addr, plants[i].intervalMinutes);
        addr += sizeof(int);

        // Load currentHistoryIndex
        EEPROM.get(addr, plants[i].currentHistoryIndex);
        addr += sizeof(int);

        // Load needsWatering
        EEPROM.get(addr, plants[i].needsWatering);
        addr += sizeof(bool);

        // Padding to align to 4 bytes
        addr += 3;

        // Load wateringHistory
        bool validHistory = true;
        for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
            time_t timestamp;
            EEPROM.get(addr, timestamp);
            addr += sizeof(time_t);
            float amount;
            EEPROM.get(addr, amount);
            addr += sizeof(float);

            // Validate the loaded data
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

    // Invalidate EEPROM data by resetting the magic number
    uint32_t magicNumber = 0;
    EEPROM.put(addr, magicNumber);
    addr += sizeof(uint32_t);

    // Clear plant data
    for (int i = 0; i < NUM_PUMPS; i++) {
        // Clear name
        char emptyName[32] = {0};
        EEPROM.put(addr, emptyName);
        addr += 32;

        // Zero ozPerWatering
        float zeroFloat = 0.0;
        EEPROM.put(addr, zeroFloat);
        addr += sizeof(float);

        // Zero intervalMinutes
        int zeroInt = 0;
        EEPROM.put(addr, zeroInt);
        addr += sizeof(int);

        // Zero currentHistoryIndex
        EEPROM.put(addr, zeroInt);
        addr += sizeof(int);

        // Zero needsWatering
        bool falseBool = false;
        EEPROM.put(addr, falseBool);
        addr += sizeof(bool);

        // Padding to align to 4 bytes
        addr += 3;

        // Zero wateringHistory
        for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
            time_t zeroTime = 0;
            EEPROM.put(addr, zeroTime);
            addr += sizeof(time_t);

            EEPROM.put(addr, zeroFloat);
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