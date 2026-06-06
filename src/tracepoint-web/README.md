# tracepoint-web

Next.js frontend for TracePoint — live CAN telemetry dashboard, session recording controls, and historical analytics.

## Pages

| Route | Purpose |
|-------|---------|
| `/` | Live dashboard — SignalR stream, uPlot chart, DBC upload, start/stop recording, session list |
| `/analytics/[sessionId]` | Recorded session chart with drag-to-zoom; zoom range stored in URL query params |

## Features

- **Live chart** — subscribes to `ReceiveMeasurement` on `/telemetryHub`; uses uPlot for performance with rolling 200-point window
- **Recording** — calls `StartRecording` / `StopRecording` on the SignalR hub; session name is generated on the client
- **DBC upload** — sends `.dbc` file to `POST /api/telemetry/upload-dbc` so the backend can decode raw CAN values
- **Session history** — fetches `GET /api/telemetry/sessions` and links to analytics per session
- **Analytics zoom** — selecting a range updates `?min=&max=` in the URL (shareable view); double-click resets

## Tech stack

- Next.js 16 (App Router), React 19, TypeScript
- Tailwind CSS 4
- [@microsoft/signalr](https://www.npmjs.com/package/@microsoft/signalr) for real-time data
- [uPlot](https://github.com/leeoniya/uPlot) + `uplot-react` for charts (live and historical)

## Prerequisites

- Node.js 20+
- TracePoint.Api running on `http://localhost:5247`
- Mosquitto + gateway (or simulator) if you want real CAN data on the live chart

## Run locally

```bash
npm install
npm run dev
```

Open [http://localhost:3000](http://localhost:3000).

Production build:

```bash
npm run build
npm start
```

## Backend URLs

API and SignalR endpoints are currently hardcoded to `http://localhost:5247`. Main touch points:

- `app/page.tsx` — live dashboard, SignalR connection, DBC upload, sessions
- `app/analytics/[sessionId]/page.tsx` — historical measurements with zoom

Move these to environment variables (e.g. `NEXT_PUBLIC_API_URL`) when deploying.

## UI structure

```
app/
├── page.tsx                    # Live dashboard
├── analytics/[sessionId]/
│   └── page.tsx                # Session analytics
├── globals.css
└── layout.tsx
```

## Development notes

- SignalR connection is kept as a module-level singleton so navigation does not reconnect on every render.
- Live stream receives ~10% of frames (throttled server-side); the database still stores all frames during recording.
- Chart time axis for live data is adjusted with a client-side offset so ESP `millis()` aligns with wall clock display.

## Related docs

- [Project overview](../../README.md)
- [Backend README](../TracePoint.Api/README.md)
