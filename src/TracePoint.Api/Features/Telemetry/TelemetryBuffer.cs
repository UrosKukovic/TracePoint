using System.Threading.Channels;
using TracePoint.Shared;

namespace TracePoint.Api.Features.Telemetry;

public class TelemetryBuffer
{
    public Guid? CurrentSessionId { get; set; }
    private readonly Channel<CanMeasurementDto> _channel;

    public long StartMillis { get; set; }
    public DateTime StartTimeUtc { get; set; }

    public TelemetryBuffer()
    {
        // Unbounded so producers never block waiting for the database to catch up
        _channel = Channel.CreateUnbounded<CanMeasurementDto>();
    }

    public ChannelReader<CanMeasurementDto> Reader => _channel.Reader;
    public ChannelWriter<CanMeasurementDto> Writer => _channel.Writer;
}