using Microsoft.AspNetCore.SignalR.Client;
using TracePoint.Shared;

Console.WriteLine("=== TracePoint Hardware Simulator ===");

var connection = new HubConnectionBuilder()
    .WithUrl("http://localhost:5247/telemetryHub") // Adjust the port if needed — see Api/Properties/launchSettings.json
    .WithAutomaticReconnect()
    .Build();

try
{
    await connection.StartAsync();
    Console.WriteLine("[Simulator] Connected to SignalR Hub.");

    var random = new Random();

    while (true)
    {
        var mockMeasurement = new CanMeasurementDto
        {
            TimestampMs = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(),
            CanId = (uint)random.Next(0, 0x7FF),
            Value = Math.Round(random.NextDouble() * 100, 2),
            Channel = "SIM_VOLTAGE",
            Data = new byte[] { 0x01, 0x02, (byte)random.Next(0, 255) }
        };

        await connection.SendAsync("SendMeasurement", mockMeasurement);
        
        Console.WriteLine($"[Sent]: ID: 0x{mockMeasurement.CanId:X} | Value: {mockMeasurement.Value}");
        
        await Task.Delay(50);
    }
}
catch (Exception ex)
{
    Console.WriteLine($"[Error]: {ex.Message}");
}