# TracePoint TODO

## Firmware

- [ ] Fix byte truncation on CAN signal decode — `Sender` writes a 16-bit little-endian value across CAN payload bytes 0–1, `Gateway` only reads byte 0
- [ ] Bound the offline buffer — `MAX_LOCAL_FRAMES` is computed but never enforced against the `std::deque`'s growth during an outage
- [ ] Send raw CAN payload bytes over MQTT so the API can do DBC decoding server-side (currently sends a pre-reduced `float` only, doesn't match `CanMeasurementDto`)
- [ ] Move WiFi/MQTT credentials and broker IP out of source into an uncommitted config header
- [ ] Remove/gate the leftover test-disconnect debug logic in the Gateway
- [ ] Add host-side unit tests for the buffer and protocol encoding
- [ ] MQTT over TLS; add application-layer payload authentication (HMAC/AEAD)
- [ ] Add a real recovery path for CAN bus init failure (currently just logs and falls through to `loop()`)

## Hardware validation (next session)

- [ ] Wire ESP32 Sender + CAN transceiver + ESP32-S3 Gateway on breadboard, CAN bus at 125 kbps
- [ ] Flash Sender and Gateway firmware, confirm frames flow end-to-end through the real hardware (not the simulator) into the dashboard
- [ ] Photograph the physical setup, or draw a wiring/schematic diagram instead if the breadboard looks too messy
- [ ] Disable the Gateway's WiFi mid-session; confirm frames keep accumulating in its local (PSRAM-backed) buffer — check Serial monitor output
- [ ] Re-enable WiFi; confirm the buffered backlog flushes to MQTT/API with no gaps or duplicates
- [ ] Screenshot the analytics view showing a continuous, gap-free signal across the outage window — this is the actual proof of the offline-first buffering claim in the README

## README polish

- [ ] Wiring/schematic diagram of the CAN bus setup
- [ ] Short demo GIF of the live dashboard updating in real time
- [ ] "Known limitations / roadmap" section
- [ ] Badges row (.NET/Next.js versions, license)
