# TracePoint.Api

ASP.NET Core backend for the TracePoint pipeline. It connects MQTT telemetry from the ESP32 gateway to a PostgreSQL database and to browser clients over SignalR.

## Responsibilities

- Subscribe to binary CAN frames on MQTT topic `telemetry/binary`
- Decode signals using an in-memory DBC registry (uploaded from the frontend)
- Broadcast thinned live measurements to connected clients
- Manage recording sessions and persist measurements in batches
- Serve session list and downsampled historical data for analytics charts

## Main components

| File / folder | Role |
|---------------|------|
| `Features/Telemetry/MqttBridgeWorker.cs` | MQTT client; parses binary frames, applies DBC, writes to buffer, pushes to SignalR |
| `Features/Telemetry/TelemetryHub.cs` | SignalR hub — `StartRecording`, `StopRecording`, session lifecycle |
| `Features/Telemetry/DatabaseWorker.cs` | Background worker; batch inserts (500 rows) via Dapper |
| `Features/Telemetry/TelemetryBuffer.cs` | `System.Threading.Channels` queue between ingest and DB writer |
| `Features/Telemetry/DbcRegistry.cs` | Parses and caches DBC messages for O(1) lookup by CAN ID |
| `Features/Telemetry/TelemetryController.cs` | REST endpoints for sessions and measurements |
| `Features/Telemetry/UploadDbc.cs` | `POST /api/telemetry/upload-dbc` |
| `Data/AppDbContext.cs` | EF Core context for sessions and measurements |

## Data flow

```
MQTT (binary frames)
    → MqttBridgeWorker (DBC decode)
        → TelemetryBuffer (always, when recording)
        → SignalR (every 10th frame, live preview)
    → DatabaseWorker
        → PostgreSQL / TimescaleDB
```

### Binary frame format

Matches the packed struct on the gateway (16 bytes):

```
uint32  CanId
float   Value      (raw value before DBC, or first-byte shortcut in PoC)
int64   TimestampMs
```

## API endpoints

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/api/telemetry/sessions` | List recorded sessions |
| `GET` | `/api/telemetry/sessions/{id}/measurements?min=&max=` | Downsampled `[x[], y[]]` for uPlot |
| `POST` | `/api/telemetry/upload-dbc` | Upload `.dbc` file (multipart form) |
| WS | `/telemetryHub` | SignalR — `ReceiveMeasurement`, `StartRecording`, `StopRecording` |

Historical queries use TimescaleDB `time_bucket` to return roughly 2000 points for the selected time range. Pass `min` and `max` as Unix seconds to zoom.

## Tech stack

- .NET 10, ASP.NET Core
- SignalR, MQTTnet 5
- EF Core + Npgsql, Dapper for bulk inserts
- DbcParserLib for CAN database files
- MediatR (registered; room for CQRS handlers)

## Configuration

`appsettings.json`:

```json
{
  "ConnectionStrings": {
    "DefaultConnection": "Host=localhost;Port=5432;Database=tracepoint;Username=postgres;Password=postgres"
  }
}
```

MQTT broker is expected at `127.0.0.1:1883` (hardcoded in `MqttBridgeWorker` for now).

CORS allows `http://localhost:3000` for the Next.js app.

## Run locally

```bash
# from this directory
dotnet restore
dotnet ef database update   # requires dotnet-ef tool
dotnet run
```

Default URL: `http://localhost:5247`

Test endpoints with `TracePoint.Api.http` or curl:

```bash
curl http://localhost:5247/api/telemetry/sessions
```

## Related projects

- `../TracePoint.Shared` — `CanMeasurementDto`, `Measurement`, `TelemetrySession`
- `../TracePoint.Simulator` — sends mock data to SignalR when hardware is not available
- `../tracepoint-web` — frontend consumer

## Notes / limitations

- DBC decoding currently looks for a signal named `SineWave` in the matched message (PoC shortcut).
- No authentication on API or SignalR.
- HTTPS redirection is enabled; local dev usually runs on HTTP port 5247.
