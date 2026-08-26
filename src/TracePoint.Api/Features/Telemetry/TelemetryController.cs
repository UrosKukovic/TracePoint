using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using TracePoint.Api.Data;
using Npgsql;
using Dapper;

namespace TracePoint.Api.Features.Telemetry;

[ApiController]
[Route("api/[controller]")]
public class TelemetryController : ControllerBase
{
    private readonly AppDbContext _db;

    public TelemetryController(AppDbContext db)
    {
        _db = db;
    }

    [HttpGet("sessions")]
    public async Task<IActionResult> GetSessions()
    {
        var sessions = await _db.Sessions
            .OrderByDescending(s => s.CreatedAt)
            .Select(s => new 
            { 
                s.Id, 
                s.Name, 
                s.CreatedAt,
                s.FirmwareVersion
            })
            .ToListAsync();

        return Ok(sessions);
    }

    [HttpGet("sessions/{sessionId}/measurements")]
    public async Task<IActionResult> GetSessionMeasurements(
        Guid sessionId, 
        [FromQuery] double? min, 
        [FromQuery] double? max)
    {
        using var conn = new NpgsqlConnection(_db.Database.GetConnectionString());
        await conn.OpenAsync();

        if (min == null || max == null)
        {
            var range = await conn.QuerySingleOrDefaultAsync<(DateTime? start, DateTime? end)>(
                "SELECT MIN(\"Time\"), MAX(\"Time\") FROM \"Measurements\" WHERE \"SessionId\" = @sessionId",
                new { sessionId });

            if (range.start == null || range.end == null)
            {
                return Ok(new[] { Array.Empty<double>(), Array.Empty<double>() });
            }

            min = ((DateTimeOffset)range.start.Value).ToUnixTimeMilliseconds() / 1000.0;
            max = ((DateTimeOffset)range.end.Value).ToUnixTimeMilliseconds() / 1000.0;
        }

        var startTime = DateTimeOffset.FromUnixTimeMilliseconds((long)(min * 1000)).UtcDateTime;
        var endTime = DateTimeOffset.FromUnixTimeMilliseconds((long)(max * 1000)).UtcDateTime;

        var durationMs = (endTime - startTime).TotalMilliseconds;
        var bucketMs = Math.Max(1, durationMs / 2000);

        string sql = @"
            SELECT 
                time_bucket(@interval, ""Time"") AS Bucket,
                AVG(""Value"") AS Val
            FROM ""Measurements""
            WHERE ""SessionId"" = @sessionId AND ""Time"" BETWEEN @startTime AND @endTime
            GROUP BY Bucket
            ORDER BY Bucket ASC";

        try
        {
            var data = await conn.QueryAsync<(DateTime Bucket, double Val)>(sql,
                new { interval = TimeSpan.FromMilliseconds(bucketMs), sessionId, startTime, endTime });

            var x = data.Select(d => ((DateTimeOffset)d.Bucket).ToUnixTimeMilliseconds() / 1000.0).ToArray();
            var y = data.Select(d => d.Val).ToArray();

            return Ok(new[] { x, y });
        }
        catch (Exception ex)
        {
            return BadRequest(ex.Message);
        }
    }
}