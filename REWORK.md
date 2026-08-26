# REWORK.md

## Purpose

This is the plan of record for turning TracePoint's ESP32 firmware from a single-file proof of concept into production-grade embedded C++. The technical debt identified during review, the plan used to resolve it, and every fix are tracked here and in `BUGS.md` as they land.

## Why this rework

TracePoint's system architecture (offline-first CAN gateway, binary MQTT protocol, DBC-based decoding, TimescaleDB storage, live dashboard) is sound. The firmware layer (`src/TracePoint.Gateway`, `src/TracePoint.Sender`) was written fast, as a single-file proof of concept, to validate the pipeline end-to-end. It has real bugs, no tests, and no transport/payload security.

## Bugs found during review (tracked live in `BUGS.md`)

1. **Byte truncation.** `TracePoint.Sender` encodes a 16-bit little-endian signal across CAN payload bytes 0–1 (per the DBC comment: `0|16@1+`), but `TracePoint.Gateway`'s `loop()` only reads `rxFrame.data[0]` into `TelemetryFrame.value` (as a `float`) — the high byte is silently dropped. Any raw value above 255 is corrupted before it leaves the device.
2. **Unbounded offline buffer.** `MAX_LOCAL_FRAMES` is computed (`1e6 / 16`) in `TracePoint.Gateway/src/main.cpp` but never checked before `localBuffer.push_back(...)`. The `std::deque<TelemetryFrame>` can grow without bound during a WiFi/MQTT outage, which defeats the "offline-first" claim in the README and risks exhausting PSRAM. The sizing comment ("1MB / 8MB") is also inconsistent with the ESP32-S3 N8R8's actual 8MB PSRAM.
3. **Protocol mismatch with `CanMeasurementDto`.** The shared DTO (`TracePoint.Shared/CanMeasurementDto.cs`) expects a raw `byte[] Data` field so the API can do DBC-based decoding server-side. The Gateway's binary MQTT payload (`TelemetryFrame`) never sends raw payload bytes — only a pre-reduced `float value` — so the README's "DBC decoding at ingest" claim cannot be functioning correctly over this path.
4. **Hardcoded plaintext secrets.** WiFi SSID/password and the API/MQTT broker IP are compiled directly into `main.cpp` in both firmware projects.
5. **Debug/test code left in "production" firmware.** `testDisconnectDone`/`testReconnectDone` auto-disconnect-after-5-seconds logic lives inline in the Gateway. The Sender has dead code (`sineAngle`, `sineStep`, an unused `math.h` include) — the comment claims a sine wave; the code actually sends a static value of `150`.
6. **No tests.** Neither firmware project has any unit tests. PlatformIO supports a native/host test environment for exactly this — pure logic (buffer, framing/encoding) doesn't need real hardware to test.
7. **No transport or payload security.** MQTT is plaintext (`WiFiClient`, not `WiFiClientSecure`); there's no application-layer authentication on the telemetry payload.

## Definition of done

This rework is complete when all of the following are true:

- [ ] Firmware restructured into real RAII-managed classes (no global-state single-file `main.cpp`), templates used where they genuinely fit (a shared transport interface), no unbounded dynamic allocation.
- [ ] All bugs listed above are fixed and logged in `BUGS.md` with root-cause explanations.
- [ ] Host-side unit tests exist for the buffer, protocol encoding, and any other pure-logic classes.
- [ ] MQTT runs over TLS; telemetry payloads carry an application-layer HMAC/AEAD; the per-device-key rationale is documented even if the provisioning implementation starts minimal.
- [ ] The offline-first buffering claim is proven on real hardware — WiFi killed mid-session, buffer holds, backlog drains cleanly on reconnect — with a screenshot/GIF as evidence. This closes out the "Hardware validation" section already in `TODO.md`.
- [ ] The firmware has its own standalone README section explaining the modern C++ patterns used, why, and what a genuinely production version would still need (secure key provisioning, etc.) — a reader shouldn't have to read the .NET/Next.js code to judge the C++ work.
- [ ] Git history shows the work as separate, meaningful commits per ticket, not one squash.

## Work plan — grouped by C++ topic, strictly sequential

Each group is closed out (theory understood, code written, tests passing, relevant bugs logged) before the next group starts.

### Group 1 — RAII & resource management
Wrap CAN bus init/deinit, WiFi connection lifecycle, MQTT client, and the NeoPixel status LED as RAII-managed classes. Move hardcoded config (WiFi/MQTT credentials, broker IP, pins) out of source into a not-committed config header, as part of treating configuration as a resource. Remove the leftover test-disconnect debug logic (or gate it explicitly behind a build flag).

### Group 2 — Move semantics & smart pointers
Apply move semantics wherever ownership genuinely transfers (batch buffer handoff, transport object lifetime). No raw `new`/`delete` in the result.

### Group 3 — Templates & generic interfaces
Design a generic uplink-transport interface (e.g. `IUplinkTransport`) so WiFi/MQTT is one implementation of it — sets up a future cellular (STM32L462E-CELL1) implementation as a natural extension later, not a rewrite.

### Group 4 — STL & no-heap containers
Replace the unbounded `std::deque` with a real fixed-capacity ring buffer that actually enforces `MAX_LOCAL_FRAMES`. This is where bug #2 gets properly fixed, not just capped after the fact.

### Group 5 — Modern OOP design
`NetworkManager` and `MqttPublisher` as explicit state machines (replacing the scattered `if` conditions in `loop()`), composition over inheritance, and an explicit note on where virtual dispatch cost would or wouldn't matter here.

### Group 6 — Embedded-specific concerns
`constexpr` for compile-time config, explicit no-exceptions/no-RTTI build flags, PSRAM-aware buffer sizing (fixes bug #2's sizing comment properly, tied to actual detected PSRAM).

### Group 7 — Security & cryptography
MQTT over TLS (port 8883, Mosquitto configured with a server cert). Application-layer HMAC or AES-GCM over each binary batch payload via mbedTLS on the ESP32 side, matched by `HMACSHA256`/`AesGcm` on the .NET API side. Move from one shared secret to a per-device key, documented explicitly as "production would use secure factory provisioning / a hardware secure element" if the full provisioning flow isn't built out. Optional: one hand-rolled lightweight MAC primitive, clearly labeled as a from-scratch learning exercise, contrasted against the vetted mbedTLS AEAD used for anything actually load-bearing.

### Group 8 — Hardware validation & documentation
Execute the existing `TODO.md` hardware-validation checklist for real: wire the bus, kill WiFi mid-session, confirm the buffer holds and drains cleanly, screenshot the analytics view showing a gap-free signal across the outage window. Write the firmware README section, a "Skills demonstrated" section, and a "Known limitations / roadmap" section. `TODO.md`'s content is absorbed into this group; the file is removed once this group closes.
