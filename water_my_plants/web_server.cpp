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
    Serial.println("\n=== Setting up web server ===");
    Serial.println("Registering routes:");
    Serial.println(" - GET /");
    Serial.println(" - GET /api/plants");
    Serial.println(" - POST /api/plants/water-now");
    Serial.println(" - PUT /api/plants/amount");
    Serial.println(" - PUT /api/plants/interval");
    Serial.println(" - PUT /api/plants/name");
    Serial.println(" - GET /test");
    
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }

    Serial.println("Setting up web server...");

    // Add error handler
    server.onNotFound([](AsyncWebServerRequest *request) {
        Serial.printf("404 - Not Found: %s\n", request->url().c_str());
        request->send(404);
    });

    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        Serial.printf("Request body received: %.*s\n", len, (char*)data);
    });

    // Set CORS headers first
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Accept, Origin");

    // Handle CORS pre-flight requests
    server.on("/*", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        Serial.println("Handling OPTIONS request");
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
                plants[plantIndex].needsWatering = true;
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
            Serial.println("Processing amount update");
            
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, (char*)data);

            if (error) {
                Serial.printf("JSON parse error: %s\n", error.c_str());
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            if (!doc.containsKey("plantIndex") || !doc.containsKey("ozPerWatering")) {
                Serial.println("Missing required fields");
                request->send(400, "application/json", "{\"error\":\"Missing required fields\"}");
                return;
            }

            int plantIndex = doc["plantIndex"].as<int>();
            float amount = doc["ozPerWatering"].as<float>();
            
            Serial.printf("Plant index: %d, New amount: %.2f\n", plantIndex, amount);

            if (plantIndex < 0 || plantIndex >= NUM_PUMPS) {
                Serial.println("Invalid plant index");
                request->send(400, "application/json", "{\"error\":\"Invalid plant index\"}");
                return;
            }

            if (amount <= 0) {
                Serial.println("Invalid amount (<=0)");
                request->send(400, "application/json", "{\"error\":\"Amount must be greater than 0\"}");
                return;
            }

            plants[plantIndex].ozPerWatering = amount;
            saveWateringTimes();
            Serial.println("Amount update successful");
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
                    plants[plantIndex].name[sizeof(plants[plantIndex].name) - 1] = '\0'; // Ensure null termination
                    saveWateringTimes();  // Save changes to EEPROM
                    request->send(200, "application/json", "{\"success\":true}");
                    return;
                } else {
                    request->send(400, "application/json", "{\"error\":\"Name too long (max 31 characters)\"}");
                    return;
                }
            }

            request->send(400, "application/json", "{\"error\":\"Invalid name\"}");
        }
    );

    // Add a test endpoint
    server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("Test endpoint hit");
        request->send(200, "text/plain", "Test endpoint working");
    });

    server.begin();
    Serial.println("Web server started");
}