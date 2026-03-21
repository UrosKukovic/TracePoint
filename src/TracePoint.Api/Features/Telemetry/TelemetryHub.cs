using Microsoft.AspNetCore.SignalR;
using Dapper;

using TracePoint.Shared;

namespace TracePoint.Api.Features.Telemetry;

public class TelemetryHub : Hub
{
    private readonly TelemetryBuffer _buffer;
    private readonly IConfiguration _config;

    public TelemetryHub(TelemetryBuffer buffer, IConfiguration config)
    {
        _buffer = buffer;
        _config = config;
    }

    // To bo poklical Next.js, ko pritisneš gumb "Start"
    public async Task StartRecording(string sessionName)
    {
        var newSessionId = Guid.NewGuid();
        _buffer.CurrentSessionId = newSessionId;

        // Takoj zapišemo v tabelo 'sessions', da vemo, da obstaja
        using (var conn = new Npgsql.NpgsqlConnection(_config.GetConnectionString("DefaultConnection")))
        {
            const string sql = "INSERT INTO sessions (id, name, created_at) VALUES (@Id, @Name, NOW())";
            await conn.ExecuteAsync(sql, new { Id = newSessionId, Name = sessionName });
        }

        Console.WriteLine($"[Session]: Started {sessionName} ({newSessionId})");
        
        // Obvestimo vse kliente (npr. Next.js), da se je snemanje začelo
        await Clients.All.SendAsync("RecordingStarted", new { id = newSessionId, name = sessionName });
    }

    public void StopRecording()
    {
        Console.WriteLine($"[Session]: Stopped recording {_buffer.CurrentSessionId}");
        _buffer.CurrentSessionId = null;
    }

    public async Task SendMeasurement(CanMeasurementDto measurement)
    {
        // 1. Real-time broadcast za Next.js graf
        await Clients.All.SendAsync("ReceiveMeasurement", measurement);

        // 2. Oddaj v buffer za vpis v bazo (v ozadju)
        _buffer.Writer.TryWrite(measurement);
    }

    public override async Task OnConnectedAsync()
    {
        Console.WriteLine($"[Hub]: Device connected: {Context.ConnectionId}");
        await base.OnConnectedAsync();
    }
}