#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <EEPROM.h>

#define EEPROM_SIZE (NUM_PUMPS * (sizeof(int) + WATERING_HISTORY_SIZE * (sizeof(time_t) + sizeof(float))))
#define WATERING_HISTORY_SIZE 5  // Number of watering events to store per plant
#define WATERING_TIME_ADDR 0  // Starting address for watering times

// WiFi credentials
const char* ssid = "ConnectoPatronum";
const char* password = "AccioInternet";

// Time server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;    // EST is UTC-5 * 3600 = -18000
const int daylightOffset_sec = 3600;  // 1 hour DST

// Add additional NTP servers for redundancy
const char* ntpServer1 = "time.google.com";
const char* ntpServer2 = "time.cloudflare.com";

// Constants for flow rate calculations
const float OZ_PER_MINUTE = 12.0 / 4.0;  // 12 oz / 4 minutes = 3 oz per minute
const unsigned long MILLIS_PER_OZ = (4L * 60L * 1000L) / 12L;  // 20000 milliseconds per oz

// Structure to store watering event details
struct WateringEvent {
    time_t timestamp;
    float amount;
};

// Modified Plant structure to include watering history
struct Plant {
    const char* name;        
    float ozPerWatering;     
    int intervalMinutes;     
    WateringEvent wateringHistory[WATERING_HISTORY_SIZE];  // Circular buffer of last 5 waterings
    int currentHistoryIndex;  // Index for circular buffer
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

// // Plant definitions
Plant plants[] = {
    {"Prickly Pear", 3.0, 20160, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {"Rosemary", 4.0, 10080, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {"Fittonia", 2.5, 1440, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {"Thyme", 3.0, 10080, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {"Myrtle", 4.0, 4320, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {"No Plant", 0.0, 100000, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {"Lavender", 3.0, 14400, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
    {"Mint Plant", 3.5, 2880, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false}
};

// Plant definitions
// Plant plants[] = {
//     {"Prickly Pear", 2.0, 2, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
//     {"Rosemary", 4.0, 1, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
//     {"Fittonia", 1.5, 1, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
//     {"Thyme", 2.0, 10, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
//     {"No Plant", 0.0, 15, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
//     {"Myrtle", 3.0, 5, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
//     {"Lavender", 2.0, 2, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false},
//     {"Mint Plant", 2.5, 20, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, 0, false}
// };

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

const int NUM_PUMPS = sizeof(pumps) / sizeof(pumps[0]);

bool syncTime() {
    // Configure NTP time with multiple servers
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
        
        // Configure time
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

void saveWateringTimes() {
    int addr = WATERING_TIME_ADDR;
    for (int i = 0; i < NUM_PUMPS; i++) {
        // Save current history index
        EEPROM.put(addr, plants[i].currentHistoryIndex);
        addr += sizeof(int);

        // Save watering history
        for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
            EEPROM.put(addr, plants[i].wateringHistory[j].timestamp);
            addr += sizeof(time_t);
            EEPROM.put(addr, plants[i].wateringHistory[j].amount);
            addr += sizeof(float);
        }
    }
    EEPROM.commit();
}


// Function to load watering times from EEPROM

// Modified loadWateringTimes to validate data
void loadWateringTimes() {
    int addr = WATERING_TIME_ADDR;
    time_t currentTime;
    time(&currentTime);

    for (int i = 0; i < NUM_PUMPS; i++) {
        // Load current history index
        int loadedIndex;
        EEPROM.get(addr, loadedIndex);
        addr += sizeof(int);

        // Validate index
        if (loadedIndex < 0 || loadedIndex >= WATERING_HISTORY_SIZE) {
            loadedIndex = 0;  // Reset if invalid
        }
        plants[i].currentHistoryIndex = loadedIndex;

        // Load watering history
        bool validHistory = true;
        for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
            time_t timestamp;
            EEPROM.get(addr, timestamp);
            addr += sizeof(time_t);
            float amount;
            EEPROM.get(addr, amount);
            addr += sizeof(float);

            // Validate timestamp and amount
            if (timestamp > currentTime || timestamp < 0 ||
                amount < 0 || amount > 100) {  // 100 oz as reasonable maximum
                validHistory = false;
                break;
            }

            plants[i].wateringHistory[j].timestamp = timestamp;
            plants[i].wateringHistory[j].amount = amount;
        }

        // If invalid data found, reset this plant's history
        if (!validHistory) {
            plants[i].currentHistoryIndex = 0;
            for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
                plants[i].wateringHistory[j].timestamp = 0;
                plants[i].wateringHistory[j].amount = 0;
            }
        }

        // Print loaded history
        if (plants[i].wateringHistory[0].timestamp != 0) {
            Serial.printf("Loaded watering history for %s:\n", plants[i].name);
            for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
                if (plants[i].wateringHistory[j].timestamp != 0) {
                    char timeStr[30];
                    struct tm* timeinfo = localtime(&plants[i].wateringHistory[j].timestamp);
                    if (timeinfo) {
                        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
                        Serial.printf("  %s: %.1f oz\n", timeStr,
                                    plants[i].wateringHistory[j].amount);
                    }
                }
            }
        }
    }
}

void checkWateringNeeds() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time - skipping watering check");
        return;
    }
    
    time_t now = mktime(&timeinfo);
    
    for (int i = 0; i < NUM_PUMPS; i++) {
        Plant* plant = pumps[i].plant;
        if (plant->intervalMinutes == 0) continue;  // Skip unused pumps
        
        // Get most recent watering time
        int lastIndex = (plant->currentHistoryIndex - 1 + WATERING_HISTORY_SIZE) 
                       % WATERING_HISTORY_SIZE;
        time_t lastWatered = plant->wateringHistory[lastIndex].timestamp;
        
        // Convert interval to seconds for comparison
        if (lastWatered == 0 || (now - lastWatered) >= (plant->intervalMinutes * 60)) {
            plant->needsWatering = true;
            pumps[i].runDuration = (unsigned long)(plant->ozPerWatering * MILLIS_PER_OZ);
        }
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


// Modified waterPlants function that saves times after watering
// Modified waterPlants function to store watering history
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
            // Start watering
            pumpOn(pumps[i]);
            pumps[i].isRunning = true;
            pumps[i].startTime = currentMillis;
            
            char timeStr[30];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
            Serial.printf("%s Starting to water %s\n", timeStr, pumps[i].plant->name);
        }
        else if (pumps[i].isRunning) {
            if (currentMillis - pumps[i].startTime >= pumps[i].runDuration) {
                // Stop watering
                pumpOff(pumps[i]);
                pumps[i].isRunning = false;
                
                // Store watering event in history
                int currentIndex = pumps[i].plant->currentHistoryIndex;
                pumps[i].plant->wateringHistory[currentIndex].timestamp = now;
                pumps[i].plant->wateringHistory[currentIndex].amount = 
                    pumps[i].plant->ozPerWatering;
                
                // Update circular buffer index
                pumps[i].plant->currentHistoryIndex = 
                    (currentIndex + 1) % WATERING_HISTORY_SIZE;
                
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

 
 void resetEEPROM() {
    int addr = WATERING_TIME_ADDR;
    
    // Loop through each pump's data
    for (int i = 0; i < NUM_PUMPS; i++) {
        // Reset current history index
        EEPROM.put(addr, 0);
        addr += sizeof(int);
        
        // Reset watering history entries
        for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
            // Reset timestamp
            time_t zeroTime = 0;
            EEPROM.put(addr, zeroTime);
            addr += sizeof(time_t);
            
            // Reset amount
            float zeroAmount = 0.0;
            EEPROM.put(addr, zeroAmount);
            addr += sizeof(float);
        }
    }
    
    // Commit the changes to EEPROM
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

    // Calculate the starting address for this plant's data
    int addr = WATERING_TIME_ADDR + 
               plantIndex * (sizeof(int) + WATERING_HISTORY_SIZE * (sizeof(time_t) + sizeof(float)));
    
    // Reset current history index
    EEPROM.put(addr, 0);
    addr += sizeof(int);
    
    // Reset watering history entries
    for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
        // Reset timestamp
        time_t zeroTime = 0;
        EEPROM.put(addr, zeroTime);
        addr += sizeof(time_t);
        
        // Reset amount
        float zeroAmount = 0.0;
        EEPROM.put(addr, zeroAmount);
        addr += sizeof(float);
    }
    
    // Also reset the plant's current memory values
    plants[plantIndex].currentHistoryIndex = 0;
    for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
        plants[plantIndex].wateringHistory[j].timestamp = 0;
        plants[plantIndex].wateringHistory[j].amount = 0;
    }
    
    // Commit the changes to EEPROM
    if (EEPROM.commit()) {
        Serial.printf("Successfully reset watering history for %s\n", plants[plantIndex].name);
    } else {
        Serial.printf("Failed to reset watering history for %s\n", plants[plantIndex].name);
    }
}

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
    
    
    // // Turn all pumps on full and leave them on
    // for (int i = 0; i < NUM_PUMPS; i++) {
    //     pumpOn(pumps[i]);
    // }
    //     delay(10000000);  // Give the system time to stabilize

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
    
    // Initialize pump pins
    for (int i = 0; i < NUM_PUMPS; i++) {
        pinMode(pumps[i].in1, OUTPUT);
        pinMode(pumps[i].in2, OUTPUT);
        pumpOff(pumps[i]);
    }

        // Initialize EEPROM
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("Failed to initialize EEPROM");
        return;
    }
    // resetEEPROM();

    // Load saved watering times before connecting to WiFi
    // resetPlantHistory(4);  // Since Myrtle is at index 4
    loadWateringTimes();
    
    
    Serial.println("Plant Watering System Initialized");
    printPlantSchedules();
}


void loop() {
    static unsigned long lastWiFiCheck = 0;
    static unsigned long lastTimeSync = 0;
    unsigned long currentMillis = millis();
    
    // Check WiFi more frequently - every 5 minutes
    if (currentMillis - lastWiFiCheck >= 300000) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected - reconnecting...");
            setupWiFi();
        }
        lastWiFiCheck = currentMillis;
    }
    
    // Sync time every hour instead of 12 hours
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

void pumpOn(Pump& pump) {
    digitalWrite(pump.in1, HIGH);
    digitalWrite(pump.in2, LOW);
}

void pumpOff(Pump& pump) {
    digitalWrite(pump.in1, LOW);
    digitalWrite(pump.in2, LOW);
}