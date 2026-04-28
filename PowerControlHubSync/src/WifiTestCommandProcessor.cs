namespace SmartFuseSync;

/// <summary>
/// Represents a WiFi HTTP test case with method, endpoint, body, and expected JSON response
/// </summary>
internal class WifiTestCommand
{
    public string Method { get; set; } = "GET";
    public string Endpoint { get; set; } = string.Empty;
    public string Body { get; set; } = string.Empty;
    public string[] ExpectedResponses { get; set; } = Array.Empty<string>();

    public bool IsMatch(string actualJsonResponse)
    {
        foreach (var expected in ExpectedResponses)
        {
            if (MatchesJsonPattern(actualJsonResponse, expected))
                return true;
        }
        return false;
    }

    private static bool MatchesJsonPattern(string actualJson, string pattern)
    {
        // Handle wildcard matching for JSON
        if (pattern.Contains('*'))
        {
            // Simple wildcard matching - just check if the pattern structure exists
            // Remove wildcards and check if remaining structure matches
            string regexPattern = "^" + System.Text.RegularExpressions.Regex.Escape(pattern)
                .Replace("\\*", ".*")
                .Replace("\\{", "\\{")
                .Replace("\\}", "\\}") + "$";

            return System.Text.RegularExpressions.Regex.IsMatch(actualJson.Trim(), regexPattern, 
                System.Text.RegularExpressions.RegexOptions.Singleline);
        }

        // Check if pattern is contained in actual (for partial matching)
        if (actualJson.Contains(pattern.Trim()))
            return true;

        // Exact match
        return actualJson.Trim().Equals(pattern.Trim(), StringComparison.OrdinalIgnoreCase);
    }
}

/// <summary>
/// Parser for WiFi HTTP test command files
/// </summary>
internal static class WifiTestCommandParser
{
    /// <summary>
    /// Parses a WiFi test command file into WifiTestCommand objects
    /// Format: METHOD /endpoint [body] -> JSON_PATTERN | ALTERNATE_JSON_PATTERN
    /// </summary>
    public static List<WifiTestCommand> ParseTestFile(string filePath)
    {
        if (!File.Exists(filePath))
            throw new FileNotFoundException($"WiFi test file not found: {filePath}");

        var tests = new List<WifiTestCommand>();
        var lines = File.ReadAllLines(filePath);

        foreach (var rawLine in lines)
        {
            var line = rawLine.Trim();

            // Skip blank lines and comments
            if (string.IsNullOrEmpty(line) || line.StartsWith('#'))
                continue;

            // Parse format: METHOD /endpoint [body] -> EXPECTED_JSON_RESPONSE
            var parts = line.Split(new[] { "->" }, StringSplitOptions.TrimEntries);
            if (parts.Length != 2)
            {
                Console.WriteLine($"Warning: Skipping invalid WiFi test line: {line}");
                continue;
            }

            // Parse left side: METHOD /endpoint [body]
            var leftSide = parts[0].Trim();
            var leftParts = leftSide.Split(new[] { ' ' }, 3, StringSplitOptions.RemoveEmptyEntries);

            if (leftParts.Length < 2)
            {
                Console.WriteLine($"Warning: Invalid format (need METHOD /endpoint): {line}");
                continue;
            }

            string method = leftParts[0].ToUpper();
            string endpoint = leftParts[1];
            string body = leftParts.Length > 2 ? leftParts[2] : string.Empty;

            // Parse right side: JSON responses
            var responses = parts[1].Split('|', StringSplitOptions.TrimEntries)
                                    .Select(r => r.Trim())
                                    .ToArray();

            tests.Add(new WifiTestCommand
            {
                Method = method,
                Endpoint = endpoint,
                Body = body,
                ExpectedResponses = responses
            });
        }

        return tests;
    }
}

/// <summary>
/// Result of a WiFi test execution
/// </summary>
internal class WifiTestResult
{
    public string Method { get; set; } = string.Empty;
    public string Endpoint { get; set; } = string.Empty;
    public string Body { get; set; } = string.Empty;
    public string ActualResponse { get; set; } = string.Empty;
    public string[] ExpectedResponses { get; set; } = Array.Empty<string>();
    public bool Passed { get; set; }
    public string? ErrorMessage { get; set; }
}
