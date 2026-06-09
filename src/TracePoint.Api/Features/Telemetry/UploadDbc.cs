namespace TracePoint.Api.Features.Telemetry;

using Microsoft.AspNetCore.Mvc;

[ApiController]
[Route("api/telemetry")]
public class UploadDbcController : ControllerBase
{
    private readonly DbcRegistry _dbcRegistry;

    public UploadDbcController(DbcRegistry dbcRegistry)
    {
        _dbcRegistry = dbcRegistry;
    }

    [HttpPost("upload-dbc")]
    public async Task<IActionResult> Upload(IFormFile file)
    {
        if (file == null || file.Length == 0)
            return BadRequest("No file uploaded.");

        if (!file.FileName.EndsWith(".dbc"))
            return BadRequest("Only .dbc files are allowed.");

        using var stream = file.OpenReadStream();
        using var reader = new StreamReader(stream);
        var content = await reader.ReadToEndAsync();

        // Save to our Registry (we'll create this next)
        _dbcRegistry.LoadFromText(content);

        return Ok(new { message = "DBC loaded successfully", fileName = file.FileName });
    }
}