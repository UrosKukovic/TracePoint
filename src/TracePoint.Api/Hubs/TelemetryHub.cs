using Microsoft.AspNetCore.SignalR;
using TracePoint.Shared;

namespace TracePoint.Api.Hubs;

public class TelemetryHub : Hub
{
    // This method will be called by ESP32
    public async Task SendMeasurement(CanMeasurementDto measurement)
    {
        // broadcast to blazor
        await Clients.All.SendAsync("ReceiveMeasurement", measurement);
    }
    
    // Connection logging for debug
    public override async Task OnConnectedAsync()
    {
        Console.WriteLine($"[Hub]: Device connected: {Context.ConnectionId}");
        await base.OnConnectedAsync();
    }
}