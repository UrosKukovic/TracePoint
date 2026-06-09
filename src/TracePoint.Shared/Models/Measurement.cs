namespace TracePoint.Shared.Models;

public class Measurement
{
    public long Id { get; set; }
    public DateTime Time { get; set; } 
    public uint CanId { get; set; }
    public double Value { get; set; }
    public string Channel { get; set; } = string.Empty;

    public Guid SessionId { get; set; }
    public TelemetrySession Session { get; set; } = null!;
}