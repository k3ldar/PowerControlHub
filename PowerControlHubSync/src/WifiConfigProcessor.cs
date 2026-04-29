using CommandLinePlus;
using System.Net.Http;
using System.Text;
using System.Text.Json;

namespace SmartFuseSync;

[CmdLineDescription("Manages configuration updates for PowerControlHub devices via WiFi")]
internal class WifiConfigProcessor : BaseCommandLine, IDisposable
{
    private HttpClient? _httpClient;

    public override string Name => "WifiConfig";

    public override int SortOrder => 1;

    public override bool IsEnabled => true;

    public override void DisplayHelp()
    {
        Display.WriteLine(VerbosityLevel.Quiet, "WiFi configuration management commands for PowerControlHub devices");
        Display.WriteLine(VerbosityLevel.Quiet, "Usage: wificonfig <command> [options]");
    }

    public override int Execute(string[] args) => 0;

    [CmdLineDescription("Updates configuration from a file to PowerControlHub device via HTTP")]
    public int Update(
        [CmdLineAbbreviation("f", "Path to the configuration file")] string filePath,
        [CmdLineAbbreviation("i", "IP address of the PowerControlHub device")] string ipAddress,
        [CmdLineAbbreviation("p", "HTTP port")] int port = 80,
        [CmdLineAbbreviation("w", "WiFi SSID (replaces {{WIFI_SSID}} placeholder)")] string? wifiSsid = null,
        [CmdLineAbbreviation("x", "WiFi Password (replaces {{WIFI_PASSWORD}} placeholder)")] string? wifiPassword = null)
    {
        if (!IsEnabled)
            return -1;

        try
        {
            if (!File.Exists(filePath))
            {
                Display.WriteLine(VerbosityLevel.Quiet, $"Error: Configuration file not found: {filePath}");
                return -1;
            }

            Display.WriteLine(VerbosityLevel.Normal, $"Reading configuration from: {filePath}");

            // Build parameter dictionary for substitution
            var parameters = new Dictionary<string, string>();
            if (!string.IsNullOrEmpty(wifiSsid))
                parameters["WIFI_SSID"] = wifiSsid;
            if (!string.IsNullOrEmpty(wifiPassword))
                parameters["WIFI_PASSWORD"] = wifiPassword;

            // Read and process commands with parameter substitution
            string[] lines = CommandFileProcessor.ReadCommands(filePath, parameters);

            string baseUrl = $"http://{ipAddress}:{port}";

            Display.WriteLine(VerbosityLevel.Normal, $"Connecting to device at: {baseUrl}");

            _httpClient = new HttpClient { Timeout = TimeSpan.FromSeconds(10) };

            foreach (string line in lines)
            {
                // Validate that all placeholders have been replaced
                if (!CommandFileProcessor.ValidateCommand(line))
                {
                    Display.WriteLine(VerbosityLevel.Quiet, $"Error: Command contains unreplaced placeholder: {line}");
                    Display.WriteLine(VerbosityLevel.Quiet, "Hint: Use -w for WiFi SSID and -x for WiFi Password");
                    return -1;
                }

                // Extract command name and body
                int colonIndex = line.IndexOf(':');
                string commandName = colonIndex >= 0 ? line[..colonIndex] : line;
                string body = colonIndex >= 0 ? line[(colonIndex + 1)..] : string.Empty;

                Display.WriteLine(VerbosityLevel.Normal, $"Sending {commandName}");

                string url = $"{baseUrl}/api/command/{commandName}";
                using StringContent content = new(body, Encoding.UTF8, "text/plain");
                HttpResponseMessage httpResponse = _httpClient.PostAsync(url, content).GetAwaiter().GetResult();
                string responseJson = httpResponse.Content.ReadAsStringAsync().GetAwaiter().GetResult();

                bool success = false;
                string errorMessage = string.Empty;

                try
                {
                    using JsonDocument doc = JsonDocument.Parse(responseJson);
                    JsonElement root = doc.RootElement;
                    success = root.TryGetProperty("success", out JsonElement successProp) && successProp.GetBoolean();
                    if (!success)
                        errorMessage = root.TryGetProperty("error", out JsonElement errorProp)
                            ? errorProp.GetString() ?? "unknown error"
                            : "unknown error";
                }
                catch (JsonException)
                {
                    Display.WriteLine(VerbosityLevel.Quiet, $"Warning: Could not parse response for {commandName}: {responseJson}");
                    continue;
                }

                if (success)
                {
                    Display.WriteLine(VerbosityLevel.Normal, $"ACK:{commandName} received");
                }
                else
                {
                    Display.WriteLine(VerbosityLevel.Normal, $"ERR:{commandName} received");
                    Display.WriteLine(VerbosityLevel.Quiet, $"Warning: Device reported error for {commandName}: {errorMessage}");
                }
            }

            Display.WriteLine(VerbosityLevel.Quiet, "Configuration update completed successfully");
            return 0;
        }
        catch (HttpRequestException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: HTTP communication error: {ex.Message}");
            return -1;
        }
        catch (TaskCanceledException)
        {
            Display.WriteLine(VerbosityLevel.Quiet, "Error: Request timed out");
            return -1;
        }
        catch (Exception ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: {ex.Message}");
            return -1;
        }
        finally
        {
            _httpClient?.Dispose();
            _httpClient = null;
        }
    }

