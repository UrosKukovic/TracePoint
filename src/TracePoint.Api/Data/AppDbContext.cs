using Microsoft.EntityFrameworkCore;

using TracePoint.Shared.Models;

namespace TracePoint.Api.Data;

public class AppDbContext : DbContext
{
    public AppDbContext(DbContextOptions<AppDbContext> options) : base(options) { }

    public DbSet<TelemetrySession> Sessions => Set<TelemetrySession>();
    public DbSet<Measurement> Measurements => Set<Measurement>();

    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        // Konfiguracija za Measurement (Composite Key za Hypertable podporo)
        modelBuilder.Entity<Measurement>()
            .HasKey(m => new { m.Time, m.Id });

        modelBuilder.Entity<Measurement>()
            .Property(m => m.Id)
            .ValueGeneratedOnAdd();

        // Relacija: Ena seja ima mnogo meritev
        modelBuilder.Entity<Measurement>()
            .HasOne(m => m.Session)
            .WithMany(s => s.Measurements)
            .HasForeignKey(m => m.SessionId);
    }
}