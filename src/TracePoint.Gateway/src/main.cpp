#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>
#include <Adafruit_NeoPixel.h>

#define CAN_TX_PIN GPIO_NUM_5
#define CAN_RX_PIN GPIO_NUM_4
#define RGB_PIN GPIO_NUM_38
#define NUM_PIXELS 1

Adafruit_NeoPixel pixels(NUM_PIXELS, RGB_PIN, NEO_RGB + NEO_KHZ800);
CanFrame rxFrame;

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
    ESP32Can.setRxQueueSize(10);

    // Begin
    if (ESP32Can.begin())
    {
      Serial.println("GATEWAY: Controller successfully set with speed 125");
    }

    else
    {
      Serial.println("GATEWAY: Error with running the controller!");
    }
}

void loop() {
    // Waiting for packet
    if(ESP32Can.readFrame(rxFrame, 1000)) 
    {
        // packet received
        // Turn on RED
        pixels.setPixelColor(0, pixels.Color(255, 0, 0)); 
        pixels.show();

        Serial.printf("GATEWAY: Received ID: %03X | Data: ", rxFrame.identifier);
        for(int i = 0; i < rxFrame.data_length_code; i++) {
            Serial.printf("%02X ", rxFrame.data[i]);
        }
        Serial.println();

        // Small delay and then turn off the RED
        delay(50);
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
    }
    else 
    {
        // timeout 1 second without packet
        // Blink white LED
        Serial.println("GATEWAY: Not receiving any packets");
        
        pixels.setPixelColor(0, pixels.Color(255, 255, 255));
        pixels.show();
        delay(200);
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
    }
}