using Dapper;
using Npgsql;
using TracePoint.Shared;
using TracePoint.Shared.Models;

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
                // Wait for data, but no longer than 1 second, so the buffer still gets flushed regularly
                if (await _buffer.Reader.WaitToReadAsync(stoppingToken))
                {
                    while (_buffer.Reader.TryRead(out var measurement))
                    {
                        batch.Add(measurement);

                        if (batch.Count >= 500)
                        {
                            await SaveBatch(batch);
                            batch.Clear();
                        }
                    }
                }

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

            await Task.Delay(500, stoppingToken);
        }
    }

    private async Task SaveBatch(List<CanMeasurementDto> items)
    {
        if (!_buffer.CurrentSessionId.HasValue) return;

        if (_buffer.StartMillis == -1 && items.Count > 0) {
            _buffer.StartMillis = items[0].TimestampMs;
        }

        var mappedItems = items.Select(x => {
            var offsetMs = x.TimestampMs - _buffer.StartMillis;
            var realTime = _buffer.StartTimeUtc.AddMilliseconds(offsetMs);

            return new {
                Time = realTime,
                CanId = (long)x.CanId,
                x.Value,
                x.Channel,
                SessionId = _buffer.CurrentSessionId.Value
            };
        });

        using var conn = new NpgsqlConnection(_connectionString);
        const string sql = @"
            INSERT INTO ""Measurements"" (""Time"", ""CanId"", ""Value"", ""Channel"", ""SessionId"") 
            VALUES (@Time, @CanId, @Value, @Channel, @SessionId)";

        await conn.ExecuteAsync(sql, mappedItems);
    }
}