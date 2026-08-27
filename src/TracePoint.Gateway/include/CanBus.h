#include <ESP32-TWAI-CAN.hpp>

class CanBus {
public:
    CanBus(int16_t tx_pin, int16_t rx_pin, TwaiSpeed speed, uint8_t queueSize)
    {
        ESP32Can.setPins(tx_pin, rx_pin);
        ESP32Can.setSpeed(speed);
        ESP32Can.setRxQueueSize(queueSize);
        result = ESP32Can.begin();
    }

    bool readFrame(CanFrame& frame, uint32_t timeout)
    {
        if (!result)
            return false;
        else
            return ESP32Can.readFrame(frame, timeout);
    }

    bool writeFrame(CanFrame& frame, uint32_t timeout)
    {
        if (!result)
            return false;
        else
            return ESP32Can.writeFrame(frame, timeout);
    }

    explicit operator bool() const
    {
        return result;
    }

    ~CanBus()
    {
        ESP32Can.end();
    }
private:
    bool result;
};