    [CmdLineDescription("Queries current OTA firmware status from PowerControlHub device (F13)")]
    public int OtaStatus(
        [CmdLineAbbreviation("i", "IP address of the PowerControlHub device")] string ipAddress,
        [CmdLineAbbreviation("p", "HTTP port")] int port = 80)
    {
        if (!IsEnabled)
            return -1;

        string baseUrl = $"http://{ipAddress}:{port}";
        Display.WriteLine(VerbosityLevel.Normal, $"Querying OTA status from {baseUrl}...");

        try
        {
            string responseJson = PostCommand(baseUrl, "F13", string.Empty);

            using JsonDocument doc = JsonDocument.Parse(responseJson);
            JsonElement root = doc.RootElement;
            bool success = root.TryGetProperty("success", out JsonElement successProp) && successProp.GetBoolean();

            if (success)
            {
                string version   = root.TryGetProperty("v",    out JsonElement vProp)    ? vProp.GetString()    ?? "unknown" : "unknown";
                string available = root.TryGetProperty("av",   out JsonElement avProp)   ? avProp.GetString()   ?? ""        : "";
                string state     = root.TryGetProperty("s",    out JsonElement sProp)    ? sProp.GetString()    ?? "unknown" : "unknown";
                string auto      = root.TryGetProperty("auto", out JsonElement autoProp) ? autoProp.GetString() ?? "0"       : "0";

                Display.WriteLine(VerbosityLevel.Quiet, $"Current version : {version}");
                Display.WriteLine(VerbosityLevel.Quiet, $"Available version: {(string.IsNullOrEmpty(available) ? "none" : available)}");
                Display.WriteLine(VerbosityLevel.Quiet, $"OTA state        : {state}");
                Display.WriteLine(VerbosityLevel.Quiet, $"Auto-apply       : {(auto == "1" ? "enabled" : "disabled")}");
                return 0;
            }
            else
            {
                string error = root.TryGetProperty("error", out JsonElement errorProp) ? errorProp.GetString() ?? "unknown error" : "unknown error";
                Display.WriteLine(VerbosityLevel.Quiet, $"Error: {error}");
                return -1;
            }
        }
        catch (HttpRequestException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: HTTP communication error: {ex.Message}");
            return -1;
        }
        catch (TaskCanceledException)
        {
            Display.WriteLine(VerbosityLevel.Quiet, "Error: Request timed out");
            return -1;
        }
        catch (JsonException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: Could not parse response: {ex.Message}");
            return -1;
        }
    }

