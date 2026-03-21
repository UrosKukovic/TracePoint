#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32-TWAI-CAN.hpp>
#include <vector>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>

// --- Configuration ---
const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PASS = "WIFI_PASS";
const char* API_URL = "http://PC_IP:5247/api/telemetry/ingest/batch"; // Use your PC's IP

// MQTT config
const char* MQTT_SERVER = "PC_IP";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "telemetry/binary";

// Pins for ESP32-S3 N8R8
#define CAN_TX_PIN GPIO_NUM_5
#define CAN_RX_PIN GPIO_NUM_4
#define RGB_PIN GPIO_NUM_38
#define NUM_PIXELS 1
Adafruit_NeoPixel pixels(NUM_PIXELS, RGB_PIN, NEO_RGB + NEO_KHZ800);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Batching Settings
const int MAX_BATCH_SIZE = 10;
const int FLUSH_INTERVAL_MS = 100;
unsigned long lastFlushTime = 0;

// Data Structure (Matches your CanMeasurementDto)
struct __attribute__((packed)) TelemetryFrame {
    uint32_t canId;
    float value;
    long long timestampMs;
};

std::vector<TelemetryFrame> batchBuffer;

// --- Function Prototypes ---
void connectToWiFi();
void sendBatch();
void reconnectMqtt();

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

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setBufferSize(4096);

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
    if (!mqttClient.connected()) {
        reconnectMqtt();
    }
    mqttClient.loop();

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
    if (!mqttClient.connected()) {
        reconnectMqtt();
    }

    // Izračunamo velikost v bajtih: število elementov * 16 bajtov (velikost strukture)
    size_t packetSize = batchBuffer.size() * sizeof(TelemetryFrame);
    
    // Dobimo kazalec na začetek podatkov v vektorju
    const uint8_t* payload = reinterpret_cast<const uint8_t*>(batchBuffer.data());

    // Pošljemo surove bajte neposredno iz pomnilnika
    if (mqttClient.publish(MQTT_TOPIC, payload, packetSize))
    {
        Serial.printf("MQTT Binary: Sent %d frames\n", batchBuffer.size());
        batchBuffer.clear();
        lastFlushTime = millis();
    }
    else
    {
        Serial.println("MQTT: Publish failed! Preveri 'mqttClient.setBufferSize(4096)' v setupu.");
    }
}

void reconnectMqtt() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Poskusi se povezati z unikatnim ID-jem
        if (mqttClient.connect("TracePoint_Gateway_S3")) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 2 seconds");
            delay(2000);
        }
    }
}