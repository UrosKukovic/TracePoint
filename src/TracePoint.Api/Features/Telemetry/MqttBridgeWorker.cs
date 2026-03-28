namespace TracePoint.Api.Features.Telemetry;

using Microsoft.AspNetCore.SignalR;
using MQTTnet;
using System.Runtime.InteropServices;
using System.Buffers;

using TracePoint.Shared;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct TelemetryFrameProxy
{
    public uint CanId;        // 4 bajti
    public float Value;       // 4 bajti
    public long TimestampMs;  // 8 bajtov
}


public class MqttBridgeWorker : BackgroundService
{
    const string MQTT_TOPIC = "telemetry/binary";
    private readonly IHubContext<TelemetryHub> _hubContext;
    private readonly TelemetryBuffer _buffer;
    private readonly ILogger<MqttBridgeWorker> _logger;
    private readonly MqttClientFactory _mqttFactory;
    private int _liveSkipCounter = 0;

    public MqttBridgeWorker(
        IHubContext<TelemetryHub> hubContext, 
        TelemetryBuffer buffer, 
        ILogger<MqttBridgeWorker> logger)
    {
        _hubContext = hubContext;
        _buffer = buffer;
        _logger = logger;
        _mqttFactory = new MqttClientFactory();
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        
        using var mqttClient = _mqttFactory.CreateMqttClient();

        var mqttClientOptions = new MqttClientOptionsBuilder()
            .WithTcpServer("127.0.0.1", 1883)
            .WithCleanSession()
            .Build();

        
        // Nastavitev procesiranja sporočil
        mqttClient.ApplicationMessageReceivedAsync += async e =>
        {
            
            var sequence = e.ApplicationMessage.Payload;
            if (sequence.IsEmpty) return;

            // 1. Pretvorimo v polje (Heap-allocated), ki preživi 'await'
            byte[] dataArray = sequence.ToArray(); 
            
            int frameSize = Marshal.SizeOf<TelemetryFrameProxy>();
            int frameCount = dataArray.Length / frameSize;

            for (int i = 0; i < frameCount; i++)
            {
                int offset = i * frameSize;
                var frame = MemoryMarshal.Read<TelemetryFrameProxy>(dataArray.AsSpan(offset, frameSize));

                var dto = new CanMeasurementDto 
                {
                    CanId = frame.CanId,
                    Value = frame.Value,
                    TimestampMs = frame.TimestampMs,
                    Channel = "CAN_BUS_0"
                };

                // 1. ALWAYS write to buffer for Database (100% data fidelity)
                _buffer.Writer.TryWrite(dto);

                // 2. ONLY send to UI if counter hits 10 (10% data for preview)
                _liveSkipCounter++;
                if (_liveSkipCounter >= 10) 
                {
                    await _hubContext.Clients.All.SendAsync("ReceiveMeasurement", dto, stoppingToken);
                    _liveSkipCounter = 0;
                }
            }
        };

        // Povezava in naročanje
        try
        {
            await mqttClient.ConnectAsync(mqttClientOptions, stoppingToken);

            var topicFilter = _mqttFactory.CreateTopicFilterBuilder()
                .WithTopic(MQTT_TOPIC)
                .WithAtLeastOnceQoS()
                .Build();

            var subscribeOptions = _mqttFactory.CreateSubscribeOptionsBuilder()
                .WithTopicFilter(topicFilter)
                .Build();

            await mqttClient.SubscribeAsync(subscribeOptions, stoppingToken);
            _logger.LogInformation("MQTT Bridge: Povezan in naročen na 'telemetry/batch'");
        }
        catch (Exception ex)
        {
            _logger.LogCritical(ex, "MQTT Bridge: Ni se mogoče povezati na Mosquitto!");
        }

        // Glavna zanka, ki drži worker živ
        while (!stoppingToken.IsCancellationRequested)
        {
            if (!mqttClient.IsConnected)
            {
                _logger.LogWarning("MQTT Bridge: Povezava izgubljena, poskušam ponovno...");
                try { await mqttClient.ConnectAsync(mqttClientOptions, stoppingToken); } catch { }
            }
            await Task.Delay(5000, stoppingToken);
        }

        // Čist odklop
        await mqttClient.DisconnectAsync(new MqttClientDisconnectOptionsBuilder()
            .WithReason(MqttClientDisconnectOptionsReason.NormalDisconnection).Build());
    }
}