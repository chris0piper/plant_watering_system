#include "water_my_plants.h"
#include "homepage.h"

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
        int currentIndex = plants[i].currentHistoryIndex;
        for (int j = 0; j < WATERING_HISTORY_SIZE; j++) {
            if (j > 0) json += ",";
            // Calculate the index going backwards from current
            int historyIndex = (currentIndex - 1 - j + WATERING_HISTORY_SIZE) % WATERING_HISTORY_SIZE;
            json += "{";
            json += "\"timestamp\":" + String(plants[i].wateringHistory[historyIndex].timestamp) + ",";
            json += "\"amount\":" + String(plants[i].wateringHistory[historyIndex].amount);
            json += "}";
        }
        json += "]";
        
        json += "}";
    }
    json += "]";
    return json;
}

void setupWebServer() {
    Serial.println("\n=== Setting up web server ===");
    Serial.println("Registering routes:");
    Serial.println(" - GET /");
    Serial.println(" - GET /api/plants");
    Serial.println(" - POST /api/plants/water-now");
    Serial.println(" - PUT /api/plants/amount");
    Serial.println(" - PUT /api/plants/interval");
    Serial.println(" - PUT /api/plants/name");

    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }

    // Add error handler
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404);
    });

    // Set CORS headers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Accept, Origin");

    // Handle CORS pre-flight requests
    server.on("/*", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        request->send(200);
    });

    // Serve homepage.html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", HOMEPAGE_HTML);
    });

    // Get all plants data
    server.on("/api/plants", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        response->print(getPlantDataJson());
        request->send(response);
    });

    // Handle water now request
    server.on(
        "/api/plants/water-now", 
        HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, (char*)data);

            if (error || !doc.containsKey("plantIndex")) {
                request->send(400, "application/json", "{\"error\":\"Invalid request body\"}");
                return;
            }

            int plantIndex = doc["plantIndex"].as<int>();
            if (plantIndex >= 0 && plantIndex < NUM_PUMPS) {
                pumps[plantIndex].plant->needsWatering = true;
                pumps[plantIndex].runDuration = (unsigned long)(pumps[plantIndex].plant->ozPerWatering * MILLIS_PER_OZ);
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(400, "application/json", "{\"error\":\"Invalid plant index\"}");
            }
        }
    );

    // Handle amount update
    server.on(
        "/api/plants/amount",
        HTTP_PUT,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, (char*)data);

            if (error || !doc.containsKey("plantIndex") || !doc.containsKey("ozPerWatering")) {
                request->send(400, "application/json", "{\"error\":\"Missing required fields\"}");
                return;
            }

            int plantIndex = doc["plantIndex"].as<int>();
            float amount = doc["ozPerWatering"].as<float>();

            if (plantIndex < 0 || plantIndex >= NUM_PUMPS) {
                request->send(400, "application/json", "{\"error\":\"Invalid plant index\"}");
                return;
            }

            if (amount <= 0) {
                request->send(400, "application/json", "{\"error\":\"Amount must be greater than 0\"}");
                return;
            }

            plants[plantIndex].ozPerWatering = amount;
            saveWateringTimes();
            request->send(200, "application/json", "{\"success\":true}");
        }
    );

    // Handle interval update
    server.on(
        "/api/plants/interval",
        HTTP_PUT,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, (char*)data);

            if (error || !doc.containsKey("plantIndex") || !doc.containsKey("intervalDays")) {
                request->send(400, "application/json", "{\"error\":\"Invalid request body\"}");
                return;
            }

            int plantIndex = doc["plantIndex"].as<int>();
            float days = doc["intervalDays"].as<float>();

            if (plantIndex < 0 || plantIndex >= NUM_PUMPS) {
                request->send(400, "application/json", "{\"error\":\"Invalid plant index\"}");
                return;
            }

            if (days <= 0) {
                request->send(400, "application/json", "{\"error\":\"Interval must be greater than 0\"}");
                return;
            }

            plants[plantIndex].intervalMinutes = days * 24 * 60;
            saveWateringTimes();
            request->send(200, "application/json", "{\"success\":true}");
        }
    );

    // Handle name update
    server.on(
        "/api/plants/name",
        HTTP_PUT,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, (char*)data);

            if (error || !doc.containsKey("plantIndex") || !doc.containsKey("name")) {
                request->send(400, "application/json", "{\"error\":\"Invalid request body\"}");
                return;
            }

            int plantIndex = doc["plantIndex"].as<int>();
            const char* newName = doc["name"].as<const char*>();

            if (plantIndex < 0 || plantIndex >= NUM_PUMPS) {
                request->send(400, "application/json", "{\"error\":\"Invalid plant index\"}");
                return;
            }

            if (newName && strlen(newName) > 0) {
                size_t nameLength = strlen(newName);
                if (nameLength < sizeof(plants[plantIndex].name)) {
                    strncpy(plants[plantIndex].name, newName, sizeof(plants[plantIndex].name) - 1);
                    plants[plantIndex].name[sizeof(plants[plantIndex].name) - 1] = '\0';
                    saveWateringTimes();
                    request->send(200, "application/json", "{\"success\":true}");
                    return;
                }
                request->send(400, "application/json", "{\"error\":\"Name too long\"}");
                return;
            }
            request->send(400, "application/json", "{\"error\":\"Invalid name\"}");
        }
    );

    server.begin();
    Serial.println("Web server started");
}