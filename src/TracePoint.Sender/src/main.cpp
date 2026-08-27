#include "StatusLed.h"
#include "CanBus.h"

#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>

#define CAN_TX_PIN GPIO_NUM_5
#define CAN_RX_PIN GPIO_NUM_4
#define CAN_SPEED 125
#define CAN_TX_QUEUE_SIZE 1
#define CAN_RX_QUEUE_SIZE 1
#define RGB_PIN GPIO_NUM_38

// LED object creation
StatusLed statusLed(RGB_PIN, 50);
// CanBus object creation
CanBus canBus(CAN_TX_PIN, CAN_RX_PIN, ESP32Can.convertSpeed(CAN_SPEED), CAN_TX_QUEUE_SIZE, CAN_RX_QUEUE_SIZE);

// Variable to track LED off-time
uint32_t ledOffMillis = 0;

void setup() {
    // Serial.begin(115200);
    delay(2000);
}

void loop() {
    static uint32_t lastStamp = 0;
    uint32_t currentMillis = millis();
    
    // LED Handling
    if (ledOffMillis > 0 && currentMillis >= ledOffMillis) {
        // Turn off the LED
        statusLed.off();
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

        if (canBus.writeFrame(testFrame, 1)) {
            Serial.printf("TEST SENDER: ID 0x123 | Raw: %d | Target: 0.5V\n", rawValue);
            // set to green LED upon sending a CAN frame
            statusLed.setColor(0,255,0);
            ledOffMillis = currentMillis + 10; 
        } 
        else {
            Serial.println("TEST SENDER: Send failed!");
        }
    }
}