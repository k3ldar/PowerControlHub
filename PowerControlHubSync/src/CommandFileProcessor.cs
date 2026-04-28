using System.Text.RegularExpressions;

namespace SmartFuseSync;

/// <summary>
/// Helper class for processing command files with parameter substitution and comment handling
/// </summary>
internal static class CommandFileProcessor
{
    /// <summary>
    /// Reads a command file and processes lines, skipping comments and blank lines
    /// </summary>
    /// <param name="filePath">Path to the command file</param>
    /// <param name="parameters">Optional parameters for substitution (e.g., WIFI_SSID, WIFI_PASSWORD)</param>
    /// <returns>Array of processed command lines</returns>
    public static string[] ReadCommands(string filePath, Dictionary<string, string>? parameters = null)
    {
        if (!File.Exists(filePath))
            throw new FileNotFoundException($"Command file not found: {filePath}");

        var lines = File.ReadAllLines(filePath);
        var commands = new List<string>();

        foreach (var rawLine in lines)
        {
            var line = rawLine.Trim();

            // Skip blank lines and comments
            if (string.IsNullOrEmpty(line) || line.StartsWith('#'))
                continue;

            // Apply parameter substitution if provided
            if (parameters != null)
            {
                foreach (var kvp in parameters)
                {
                    line = line.Replace($"{{{{{kvp.Key}}}}}", kvp.Value);
                }
            }

            commands.Add(line);
        }

        return commands.ToArray();
    }

    /// <summary>
    /// Validates that all required parameters have been substituted
    /// </summary>
    /// <param name="command">Command to validate</param>
    /// <returns>True if valid, false if placeholders remain</returns>
    public static bool ValidateCommand(string command)
    {
        // Check if any {{PLACEHOLDER}} patterns remain
        return !Regex.IsMatch(command, @"\{\{[A-Z_]+\}\}");
    }

    /// <summary>
    /// Extracts the command name from a full command line
    /// </summary>
    /// <param name="command">Full command line (e.g., "M0:v=1")</param>
    /// <returns>Command name (e.g., "M0")</returns>
    public static string GetCommandName(string command)
    {
        int colonIndex = command.IndexOf(':');
        return colonIndex >= 0 ? command[..colonIndex] : command;
    }
}
