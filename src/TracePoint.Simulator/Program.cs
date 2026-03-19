using Microsoft.AspNetCore.SignalR.Client;
using TracePoint.Shared;

Console.WriteLine("=== TracePoint Hardware Simulator ===");

var connection = new HubConnectionBuilder()
    .WithUrl("http://localhost:5247/telemetryHub") // Prilagodi port (poglej v launchSettings.json od Api-ja)
    .WithAutomaticReconnect()
    .Build();

try 
{
    await connection.StartAsync();
    Console.WriteLine("[Simulator] Povezan na SignalR Hub.");

    var random = new Random();

    // 2. Simulacijska zanka
    while (true)
    {
        var mockMeasurement = new CanMeasurementDto
        {
            TimestampMs = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(),
            CanId = (uint)random.Next(0, 0x7FF), // Standardni CAN ID
            Value = Math.Round(random.NextDouble() * 100, 2), // Simulacija npr. napetosti
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