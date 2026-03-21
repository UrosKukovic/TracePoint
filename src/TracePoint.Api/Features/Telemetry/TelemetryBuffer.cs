using System.Threading.Channels;
using TracePoint.Shared;

namespace TracePoint.Api.Features.Telemetry;

public class TelemetryBuffer
{
    public Guid? CurrentSessionId { get; set; }
    private readonly Channel<CanMeasurementDto> _channel;

    public TelemetryBuffer()
    {
        // Unbounded pomeni, da lahko sprejme ogromno podatkov brez čakanja
        _channel = Channel.CreateUnbounded<CanMeasurementDto>();
    }

    public ChannelReader<CanMeasurementDto> Reader => _channel.Reader;
    public ChannelWriter<CanMeasurementDto> Writer => _channel.Writer;
}