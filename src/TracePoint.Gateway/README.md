# TracePoint firmware (Gateway + Sender)

Embedded C++ firmware for two ESP32 boards on a shared CAN bus. Together they form the edge layer of TracePoint: generate or capture CAN frames and get them to the backend over MQTT.

## Projects

| Folder | Board | Role |
|--------|-------|------|
| `TracePoint.Gateway/` | ESP32-S3 WROOM | CAN listener → local buffer → MQTT binary publish |
| `TracePoint.Sender/` | ESP32 (TWAI) | CAN frame generator for testing |

Both use **PlatformIO**, **Arduino framework**, and **ESP32-TWAI-CAN** at **125 kbps**.

## Gateway (`TracePoint.Gateway`)

### Behaviour

1. Initializes TWAI CAN on pins TX=`GPIO5`, RX=`GPIO4`.
2. On each received frame, stores a packed `TelemetryFrame` in a PSRAM-backed `std::deque`.
3. When WiFi and MQTT are connected, drains up to 50 frames from the front of the deque and publishes raw bytes to `telemetry/binary`.
4. RGB LED (NeoPixel on `GPIO38`) flashes blue on CAN activity.

### Offline handling

If the network drops, frames keep accumulating locally (bounded by `MAX_LOCAL_FRAMES`, sized from available PSRAM). When MQTT comes back, the gateway sends backlog in chunks without blocking the CAN read loop. WiFi and MQTT reconnect logic runs in the main loop (non-blocking retries).

### Binary payload

Each frame is 16 bytes, packed, little-endian — must stay in sync with `TelemetryFrameProxy` in the .NET `MqttBridgeWorker`:

```cpp
struct __attribute__((packed)) TelemetryFrame {
    uint32_t canId;
    float value;        // PoC: often first data byte cast to float
    int64_t timestampMs;
};
```

### Configuration

Edit constants at the top of `src/main.cpp`:

- `WIFI_SSID`, `WIFI_PASS`
- `MQTT_SERVER`, `MQTT_PORT` (default `1883`)
- `MQTT_TOPIC` (`telemetry/binary`)

### Dependencies (`platformio.ini`)

- `handmade0octopus/ESP32-TWAI-CAN`
- `adafruit/Adafruit NeoPixel`
- `knolleary/PubSubClient` (MQTT)
- `bblanchon/ArduinoJson`

Build flags enable PSRAM on the S3 (`-DBOARD_HAS_PSRAM`).

### Build & flash

```bash
cd src/TracePoint.Gateway
pio run -t upload
pio device monitor
```

Serial: `115200` baud.

---

## Sender (`TracePoint.Sender`)

Generates test CAN traffic so the gateway has something to read without a real vehicle ECU.

Current loop (PoC):

- Sends frame every **500 ms**
- CAN ID **`0x123`** (291 decimal) — matches `Definitions/test.dbc` in the API project
- Data bytes 0–1: little-endian `uint16` raw value `150` → decodes to **0.5 V** with factor `0.01` and offset `-1`

Same CAN pins and 125 kbps as the gateway. Green LED blink on successful TX.

```bash
cd src/TracePoint.Sender
pio run -t upload
pio device monitor
```

An alternate loop in `main.cpp` (commented out) sends random IDs with a sine wave on byte 0 — useful for stress testing buffer and chart FPS.

---

## Wiring (lab setup)

```
Sender ESP32          Gateway ESP32-S3
  CAN TX ────────────── CAN RX
  CAN GND ───────────── CAN GND
```

Use a proper CAN transceiver (e.g. SN65HVD230) on each board if you are not using a back-to-back setup that already includes one. Termination resistor (120 Ω) at the ends of the bus is required.

## Testing without MQTT

1. Run only the Sender — verify frames on serial (`TEST SENDER: ID 0x123 ...`).
2. Run Gateway + Mosquitto + API — confirm MQTT logs and live dashboard updates.
3. Disconnect WiFi mid-run — gateway should buffer; reconnect and watch backlog drain.

## Related docs

- [Project overview](../../README.md)
- [Backend MQTT bridge](../TracePoint.Api/README.md)
- Sample DBC: `../TracePoint.Api/Definitions/test.dbc`

## Known gaps

- Timestamps use `millis()` (relative), not NTP — API maps them to UTC session time on insert.
- Gateway still contains test hooks that force WiFi disconnect/reconnect after 5s/10s.
