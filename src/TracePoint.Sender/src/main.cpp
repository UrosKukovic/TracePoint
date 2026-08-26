#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>
#include <Adafruit_NeoPixel.h>
#include <math.h> // Added for sine function

#define CAN_TX_PIN GPIO_NUM_5
#define CAN_RX_PIN GPIO_NUM_4
#define RGB_PIN GPIO_NUM_38
#define NUM_PIXELS 1

Adafruit_NeoPixel pixels(NUM_PIXELS, RGB_PIN, NEO_RGB + NEO_KHZ800);

// Variables for sine wave generation
float sineAngle = 0;
const float sineStep = 0.03; 

// Variable to track LED off-time
uint32_t ledOffMillis = 0;

void setup() {
    // Serial.begin(115200);
    delay(2000);

    pixels.begin();
    pixels.setBrightness(50);

    // Setting pins
    ESP32Can.setPins(CAN_TX_PIN, CAN_RX_PIN);

    // Setting speed
    ESP32Can.setSpeed(ESP32Can.convertSpeed(125));

    // Setting queue
    ESP32Can.setRxQueueSize(1);
    ESP32Can.setTxQueueSize(1);

    // Begin
    if (ESP32Can.begin())
    {
      Serial.println("SENDER: Controller successfully set with speed 125");
    }

    else
    {
      Serial.println("SENDER: Error with running the controller!");
    }
}

void loop() {
    static uint32_t lastStamp = 0;
    uint32_t currentMillis = millis();
    
    // LED Handling
    if (ledOffMillis > 0 && currentMillis >= ledOffMillis) {
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
        ledOffMillis = 0; 
    }

    // Send every 500 ms to test the localBuffer so that we don't stuff the buffer too fast
    if (currentMillis - lastStamp > 500) {
        lastStamp = currentMillis;

        CanFrame testFrame = {0};
        
        // Set ID to match our DBC definition (0x123 = 291 decimal)
        testFrame.identifier = 0x123; 
        testFrame.extd = 0;
        testFrame.data_length_code = 8;
        
        // Set Raw Value to 150 (0x0096)
        // Since the DBC says 0|16@1+ (Little Endian), 
        // byte 0 is Low Byte, byte 1 is High Byte.
        uint16_t rawValue = 150; 
        testFrame.data[0] = (uint8_t)(rawValue & 0xFF);         // 0x96
        testFrame.data[1] = (uint8_t)((rawValue >> 8) & 0xFF);  // 0x00
        
        // Fill the rest with zeros for a clean test
        for(int i = 2; i < 8; i++) {
            testFrame.data[i] = 0;
        }

        if (ESP32Can.writeFrame(testFrame, 1)) {
            Serial.printf("TEST SENDER: ID 0x123 | Raw: %d | Target: 0.5V\n", rawValue);
            
            pixels.setPixelColor(0, pixels.Color(0, 255, 0)); 
            pixels.show();
            ledOffMillis = currentMillis + 10; 
        } 
        else {
            Serial.println("TEST SENDER: Send failed!");
        }
    }
}