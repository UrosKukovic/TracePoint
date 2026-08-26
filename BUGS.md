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

## Resolved

(none yet)
