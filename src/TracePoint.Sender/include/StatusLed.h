#pragma once

#include <Adafruit_NeoPixel.h>

#define NUM_PIXELS 1

class StatusLed {
public:
    StatusLed(int16_t pin, uint8_t brightness) : pixel(NUM_PIXELS, pin, NEO_RGB + NEO_KHZ800)
    {
        pixel.begin();
        pixel.setBrightness(brightness);
    }

    void setColor(uint8_t r, uint8_t g, uint8_t b)
    {
        pixel.setPixelColor(0, pixel.Color(r, g, b));
        pixel.show();
    }

    void off()
    {
        pixel.setPixelColor(0, pixel.Color(0, 0, 0));
        pixel.show();
    }

    // Turn off the LED upon destruction of object
    ~StatusLed()
    {
        off();
    }
private:
    Adafruit_NeoPixel pixel;
};