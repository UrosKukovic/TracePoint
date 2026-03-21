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
    Serial.begin(115200);
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
    
    // Handle turning the LED off without blocking
    if (ledOffMillis > 0 && currentMillis >= ledOffMillis) {
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
        ledOffMillis = 0; // Reset so we don't keep calling show()
    }

    // Maintain the 10ms interval (100Hz)
    if (currentMillis - lastStamp > 5) {
        lastStamp = currentMillis;

        CanFrame testFrame = {0};
        
        // 1. Randomize the ID (Standard 11-bit range: 0x100 to 0x7FF)
        testFrame.identifier = random(0x100, 0x7FF); 
        testFrame.extd = 0;
        testFrame.data_length_code = 8;
        
        // Calculate sine wave value (mapped to 0-255)
        uint8_t sineValue = (uint8_t)((sin(sineAngle) * 127.5) + 127.5);
        sineAngle += sineStep;
        if (sineAngle >= 2 * PI) sineAngle = 0;

        // 2. Fill with sine wave in the first byte, random for the rest
        testFrame.data[0] = sineValue;
        for(int i = 1; i < 8; i++) {
            testFrame.data[i] = (uint8_t)random(0, 256);
        }

        if (ESP32Can.writeFrame(testFrame, 1)) {
            Serial.printf("SENDER: Frame sent (ID: 0x%X | Sine: %d)\n", 
                          testFrame.identifier, testFrame.data[0]);

            // Logic for LEDs: Turn ON and set a future time to turn OFF
            pixels.setPixelColor(0, pixels.Color(0, 255, 0)); 
            pixels.show();
            ledOffMillis = currentMillis + 5; // LED will turn off in 5ms
        } 
        else {
            Serial.println("SENDER: Send failed!");
            pixels.setPixelColor(0, pixels.Color(255, 255, 255)); 
            pixels.show();
        }
    }
}