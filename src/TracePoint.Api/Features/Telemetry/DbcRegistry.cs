using DbcParserLib;
using DbcParserLib.Model;

namespace TracePoint.Api.Features.Telemetry;

public class DbcRegistry
{
    private readonly ILogger<DbcRegistry> _logger;
    private Dbc _currentDbc;
    
    // Thread-safe dictionary for quick ID lookups
    private Dictionary<uint, Message> _messages = new();

    public DbcRegistry(ILogger<DbcRegistry> logger)
    {
        _logger = logger;
    }

    public void LoadFromText(string dbcText)
    {
        try 
        {
            _currentDbc = Parser.Parse(dbcText);
            
            // Map messages by ID for O(1) lookup speed in the MQTT loop
            _messages = _currentDbc.Messages.ToDictionary(m => m.ID, m => m);
            
            _logger.LogInformation("DBC Registry: Loaded {Count} messages", _messages.Count);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to parse DBC file");
        }
    }

    public bool TryGetMessage(uint id, out Message message) 
        => _messages.TryGetValue(id, out message);
}