#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>
#include <Adafruit_NeoPixel.h>

#define CAN_TX_PIN GPIO_NUM_5
#define CAN_RX_PIN GPIO_NUM_4
#define RGB_PIN GPIO_NUM_38
#define NUM_PIXELS 1

Adafruit_NeoPixel pixels(NUM_PIXELS, RGB_PIN, NEO_RGB + NEO_KHZ800);

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
    
    if (millis() - lastStamp > 100) {
        lastStamp = millis();

        CanFrame testFrame = {0};
        testFrame.identifier = 0x101; 
        testFrame.extd = 0;
        testFrame.data_length_code = 8;
        
        for(int i = 0; i < 8; i++) {
            testFrame.data[i] = 0xAA;
        }
        testFrame.data[0] = 0xDE;
        testFrame.data[1] = 0xAD;

        // turn on green led
        pixels.setPixelColor(0, pixels.Color(0, 255, 0)); 
        pixels.show();

        if (ESP32Can.writeFrame(testFrame, 1)) {
            Serial.println("SENDER: Frame sent successfully (0x101)");

            // Comm success
            delay(50); 
            pixels.setPixelColor(0, pixels.Color(0, 0, 0));
            pixels.show();
        } 
        else {
            Serial.println("SENDER: Send failed!");
            
            // Error (turn on white led)
            pixels.setPixelColor(0, pixels.Color(255, 255, 255)); 
            pixels.show();
        }
    }
}