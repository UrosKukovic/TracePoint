# TracePoint TODO

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
- [ ] "Skills demonstrated" section (separate from the Tech stack table)
- [ ] "Known limitations / roadmap" section
- [ ] Badges row (.NET/Next.js versions, license)
