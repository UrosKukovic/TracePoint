#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32-TWAI-CAN.hpp>
#include <vector>
#include <Adafruit_NeoPixel.h>

// --- Configuration ---
const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PASS = "WIFI_PASS";
const char* API_URL = "http://PC_IP:5247/api/telemetry/ingest/batch"; // Use your PC's IP

// Pins for ESP32-S3 N8R8
#define CAN_TX_PIN GPIO_NUM_5
#define CAN_RX_PIN GPIO_NUM_4
#define RGB_PIN GPIO_NUM_38
#define NUM_PIXELS 1
Adafruit_NeoPixel pixels(NUM_PIXELS, RGB_PIN, NEO_RGB + NEO_KHZ800);

// Batching Settings
const int MAX_BATCH_SIZE = 10;
const int FLUSH_INTERVAL_MS = 100;
unsigned long lastFlushTime = 0;

// Data Structure (Matches your CanMeasurementDto)
struct TelemetryFrame {
    uint32_t canId;
    float value;
    long long timestampMs;
};

std::vector<TelemetryFrame> batchBuffer;

// --- Function Prototypes ---
void connectToWiFi();
void sendBatch();

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.begin(115200);
    pixels.begin();
    pixels.setBrightness(30); // Keep it dim so it doesn't blind you
    
    // Initializing feedback (Yellow = Booting/Connecting)
    pixels.setPixelColor(0, pixels.Color(255, 255, 0)); 
    pixels.show();

    // 1. Initialize WiFi
    connectToWiFi();

    // 2. Initialize CAN Bus
    ESP32Can.setPins(CAN_TX_PIN, CAN_RX_PIN);
    ESP32Can.setSpeed(ESP32Can.convertSpeed(125));
    ESP32Can.setRxQueueSize(20);

    if (ESP32Can.begin())
    {
        Serial.println("GATEWAY: CAN Initialized at 125kbps");
    }
    else
    {
        Serial.println("GATEWAY: CAN Initialization Failed!");
    }

    pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // Turn off after setup
    pixels.show();
}

void loop()
{
    CanFrame rxFrame;

    // 3. Read CAN Frames
    if (ESP32Can.readFrame(rxFrame, 5))
    {

        pixels.setPixelColor(0, pixels.Color(0, 0, 255)); 
        pixels.show();

        TelemetryFrame m;
        m.canId = rxFrame.identifier;
        m.value = (float)rxFrame.data[0]; // Logic: Taking 1st byte as value for PoC
        m.timestampMs = millis();        // Will replace with NTP later
        
        batchBuffer.push_back(m);
        Serial.printf("Captured ID: 0x%X\n", m.canId);

        delay(10); // Short visible blink
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
    }

    // 4. Periodic/Size-based Flush
    if (!batchBuffer.empty() && 
       (batchBuffer.size() >= MAX_BATCH_SIZE || (millis() - lastFlushTime > FLUSH_INTERVAL_MS)))
    {
        sendBatch();
        lastFlushTime = millis();
    }

    // 5. Keep-alive WiFi check
    if (WiFi.status() != WL_CONNECTED && millis() % 10000 == 0)
    {
        connectToWiFi();
    }
}

void connectToWiFi()
{
    Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
    }
    else
    {
        Serial.println("\nWiFi Connection Failed.");
    }
}

void sendBatch()
{
    StaticJsonDocument<2048> doc;
    JsonArray array = doc.to<JsonArray>();

    for (const auto& m : batchBuffer) 
    {
        JsonObject obj = array.createNestedObject();
        obj["canId"] = m.canId;
        obj["timestampMs"] = m.timestampMs;
        obj["value"] = m.value;
        obj["channel"] = "CAN_BUS_0";
    }

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    HTTPClient http;
    http.begin(API_URL);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode == 202)
    {
        batchBuffer.clear(); // Only clear if API accepted the data
    }
    else
    {
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
        pixels.show();

        Serial.printf("API Error [%d]: %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
    }
    http.end();
}