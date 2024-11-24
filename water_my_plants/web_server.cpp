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
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }

    // Serve the homepage
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/homepage.html", "text/html");
    });

    // API endpoint for plant data
    server.on("/api/plants", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", getPlantDataJson());
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    // Start server
    server.begin();
    Serial.println("Web server started");
}