using Microsoft.AspNetCore.SignalR;
using Microsoft.EntityFrameworkCore;

using TracePoint.Api.Features.Telemetry;
using TracePoint.Api.Data;
using TracePoint.Shared;

var builder = WebApplication.CreateBuilder(args);

// Add services to the container.
// Learn more about configuring OpenAPI at https://aka.ms/aspnet/openapi
builder.Services.AddOpenApi();

// MediatR
builder.Services.AddMediatR(cfg => cfg.RegisterServicesFromAssembly(typeof(Program).Assembly));

builder.Services.AddSingleton<TelemetryBuffer>();

builder.Services.AddHostedService<DatabaseWorker>();

// MQTT
builder.Services.AddHostedService<MqttBridgeWorker>();

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

// DB
builder.Services.AddDbContext<AppDbContext>(options =>
    options.UseNpgsql(builder.Configuration.GetConnectionString("DefaultConnection")));

// Controllers
builder.Services.AddControllers();

// DBC
builder.Services.AddSingleton<DbcRegistry>();

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

app.MapControllers();

app.Run();