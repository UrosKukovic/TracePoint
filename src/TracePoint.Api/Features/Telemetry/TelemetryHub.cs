using Microsoft.AspNetCore.SignalR;


using TracePoint.Shared;
using TracePoint.Shared.Models;
using TracePoint.Api.Data;

namespace TracePoint.Api.Features.Telemetry;

public class TelemetryHub : Hub
{
    private readonly TelemetryBuffer _buffer;
    private readonly IServiceProvider _serviceProvider; // Potrebujemo za DbContext v Hubu
    private readonly ILogger<TelemetryHub> _logger;

    public TelemetryHub(TelemetryBuffer buffer, IServiceProvider serviceProvider, ILogger<TelemetryHub> logger)
    {
        _buffer = buffer;
        _serviceProvider = serviceProvider;
        _logger = logger;
    }

    public async Task StartRecording(string sessionName)
    {
        var newSessionId = Guid.NewGuid();

        // 1. Uporabimo IServiceProvider, ker je Hub "Scoped", DbContext pa tudi.
        // To je bolj varno in "EF-way" kot ročni SQL.
        using (var scope = _serviceProvider.CreateScope())
        {
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            
            var session = new TelemetrySession
            {
                Id = newSessionId,
                Name = sessionName,
                CreatedAt = DateTime.UtcNow,
                FirmwareVersion = "v1.0-poc" // Kasneje to dobiš iz ESP32 paketa
            };

            db.Sessions.Add(session);
            await db.SaveChangesAsync();
        }

        // 2. Nastavimo buffer, da DatabaseWorker ve, kam pisati meritve
        _buffer.CurrentSessionId = newSessionId;

        _logger.LogInformation("[Session]: Started {Name} ({Id})", sessionName, newSessionId);
        
        await Clients.All.SendAsync("RecordingStarted", new { id = newSessionId, name = sessionName });
    }

    public async Task StopRecording()
    {
        var oldId = _buffer.CurrentSessionId;
        _buffer.CurrentSessionId = null;
        
        _logger.LogInformation("[Session]: Stopped recording {Id}", oldId);
        await Clients.All.SendAsync("RecordingStopped", new { id = oldId });
    }

    // To kliče MqttBridgeWorker, ko prejme podatek iz ESP32
    public async Task SendMeasurement(CanMeasurementDto measurement)
    {
        // Broadcast na frontend (uPlot graf)
        await Clients.All.SendAsync("ReceiveMeasurement", measurement);

        // Če snemamo, potisni v buffer za DatabaseWorker
        if (_buffer.CurrentSessionId.HasValue)
        {
            _buffer.Writer.TryWrite(measurement);
        }
    }
}