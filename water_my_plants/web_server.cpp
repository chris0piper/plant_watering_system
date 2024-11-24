#include "water_my_plants.h"

AsyncWebServer server(80);

// Convert plant data to JSON
String getPlantDataJson() {
    String json = "[";
    for (int i = 0; i < NUM_PUMPS; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"name\":\"" + String(plants[i].name) + "\",";
        json += "\"ozPerWatering\":" + String(plants[i].ozPerWatering) + ",";
        json += "\"intervalMinutes\":" + String(plants[i].intervalMinutes) + ",";
        json += "\"needsWatering\":" + String(plants[i].needsWatering ? "true" : "false") + ",";
        
        // Add watering history
        json += "\"wateringHistory\":[";
        for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
            if (j > 0) json += ",";
            json += "{";
            json += "\"timestamp\":" + String(plants[i].wateringHistory[j].timestamp) + ",";
            json += "\"amount\":" + String(plants[i].wateringHistory[j].amount);
            json += "}";
        }
        json += "]";
        
        json += "}";
    }
    json += "]";
    return json;
}

void setupWebServer() {
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }

    // Set up CORS headers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    // Serve homepage.html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/homepage.html", "text/html");
    });

    // Get all plants data
    server.on("/api/plants", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        response->print(getPlantDataJson());
        request->send(response);
    });

    // Handle water now request - with all required handlers
    server.on(
        "^\\/api\\/plants\\/([0-9]+)\\/water-now$", 
        HTTP_POST,
        [](AsyncWebServerRequest *request) {
            String indexStr = request->pathArg(0);
            int index = indexStr.toInt();
            
            if (index >= 0 && index < NUM_PUMPS) {
                plants[index].needsWatering = true;
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(400, "application/json", "{\"error\":\"Invalid plant index\"}");
            }
        },
        nullptr,  // onUpload
        nullptr   // onBody
    );

    // Handle amount update
    server.on(
        "^\\/api\\/plants\\/([0-9]+)\\/amount$",
        HTTP_PUT,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String indexStr = request->pathArg(0);
            int plantIndex = indexStr.toInt();

            if (plantIndex >= 0 && plantIndex < NUM_PUMPS) {
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, (char*)data);

                if (!error && doc.containsKey("ozPerWatering")) {
                    float amount = doc["ozPerWatering"].as<float>();
                    if (amount > 0) {
                        plants[plantIndex].ozPerWatering = amount;
                        saveWateringTimes();  // Save changes to EEPROM
                        request->send(200, "application/json", "{\"success\":true}");
                        return;
                    }
                }
                request->send(400, "application/json", "{\"error\":\"Invalid amount\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"Invalid plant index\"}");
            }
        }
    );
    // Handle interval update
    server.on(
        "^\\/api\\/plants\\/([0-9]+)\\/interval$",
        HTTP_PUT,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String indexStr = request->pathArg(0);
            int plantIndex = indexStr.toInt();

            if (plantIndex >= 0 && plantIndex < NUM_PUMPS) {
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, (char*)data);

                if (!error && doc.containsKey("intervalDays")) {
                    float days = doc["intervalDays"].as<float>();
                    if (days > 0) {
                        plants[plantIndex].intervalMinutes = days * 24 * 60;
                        saveWateringTimes();  // Save changes to EEPROM
                        request->send(200, "application/json", "{\"success\":true}");
                        return;
                    }
                }
                request->send(400, "application/json", "{\"error\":\"Invalid interval\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"Invalid plant index\"}");
            }
        }
    );


    // Handle name update
    server.on(
        "^\\/api\\/plants\\/([0-9]+)\\/name$",
        HTTP_PUT,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String indexStr = request->pathArg(0);
            int plantIndex = indexStr.toInt();

            if (plantIndex >= 0 && plantIndex < NUM_PUMPS) {
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, (char*)data);

                if (!error && doc.containsKey("name")) {
                    const char* newName = doc["name"].as<const char*>();
                    if (newName && strlen(newName) > 0) {
                        size_t nameLength = strlen(newName);
                        if (nameLength < sizeof(plants[plantIndex].name)) {
                            strncpy(plants[plantIndex].name, newName, sizeof(plants[plantIndex].name) - 1);
                            plants[plantIndex].name[sizeof(plants[plantIndex].name) - 1] = '\0'; // Ensure null termination
                            saveWateringTimes();  // Save changes to EEPROM
                            request->send(200, "application/json", "{\"success\":true}");
                            return;
                        } else {
                            request->send(400, "application/json", "{\"error\":\"Name too long (max 31 characters)\"}");
                            return;
                        }
                    }
                }
                request->send(400, "application/json", "{\"error\":\"Invalid name\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"Invalid plant index\"}");
            }
        }
    );

    server.begin();
    Serial.println("Web server started");
}