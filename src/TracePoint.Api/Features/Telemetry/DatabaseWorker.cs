using Dapper;
using Npgsql;
using TracePoint.Shared;

namespace TracePoint.Api.Features.Telemetry;

public class DatabaseWorker : BackgroundService
{
    private readonly TelemetryBuffer _buffer;
    private readonly string _connectionString;
    private readonly ILogger<DatabaseWorker> _logger;

    public DatabaseWorker(TelemetryBuffer buffer, IConfiguration config, ILogger<DatabaseWorker> logger)
    {
        _buffer = buffer;
        _connectionString = config.GetConnectionString("DefaultConnection") ?? "";
        _logger = logger;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        var batch = new List<CanMeasurementDto>();

        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                // Čakaj na podatke, vendar ne predolgo (max 1 sekundo), da izpraznimo buffer redno
                if (await _buffer.Reader.WaitToReadAsync(stoppingToken))
                {
                    while (_buffer.Reader.TryRead(out var measurement))
                    {
                        batch.Add(measurement);
                        
                        // Ko dosežemo 500 zapisov, jih zapišemo v bazo
                        if (batch.Count >= 500)
                        {
                            await SaveBatch(batch);
                            batch.Clear();
                        }
                    }
                }

                // Če je po ciklu še kaj ostalo v listi, zapiši zdaj
                if (batch.Count > 0)
                {
                    await SaveBatch(batch);
                    batch.Clear();
                }
            }
            catch (Exception ex) when (ex is not OperationCanceledException)
            {
                _logger.LogError(ex, "Error in DatabaseWorker");
            }

            await Task.Delay(500, stoppingToken); // Počakaj pol sekunde pred naslednjim preverjanjem
        }
    }

    private async Task SaveBatch(List<CanMeasurementDto> items)
    {
        var sessionId = _buffer.CurrentSessionId; // Vzamemo trenutni ID iz bufferja
        if (sessionId == null) return;

        using var conn = new NpgsqlConnection(_connectionString);
        const string sql = @"
            INSERT INTO measurements (time, can_id, value, channel, session_id) 
            VALUES (to_timestamp(@TimestampMs / 1000.0), @CanId, @Value, @Channel, @SessionId)";

        var mappedItems = items.Select(x => new {
            x.TimestampMs,
            CanId = (long)x.CanId,
            x.Value,
            x.Channel,
            SessionId = sessionId // Dodamo ID seje vsem vrsticam v paketu
        });

        await conn.ExecuteAsync(sql, mappedItems);
    }
}