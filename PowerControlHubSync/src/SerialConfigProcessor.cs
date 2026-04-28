using CommandLinePlus;
using System.IO.Ports;

namespace SmartFuseSync;

[CmdLineDescription("Manages configuration updates for SmartFuseBox devices via COM PORT")]
internal class SerialConfigProcessor : BaseCommandLine, IDisposable
{
    private SerialPort? _serialPort;

    public override string Name => "Config";

    public override int SortOrder => 0;

    public override bool IsEnabled => true;

    public override void DisplayHelp()
    {
        Display.WriteLine(VerbosityLevel.Quiet, "Configuration management commands for SmartFuseBox devices");
        Display.WriteLine(VerbosityLevel.Quiet, "Usage: config <command> [options]");
    }

    public override int Execute(string[] args)
    {
        return 0;
    }

    [CmdLineDescription("Updates configuration from a file to SmartFuseBox device")]
    public int Update(
        [CmdLineAbbreviation("f", "Path to the configuration file")] string filePath,
        [CmdLineAbbreviation("p", "COM port name (e.g., COM3)")] string portName,
        [CmdLineAbbreviation("b", "Baud rate for COM port connection")] int baudRate = 9600,
        [CmdLineAbbreviation("w", "WiFi SSID (replaces {{WIFI_SSID}} placeholder)")] string? wifiSsid = null,
        [CmdLineAbbreviation("x", "WiFi Password (replaces {{WIFI_PASSWORD}} placeholder)")] string? wifiPassword = null)
    {
        if (!IsEnabled)
            return -1;

        try
        {
            // Validate file exists
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

            // Initialize COM port
            Display.WriteLine(VerbosityLevel.Normal, $"Opening COM port: {portName} at {baudRate} baud");
            _serialPort = new SerialPort(portName, baudRate)
            {
                Parity = Parity.None,
                DataBits = 8,
                StopBits = StopBits.One,
                Handshake = Handshake.None,
                ReadTimeout = 1000,
                WriteTimeout = 5000
            };

            _serialPort.Open();

            if (!_serialPort.IsOpen)
            {
                Display.WriteLine(VerbosityLevel.Quiet, $"Error: Failed to open COM port: {portName}");
                return -1;
            }

            // Send configuration data line-by-line and wait for ACK for each non-blank line
            foreach (string line in lines)
            {
                // Validate that all placeholders have been replaced
                if (!CommandFileProcessor.ValidateCommand(line))
                {
                    Display.WriteLine(VerbosityLevel.Quiet, $"Error: Command contains unreplaced placeholder: {line}");
                    Display.WriteLine(VerbosityLevel.Quiet, "Hint: Use -w for WiFi SSID and -x for WiFi Password");
                    return -1;
                }

                // Extract command name for display and ACK matching
                string commandName = CommandFileProcessor.GetCommandName(line);

                Display.WriteLine(VerbosityLevel.Normal, $"Sending {commandName}");

                // Send the full line to the device
                _serialPort.WriteLine(line);

                // Loop reading lines until we find the ACK or ERR for this specific command.
                // Other lines (telemetry, status) are logged at Verbose and discarded.
                string? ackResponse = null;
                DateTime deadline = DateTime.UtcNow.AddSeconds(5);

                while (DateTime.UtcNow < deadline)
                {
                    string response;
                    try
                    {
                        response = _serialPort.ReadLine().Trim();
                    }
                    catch (TimeoutException)
                    {
                        continue;
                    }

                    if (response.Contains($"ACK:{commandName}") || response.Contains($"ERR:{commandName}"))
                    {
                        ackResponse = response;
                        break;
                    }

                    Display.WriteLine(VerbosityLevel.Full, $"  [{commandName}] {response}");
                }

                if (ackResponse is null)
                {
                    Display.WriteLine(VerbosityLevel.Quiet, $"Error: Timeout waiting for ACK from {commandName}");
                    return -1;
                }

                Display.WriteLine(VerbosityLevel.Normal, $"{ackResponse} received");

                if (ackResponse.Contains($"ERR:{commandName}"))
                    Display.WriteLine(VerbosityLevel.Quiet, $"Warning: Device reported error for {commandName}: {ackResponse}");
            }

            Display.WriteLine(VerbosityLevel.Quiet, "Configuration update completed successfully");
            return 0;
        }
        catch (UnauthorizedAccessException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: Access denied to COM port {portName}. {ex.Message}");
            return -1;
        }
        catch (IOException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: Communication error: {ex.Message}");
            return -1;
        }
        catch (Exception ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: {ex.Message}");
            return -1;
        }
        finally
        {
            ClosePort();
        }
    }

