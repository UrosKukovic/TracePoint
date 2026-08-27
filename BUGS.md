# BUGS.md

Log every confirmed fix here before anything else, including the commit message — logging is part of closing out the fix, not a separate step. No bug is too small or too "obvious" to skip. Each entry: **Date**, **Ticket**, **Bug**, **Solution**, **Explanation** (why it happened, why the fix works). Search for an existing entry before adding a new one — never log the same bug twice.

## Open

### Byte truncation on 16-bit CAN signal
- **Date found:** 2026-08-26
- **Ticket:** TBD (Group 1 or Group 4, see `REWORK.md`)
- **Bug:** `TracePoint.Sender` writes a 16-bit little-endian value across CAN payload bytes 0 and 1. `TracePoint.Gateway`'s `loop()` only reads `rxFrame.data[0]` into `TelemetryFrame.value` (a `float`), discarding byte 1. Any raw value above 255 is silently corrupted.
- **Solution:** TBD.
- **Explanation:** TBD once fixed.

### Unbounded offline buffer
- **Date found:** 2026-08-26
- **Ticket:** TBD (Group 4)
- **Bug:** `MAX_LOCAL_FRAMES` is computed in `TracePoint.Gateway/src/main.cpp` but never checked before `localBuffer.push_back(...)`. The buffer can grow without bound during an outage, contradicting the "offline-first" claim.
- **Solution:** TBD.
- **Explanation:** TBD once fixed.

### Gateway payload doesn't match CanMeasurementDto
- **Date found:** 2026-08-26
- **Ticket:** TBD (Group 1 or later)
- **Bug:** `CanMeasurementDto.Data` (raw CAN bytes, for server-side DBC decoding) has no equivalent in the Gateway's `TelemetryFrame` struct, which only ever sends a pre-reduced float. The README's "DBC decoding at ingest" claim doesn't hold for this binary MQTT path.
- **Solution:** TBD.
- **Explanation:** TBD once fixed.

### No recovery path when CAN bus init fails
- **Date found:** 2026-08-27
- **Ticket:** TBD (likely Group 5 — CAN fault state alongside WiFi/MQTT's state machines — but not yet named in `REWORK.md`)
- **Bug:** If `ESP32Can.begin()` fails at startup (now wrapped by `CanBus`'s constructor), both `Gateway` and `Sender` log a failure message and fall straight through into `loop()` forever. There's no retry, backoff, halt, or fault signal — `CanBus::readFrame()`/`writeFrame()` will just keep returning `false` for the life of the device, silently. This predates the RAII wrap (the old code did the same thing); wrapping it in `CanBus` didn't fix it, it just made the always-false behavior explicit instead of implicit.
- **Solution:** TBD.
- **Explanation:** TBD once fixed.

## Resolved

### Dead sine-wave code in Sender
- **Date found:** 2026-08-26
- **Date fixed:** 2026-08-26
- **Ticket:** Group 1 (housekeeping, ahead of the RAII work)
- **Bug:** `TracePoint.Sender/src/main.cpp` declared `sineAngle`, `sineStep`, and included `math.h` with a comment claiming sine-wave generation, but the code actually only ever writes a static raw value of `150` — none of the three were referenced anywhere in `setup()` or `loop()`.
- **Solution:** Removed the unused `#include <math.h>`, `sineAngle`, and `sineStep` declarations. No behavior change.
- **Explanation:** Leftover from an earlier version of the sender that generated a sine wave; the switch to a static test value never cleaned up the now-dead declarations. The fix is safe because nothing referenced them — a full-file grep confirmed zero other uses before removal.
