using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using TracePoint.Api.Data;

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
        // Samo bistveni podatki za "grd" seznam na frontendu
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
    public async Task<IActionResult> GetSessionMeasurements(Guid sessionId)
    {
        var measurements = await _db.Measurements
            .Where(m => m.SessionId == sessionId)
            .OrderBy(m => m.Time)
            .Select(m => new { m.Time, m.Value })
            .ToListAsync();

        // Formatiranje v Columnar format (C): [ [timestamps], [values] ]
        var x = measurements.Select(m => ((DateTimeOffset)m.Time).ToUnixTimeMilliseconds() / 1000.0).ToArray();
        var y = measurements.Select(m => m.Value).ToArray();

        return Ok(new[] { x, y });
    }
}