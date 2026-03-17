namespace TracePoint.Shared;

public class CanMeasurementDto
{
    // When it happened
    public long TimestampMs { get; set; }
    
    // CAN ID
    public uint CanId { get; set; }
    
    // Raw CAN data
    public byte[] Data { get; set; } = Array.Empty<byte>();
    
    // Channel name
    public string Channel { get; set; } = "RAW_CAN";
    
    // Value (calculated value if we like)
    public double Value { get; set; }
}