    [CmdLineDescription("Checks or applies an OTA firmware update on PowerControlHub device (F12)")]
    public int OtaUpdate(
        [CmdLineAbbreviation("i", "IP address of the PowerControlHub device")] string ipAddress,
        [CmdLineAbbreviation("a", "Apply the update if one is available")] bool apply = false,
        [CmdLineAbbreviation("p", "HTTP port")] int port = 80)
    {
        if (!IsEnabled)
            return -1;

        string baseUrl = $"http://{ipAddress}:{port}";
        string body = apply ? "apply=1" : string.Empty;

        Display.WriteLine(VerbosityLevel.Normal, apply
            ? $"Checking and applying OTA update on {baseUrl}..."
            : $"Checking for OTA update on {baseUrl}...");

        try
        {
            // Use a longer timeout — downloading firmware can take time
            string responseJson = PostCommand(baseUrl, "F12", body, timeoutSeconds: 60);

            using JsonDocument doc = JsonDocument.Parse(responseJson);
            JsonElement root = doc.RootElement;
            bool success = root.TryGetProperty("success", out JsonElement successProp) && successProp.GetBoolean();

            if (success)
            {
                string version   = root.TryGetProperty("v",  out JsonElement vProp)  ? vProp.GetString()  ?? "unknown" : "unknown";
                string available = root.TryGetProperty("av", out JsonElement avProp) ? avProp.GetString() ?? ""        : "";
                string state     = root.TryGetProperty("s",  out JsonElement sProp)  ? sProp.GetString()  ?? "unknown" : "unknown";

                Display.WriteLine(VerbosityLevel.Quiet, $"Current version  : {version}");
                Display.WriteLine(VerbosityLevel.Quiet, $"Available version: {(string.IsNullOrEmpty(available) ? "none" : available)}");
                Display.WriteLine(VerbosityLevel.Quiet, $"OTA state        : {state}");
                return 0;
            }
            else
            {
                string error = root.TryGetProperty("error", out JsonElement errorProp) ? errorProp.GetString() ?? "unknown error" : "unknown error";
                Display.WriteLine(VerbosityLevel.Quiet, $"Error: {error}");
                return -1;
            }
        }
        catch (HttpRequestException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: HTTP communication error: {ex.Message}");
            return -1;
        }
        catch (TaskCanceledException)
        {
            Display.WriteLine(VerbosityLevel.Quiet, "Error: Request timed out");
            return -1;
        }
        catch (JsonException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: Could not parse response: {ex.Message}");
            return -1;
        }
    }

    private string PostCommand(string baseUrl, string commandName, string body, int timeoutSeconds = 10)
    {
        using HttpClient client = new() { Timeout = TimeSpan.FromSeconds(timeoutSeconds) };
        string url = $"{baseUrl}/api/command/{commandName}";
        using StringContent content = new(body, Encoding.UTF8, "text/plain");
        HttpResponseMessage httpResponse = client.PostAsync(url, content).GetAwaiter().GetResult();
        return httpResponse.Content.ReadAsStringAsync().GetAwaiter().GetResult();
    }

