namespace TracePoint.Shared.Models;

public class TelemetrySession
{
    public Guid Id { get; set; }
    public string Name { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public string? CreatedBy { get; set; }
    public string? FirmwareVersion { get; set; }

    // Navigational property for EF Core
    public ICollection<Measurement> Measurements { get; set; } = new List<Measurement>();
}