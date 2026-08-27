#include "StatusLed.h"
#include "CanBus.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32-TWAI-CAN.hpp>
#include <vector>
#include <deque>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>

// --- Configuration ---
const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PASS = "WIFI_PASS";
const char* API_URL = "http://PC_IP:5247/api/telemetry/ingest/batch";

// MQTT config
const char* MQTT_SERVER = "PC_IP";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "telemetry/binary";

// Pins for ESP32-S3 N8R8
#define CAN_TX_PIN GPIO_NUM_5
#define CAN_RX_PIN GPIO_NUM_4
#define CAN_SPEED 125
#define CAN_QUEUE_SIZE 20
#define RGB_PIN GPIO_NUM_38

// LED object creation
StatusLed statusLed(RGB_PIN, 30);
// CanBus object creation
CanBus canBus(CAN_TX_PIN, CAN_RX_PIN, ESP32Can.convertSpeed(CAN_SPEED), CAN_QUEUE_SIZE);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Batching Settings
const int MAX_BATCH_SIZE = 1; // pri 10 je imel chart low fps, pri 1 ima velik fps
const int FLUSH_INTERVAL_MS = 100;
unsigned long lastFlushTime = 0;

static unsigned long ledOffTime = 0;

bool isPSRAMFound = false;

// Data Structure (Matches your CanMeasurementDto)
struct __attribute__((packed)) TelemetryFrame {
    uint32_t canId;
    float value;
    long long timestampMs;
};

enum class State
{
    ONLINE_MODE,
    OFFLINE_MODE
};

static State currentState = State::OFFLINE_MODE;

std::vector<TelemetryFrame> batchBuffer;
std::deque<TelemetryFrame> localBuffer;

// Define max number of local frames
const size_t MAX_LOCAL_FRAMES = 1e6 / 16; // 1MB / 8MB

static unsigned long lastConnCheck = 0;

// DEBUG
bool testDisconnectDone = false;
bool testReconnectDone = false;
unsigned long startTime = 0;

// Function Prototypes
void connectToWiFi();
void sendBatch();
void reconnectMqtt();

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.begin(115200);
    
    // Initializing feedback (Yellow = Booting/Connecting)
    statusLed.setColor(255,255,0);

    // Initialize WiFi
    connectToWiFi();

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setBufferSize(4096);

    // Turn off the LED after setup
    statusLed.off();

    // Check if PSRAM is detected
    Serial.println("PSRAM check");
    if (psramFound())
    {
        isPSRAMFound = true;
        Serial.printf("PSRAM found. Total: %d bytes\n", ESP.getPsramSize());
        Serial.printf("PSRAM found. Free: %d bytes\n", ESP.getFreePsram());
    }

    // DEBUG
    // Delay 15 seconds so that we can begin to record a session or open a serial monitor
    delay(15000);
    startTime = millis();
}

void loop() 
{
    unsigned long now = millis();

    // WiFi Reconnect Logic (Keep this independent)
    if (WiFi.status() != WL_CONNECTED && (now - lastConnCheck > 10000)) {
        lastConnCheck = now;
        WiFi.begin(WIFI_SSID, WIFI_PASS);
    }

    // MQTT Reconnect Logic (Keep this independent)
    if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) {
        static unsigned long lastMqttRetry = 0;
        if (now - lastMqttRetry > 5000) {
            lastMqttRetry = now;
            reconnectMqtt(); // This is now non-blocking
        }
    }

    mqttClient.loop();

    // Update state based on reality
    bool isOnline = (WiFi.status() == WL_CONNECTED && mqttClient.connected());
    currentState = isOnline ? State::ONLINE_MODE : State::OFFLINE_MODE;

    // TEST TRIGGER: Disconnect after 5s
    if (!testDisconnectDone && (now - startTime > 5000)) {
        Serial.println("--- TEST: Simulating Outage (Disconnecting) ---");
        WiFi.disconnect();
        testDisconnectDone = true;
    }
    
    // TEST TRIGGER: Reconnect after 10s (5s after disconnect)
    if (testDisconnectDone && !testReconnectDone && (now - startTime > 10000)) {
        Serial.println("--- TEST: Recovering (Connecting) ---");
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        testReconnectDone = true;
    }
    
    CanFrame rxFrame;

    // Read CAN Frames (always store inside localBuffer)
    if (canBus.readFrame(rxFrame, 5))
    {
        // blue LED upon CAN frame received
        statusLed.setColor(0,0,255);
        ledOffTime = millis() + 10;

        TelemetryFrame m;
        m.canId = rxFrame.identifier;
        m.value = (float)rxFrame.data[0]; // Logic: Taking 1st byte as value for PoC
        m.timestampMs = millis();        // Will replace with NTP later
        
        // Push to the back of the queue
        localBuffer.push_back(m);

        // DEBUG
        Serial.printf("Saved to localBuffer. LocalBuffer size: %d, PSRAM free: %d\n", localBuffer.size(), ESP.getFreePsram());

        // Turn off the LED
        statusLed.off();
    }

    if (millis() > ledOffTime)
    {
        // Turn off the LED
        statusLed.off();
    }

    // If online, move data from local to batchBuffer
    if (currentState == State::ONLINE_MODE && !localBuffer.empty())
    {
        // As soon as we get ONLINE, we send a batch not waiting for batch size or anything else
        batchBuffer.clear();

        // Lets first send max 50 frames
        int toSend = min((int)localBuffer.size(), 50);

        for (int i=0; i<toSend;i++)
        {
            // grab from the latest end and push it to the back of the batchBuffer
            batchBuffer.push_back(localBuffer.front());
            localBuffer.pop_front();
        }

        sendBatch();
        lastFlushTime = millis();

        Serial.printf("Emptying localBuffer. LocalBuffer size: %d, PSRAM free: %d\n", localBuffer.size(), ESP.getFreePsram());

    }

    if (millis() - lastConnCheck > 10000)
    {
        lastConnCheck = millis();
        if (WiFi.status() != WL_CONNECTED)
            // trigger the WiFi reconnect
            WiFi.begin(WIFI_SSID, WIFI_PASS);
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

    // Calculate size in bytes: number of elements * 16 bytes (one frame)
    size_t packetSize = batchBuffer.size() * sizeof(TelemetryFrame);
    
    // Pointer to the start of the data vector
    const uint8_t* payload = reinterpret_cast<const uint8_t*>(batchBuffer.data());

    // send raw bytes from batchBuffer
    if (mqttClient.publish(MQTT_TOPIC, payload, packetSize))
    {
        Serial.printf("MQTT Binary: Sent %d frames\n", batchBuffer.size());
        // Clear batchBuffer upon successful batch send
        batchBuffer.clear();
        lastFlushTime = millis();
    }
    else
    {
        Serial.println("MQTT: Publish failed! Check 'mqttClient.setBufferSize(4096)' in setup.");
    }
}

void reconnectMqtt() {
    if (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect("TracePoint_Gateway_S3")) {
            Serial.println("connected");
        } else {
            Serial.printf("failed, rc=%d\n", mqttClient.state());
        }
    }
}