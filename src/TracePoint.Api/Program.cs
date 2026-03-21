using Microsoft.AspNetCore.SignalR;

using TracePoint.Api.Features.Telemetry;
using TracePoint.Shared;

var builder = WebApplication.CreateBuilder(args);

// Add services to the container.
// Learn more about configuring OpenAPI at https://aka.ms/aspnet/openapi
builder.Services.AddOpenApi();

// MediatR
builder.Services.AddMediatR(cfg => cfg.RegisterServicesFromAssembly(typeof(Program).Assembly));

builder.Services.AddSingleton<TelemetryBuffer>();

builder.Services.AddHostedService<DatabaseWorker>();

builder.Services.AddCors(options =>
{
    options.AddPolicy("AllowNextJs", policy =>
    {
        policy.WithOrigins("http://localhost:3000")
              .AllowAnyMethod()
              .AllowAnyHeader()
              .AllowCredentials();
    });
});

// SignalR
builder.Services.AddSignalR();


var app = builder.Build();

// Configure the HTTP request pipeline.
if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
}

app.UseCors("AllowNextJs");

app.UseHttpsRedirection();

// Register TelemetryHub
app.MapHub<TelemetryHub>("/telemetryHub");

// Endpoint
// High-performance batch ingestion endpoint
app.MapPost("/api/telemetry/ingest/batch", async (
    List<CanMeasurementDto> measurements, 
    IHubContext<TelemetryHub> hubContext, // Use IHubContext instead of the Hub class
    TelemetryBuffer buffer) =>            // Inject buffer directly for DB throughput
{
    foreach (var m in measurements)
    {
        // 1. Broadcast to SignalR clients (Next.js dashboard)
        await hubContext.Clients.All.SendAsync("ReceiveMeasurement", m);

        // 2. Queue for DatabaseWorker to pick up and save to TimescaleDB
        buffer.Writer.TryWrite(m);
    }

    return Results.Accepted();
})
.WithDescription("Receives a batch of CAN frames from the ESP32 Gateway.");

app.Run();