    [CmdLineDescription("Lists available COM ports on the system")]
    public void ListPorts()
    {
        if (!IsEnabled)
            return;

        Display.WriteLine(VerbosityLevel.Quiet, "Available COM ports:");
        string[] ports = SerialPort.GetPortNames();

        if (ports.Length == 0)
        {
            Display.WriteLine(VerbosityLevel.Quiet, "  No COM ports found");
        }
        else
        {
            foreach (string port in ports)
            {
                Display.WriteLine(VerbosityLevel.Quiet, $"  {port}");
            }
        }
    }

    [CmdLineDescription("Tests connection to a SmartFuseBox device")]
    public int Test(
        [CmdLineAbbreviation("p", "COM port name (e.g., COM3)")] string portName,
        [CmdLineAbbreviation("b", "Baud rate for COM port connection")] int baudRate = 9600)
    {
        if (!IsEnabled)
            return -1;

        try
        {
            Display.WriteLine(VerbosityLevel.Normal, $"Testing connection to {portName} at {baudRate} baud...");

            _serialPort = new SerialPort(portName, baudRate)
            {
                Parity = Parity.None,
                DataBits = 8,
                StopBits = StopBits.One,
                Handshake = Handshake.None,
                ReadTimeout = 3000,
                WriteTimeout = 3000
            };

            _serialPort.Open();

            if (_serialPort.IsOpen)
            {
                Display.WriteLine(VerbosityLevel.Quiet, $"Success: Connected to {portName}");
                return 0;
            }
            else
            {
                Display.WriteLine(VerbosityLevel.Quiet, $"Error: Failed to connect to {portName}");
                return -1;
            }
        }
        catch (Exception ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: {ex.Message}");
            return -1;
        }
        finally
        {
            ClosePort();
        }
    }

    [CmdLineDescription("Reads current configuration from SmartFuseBox device")]
    public int Read(
        [CmdLineAbbreviation("p", "COM port name (e.g., COM3)")] string portName,
        [CmdLineAbbreviation("o", "Output file path to save configuration")] string? outputPath = null,
        [CmdLineAbbreviation("b", "Baud rate for COM port connection")] int baudRate = 9600)
    {
        if (!IsEnabled)
            return -1;

        try
        {
            Display.WriteLine(VerbosityLevel.Normal, $"Reading configuration from device on {portName}...");

            _serialPort = new SerialPort(portName, baudRate)
            {
                Parity = Parity.None,
                DataBits = 8,
                StopBits = StopBits.One,
                Handshake = Handshake.None,
                ReadTimeout = 5000,
                WriteTimeout = 5000
            };

            _serialPort.Open();

            if (!_serialPort.IsOpen)
            {
                Display.WriteLine(VerbosityLevel.Quiet, $"Error: Failed to open COM port: {portName}");
                return -1;
            }

            // Request configuration from device
            _serialPort.WriteLine("READ_CONFIG");
            string configData = _serialPort.ReadLine();

            Display.WriteLine(VerbosityLevel.Normal, "Configuration data received:");
            Display.WriteLine(VerbosityLevel.Normal, configData);

            // Save to file if output path specified
            if (!string.IsNullOrEmpty(outputPath))
            {
                File.WriteAllText(outputPath, configData);
                Display.WriteLine(VerbosityLevel.Quiet, $"Configuration saved to: {outputPath}");
            }

            return 0;
        }
        catch (Exception ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: {ex.Message}");
            return -1;
        }
        finally
        {
            ClosePort();
        }
    }