    [CmdLineDescription("Tests connection to a PowerControlHub device")]
    public int Test(
        [CmdLineAbbreviation("i", "IP address of the PowerControlHub device")] string ipAddress,
        [CmdLineAbbreviation("p", "HTTP port")] int port = 80)
    {
        if (!IsEnabled)
            return -1;

        string baseUrl = $"http://{ipAddress}:{port}";
        Display.WriteLine(VerbosityLevel.Normal, $"Testing connection to {baseUrl}...");

        try
        {
            using HttpClient client = new() { Timeout = TimeSpan.FromSeconds(5) };
            HttpResponseMessage response = client.GetAsync(baseUrl).GetAwaiter().GetResult();

            if (response.IsSuccessStatusCode)
            {
                Display.WriteLine(VerbosityLevel.Quiet, $"Success: Connected to {baseUrl}");
                return 0;
            }
            else
            {
                Display.WriteLine(VerbosityLevel.Quiet, $"Error: Device responded with HTTP {(int)response.StatusCode}");
                return -1;
            }
        }
        catch (HttpRequestException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: {ex.Message}");
            return -1;
        }
        catch (TaskCanceledException)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: Connection timed out");
            return -1;
        }
    }

    [CmdLineDescription("Runs test commands from a test file and validates responses via WiFi")]
    public int RunTests(
        [CmdLineAbbreviation("f", "Path to the test file")] string filePath,
        [CmdLineAbbreviation("i", "IP address of the PowerControlHub device")] string ipAddress,
        [CmdLineAbbreviation("p", "HTTP port")] int port = 80)
    {
        if (!IsEnabled)
            return -1;

        try
        {
            Display.WriteLine(VerbosityLevel.Normal, $"Loading test commands from: {filePath}");

            // Check if this is a WiFi-format test file (METHOD /endpoint format)
            var firstLine = File.ReadLines(filePath).FirstOrDefault(l => !string.IsNullOrWhiteSpace(l) && !l.TrimStart().StartsWith('#'));
            bool isWifiFormat = firstLine != null && (firstLine.TrimStart().StartsWith("GET ") || firstLine.TrimStart().StartsWith("POST "));

            if (isWifiFormat)
            {
                return RunWifiFormatTests(filePath, ipAddress, port);
            }
            else
            {
                return RunSerialFormatTests(filePath, ipAddress, port);
            }
        }
        catch (FileNotFoundException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: {ex.Message}");
            return -1;
        }
        catch (Exception ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: {ex.Message}");
            return -1;
        }
    }

    private int RunWifiFormatTests(string filePath, string ipAddress, int port)
    {
        try
        {
            var tests = WifiTestCommandParser.ParseTestFile(filePath);
            Display.WriteLine(VerbosityLevel.Quiet, $"Loaded {tests.Count} WiFi test cases");

            string baseUrl = $"http://{ipAddress}:{port}";
            Display.WriteLine(VerbosityLevel.Normal, $"Testing device at: {baseUrl}");

            _httpClient = new HttpClient { Timeout = TimeSpan.FromSeconds(10) };

            var results = new List<WifiTestResult>();
            int passed = 0;
            int failed = 0;

            foreach (var test in tests)
            {
                var result = new WifiTestResult
                {
                    Method = test.Method,
                    Endpoint = test.Endpoint,
                    Body = test.Body,
                    ExpectedResponses = test.ExpectedResponses
                };

                try
                {
                    Display.WriteLine(VerbosityLevel.Normal, $"Testing: {test.Method} {test.Endpoint} {test.Body}");

                    string url = $"{baseUrl}{test.Endpoint}";
                    HttpResponseMessage httpResponse;

                    if (test.Method == "GET")
                    {
                        httpResponse = _httpClient.GetAsync(url).GetAwaiter().GetResult();
                    }
                    else if (test.Method == "POST")
                    {
                        using StringContent content = new(test.Body, Encoding.UTF8, "text/plain");
                        httpResponse = _httpClient.PostAsync(url, content).GetAwaiter().GetResult();
                    }
                    else
                    {
                        result.Passed = false;
                        result.ErrorMessage = $"Unsupported HTTP method: {test.Method}";
                        failed++;
                        Display.WriteLine(VerbosityLevel.Quiet, $"  ✗ ERROR: {result.ErrorMessage}");
                        results.Add(result);
                        continue;
                    }

                    string responseJson = httpResponse.Content.ReadAsStringAsync().GetAwaiter().GetResult();
                    result.ActualResponse = responseJson;
                    result.Passed = test.IsMatch(responseJson);

                    if (result.Passed)
                    {
                        passed++;
                        Display.WriteLine(VerbosityLevel.Normal, $"  ✓ PASSED");
                        Display.WriteLine(VerbosityLevel.Full, $"    Response: {responseJson}");
                    }
                    else
                    {
                        failed++;
                        Display.WriteLine(VerbosityLevel.Quiet, $"  ✗ FAILED");
                        Display.WriteLine(VerbosityLevel.Quiet, $"    Expected: {string.Join(" | ", test.ExpectedResponses)}");
                        Display.WriteLine(VerbosityLevel.Quiet, $"    Got: {responseJson}");
                    }
                }
                catch (Exception ex)
                {
                    result.Passed = false;
                    result.ErrorMessage = ex.Message;
                    failed++;
                    Display.WriteLine(VerbosityLevel.Quiet, $"  ✗ ERROR: {ex.Message}");
                }

                results.Add(result);
                Thread.Sleep(100);
            }

            // Summary
            Display.WriteLine(VerbosityLevel.Quiet, "");
            Display.WriteLine(VerbosityLevel.Quiet, "=== WiFi Test Summary ===");
            Display.WriteLine(VerbosityLevel.Quiet, $"Total: {tests.Count}");
            Display.WriteLine(VerbosityLevel.Quiet, $"Passed: {passed}");
            Display.WriteLine(VerbosityLevel.Quiet, $"Failed: {failed}");
            Display.WriteLine(VerbosityLevel.Quiet, $"Success Rate: {(tests.Count > 0 ? (passed * 100.0 / tests.Count) : 0):F1}%");

            return failed == 0 ? 0 : -1;
        }
        finally
        {
            _httpClient?.Dispose();
            _httpClient = null;
        }
    }

    private int RunSerialFormatTests(string filePath, string ipAddress, int port)
    {
        try
        {
            var tests = TestCommandParser.ParseTestFile(filePath);
            Display.WriteLine(VerbosityLevel.Quiet, $"Loaded {tests.Count} test cases (serial format over WiFi)");

            string baseUrl = $"http://{ipAddress}:{port}";
            Display.WriteLine(VerbosityLevel.Normal, $"Testing device at: {baseUrl}");

            _httpClient = new HttpClient { Timeout = TimeSpan.FromSeconds(10) };

            // Run tests
            var results = new List<TestResult>();
            int passed = 0;
            int failed = 0;

            foreach (var test in tests)
            {
                var result = new TestResult
                {
                    Command = test.Command,
                    ExpectedResponses = test.ExpectedResponses
                };

                try
                {
                    Display.WriteLine(VerbosityLevel.Normal, $"Testing: {test.Command}");

                    // Extract command name and body
                    int colonIndex = test.Command.IndexOf(':');
                    string commandName = colonIndex >= 0 ? test.Command[..colonIndex] : test.Command;
                    string body = colonIndex >= 0 ? test.Command[(colonIndex + 1)..] : string.Empty;

                    // Send command
                    string url = $"{baseUrl}/api/command/{commandName}";
                    using StringContent content = new(body, Encoding.UTF8, "text/plain");
                    HttpResponseMessage httpResponse = _httpClient.PostAsync(url, content).GetAwaiter().GetResult();
                    string responseJson = httpResponse.Content.ReadAsStringAsync().GetAwaiter().GetResult();

                    // Parse response
                    string actualResponse;
                    try
                    {
                        using JsonDocument doc = JsonDocument.Parse(responseJson);
                        JsonElement root = doc.RootElement;
                        bool success = root.TryGetProperty("success", out JsonElement successProp) && successProp.GetBoolean();

                        if (success)
                        {
                            // Build response in ACK format for consistency
                            actualResponse = $"ACK:{commandName}=ok";

                            // Check if there are response values
                            if (root.TryGetProperty("v", out JsonElement vProp))
                            {
                                actualResponse = $"{commandName}:v={vProp}";
                            }
                        }
                        else
                        {
                            string error = root.TryGetProperty("error", out JsonElement errorProp) 
                                ? errorProp.GetString() ?? "unknown error" 
                                : "unknown error";
                            actualResponse = $"ACK:{commandName}={error}";
                        }
                    }
                    catch (JsonException)
                    {
                        actualResponse = responseJson;
                    }

                    result.ActualResponse = actualResponse;
                    result.Passed = test.IsMatch(actualResponse);

                    if (result.Passed)
                    {
                        passed++;
                        Display.WriteLine(VerbosityLevel.Normal, $"  ✓ PASSED: {actualResponse}");
                    }
                    else
                    {
                        failed++;
                        Display.WriteLine(VerbosityLevel.Quiet, $"  ✗ FAILED: Expected [{string.Join(" | ", test.ExpectedResponses)}], Got [{actualResponse}]");
                    }
                }
                catch (Exception ex)
                {
                    result.Passed = false;
                    result.ErrorMessage = ex.Message;
                    failed++;
                    Display.WriteLine(VerbosityLevel.Quiet, $"  ✗ ERROR: {ex.Message}");
                }

                results.Add(result);

                // Small delay between tests
                Thread.Sleep(100);
            }

            // Summary
            Display.WriteLine(VerbosityLevel.Quiet, "");
            Display.WriteLine(VerbosityLevel.Quiet, "=== Test Summary ===");
            Display.WriteLine(VerbosityLevel.Quiet, $"Total: {tests.Count}");
            Display.WriteLine(VerbosityLevel.Quiet, $"Passed: {passed}");
            Display.WriteLine(VerbosityLevel.Quiet, $"Failed: {failed}");
            Display.WriteLine(VerbosityLevel.Quiet, $"Success Rate: {(tests.Count > 0 ? (passed * 100.0 / tests.Count) : 0):F1}%");

            return failed == 0 ? 0 : -1;
        }
        catch (FileNotFoundException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: {ex.Message}");
            return -1;
        }
        catch (HttpRequestException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: HTTP communication error: {ex.Message}");
            return -1;
        }
        catch (TaskCanceledException)
        {
            Display.WriteLine(VerbosityLevel.Quiet, "Error: Request timed out");
            return -1;
        }
        catch (Exception ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: {ex.Message}");
            return -1;
        }
        finally
        {
            _httpClient?.Dispose();
            _httpClient = null;
        }
    }

    public void Dispose()
    {
        _httpClient?.Dispose();
        GC.SuppressFinalize(this);
    }
}