    [CmdLineDescription("Runs test commands from a test file and validates responses")]
    public int RunTests(
        [CmdLineAbbreviation("f", "Path to the test file")] string filePath,
        [CmdLineAbbreviation("p", "COM port name (e.g., COM3)")] string portName,
        [CmdLineAbbreviation("b", "Baud rate for COM port connection")] int baudRate = 9600)
    {
        if (!IsEnabled)
            return -1;

        try
        {
            Display.WriteLine(VerbosityLevel.Normal, $"Loading test commands from: {filePath}");

            var tests = TestCommandParser.ParseTestFile(filePath);
            Display.WriteLine(VerbosityLevel.Quiet, $"Loaded {tests.Count} test cases");

            // Initialize COM port
            Display.WriteLine(VerbosityLevel.Normal, $"Opening COM port: {portName} at {baudRate} baud");
            _serialPort = new SerialPort(portName, baudRate)
            {
                Parity = Parity.None,
                DataBits = 8,
                StopBits = StopBits.One,
                Handshake = Handshake.None,
                ReadTimeout = 1000,
                WriteTimeout = 5000
            };

            _serialPort.Open();

            if (!_serialPort.IsOpen)
            {
                Display.WriteLine(VerbosityLevel.Quiet, $"Error: Failed to open COM port: {portName}");
                return -1;
            }

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
                    string commandName = CommandFileProcessor.GetCommandName(test.Command);
                    Display.WriteLine(VerbosityLevel.Normal, $"Testing: {test.Command}");

                    // Send command
                    _serialPort.WriteLine(test.Command);

                    // Wait for response
                    string? response = null;
                    DateTime deadline = DateTime.UtcNow.AddSeconds(5);

                    while (DateTime.UtcNow < deadline)
                    {
                        try
                        {
                            string line = _serialPort.ReadLine().Trim();

                            // Look for ACK or actual response
                            if (line.Contains($"ACK:{commandName}") || 
                                line.Contains($"{commandName}:") ||
                                line.StartsWith(commandName))
                            {
                                // Extract just the ACK/command part, removing any debug prefixes
                                int ackIndex = line.IndexOf("ACK:");
                                int cmdIndex = line.IndexOf($"{commandName}:");

                                if (ackIndex >= 0)
                                {
                                    response = line.Substring(ackIndex);
                                }
                                else if (cmdIndex >= 0)
                                {
                                    response = line.Substring(cmdIndex);
                                }
                                else if (line.StartsWith(commandName))
                                {
                                    response = line;
                                }
                                else
                                {
                                    response = line;
                                }
                                break;
                            }

                            Display.WriteLine(VerbosityLevel.Full, $"  [{commandName}] {line}");
                        }
                        catch (TimeoutException)
                        {
                            continue;
                        }
                    }

                    if (response is null)
                    {
                        result.Passed = false;
                        result.ErrorMessage = "Timeout waiting for response";
                        failed++;
                        Console.BackgroundColor = ConsoleColor.Red;
                        Display.WriteLine(VerbosityLevel.Quiet, $"  ✗ FAILED: Timeout");
                        Console.ResetColor();
                    }
                    else
                    {
                        result.ActualResponse = response;
                        result.Passed = test.IsMatch(response);

                        if (result.Passed)
                        {
                            passed++;
                            Display.WriteLine(VerbosityLevel.Normal, $"  ✓ PASSED: {response}");
                        }
                        else
                        {
                            failed++;
                            Console.BackgroundColor = ConsoleColor.Red;
                            Display.WriteLine(VerbosityLevel.Quiet, $"  ✗ FAILED: Expected [{string.Join(" | ", test.ExpectedResponses)}], Got [{response}]");
                            Console.ResetColor();
                        }
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
        catch (UnauthorizedAccessException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: Access denied to COM port {portName}. {ex.Message}");
            return -1;
        }
        catch (IOException ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: Communication error: {ex.Message}");
            return -1;
        }
        catch (Exception ex)
        {
            Display.WriteLine(VerbosityLevel.Quiet, $"Error: {ex.Message}");
            return -1;
        }
        finally
        {
            ClosePort();
        }
    }

    private void ClosePort()
    {
        if (_serialPort != null && _serialPort.IsOpen)
        {
            try
            {
                _serialPort.Close();
            }
            catch
            {
                // Ignore errors during cleanup
            }
        }
    }

    public void Dispose()
    {
        ClosePort();
        _serialPort?.Dispose();
        GC.SuppressFinalize(this);
    }
}
