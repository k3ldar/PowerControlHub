using Microsoft.Extensions.Logging;
using PowerControlHubApp.Messages;
using PowerControlHubApp.Models;
using PowerControlHubApp.Models.Json;
using System.Net.Sockets;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Channels;
using static PowerControlHubApp.Internal.Constants;
using static PowerControlHubApp.Resources.Localization.AppResources;

namespace PowerControlHubApp.Services
{
    public class ConfigConnection : IConfigConnection, IDisposable
    {
        private readonly HttpClient _client;
        private readonly SocketsHttpHandler _handler;
        private readonly DeviceAuthHandler _authHandler;
        private readonly Channel<ConfigCommand> _queue;
        private readonly IMessageBus _messageBus;
        private readonly ILogger<ConfigConnection> _log;
        private readonly CancellationTokenSource _cts;
        private readonly Task _consumerTask;
        private string _baseUrl;
        private bool _disposed;

        public ConfigConnection(IMessageBus messageBus, ILogger<ConfigConnection> log)
        {
            _messageBus = messageBus ?? throw new ArgumentNullException(nameof(messageBus));
            _log = log ?? throw new ArgumentNullException(nameof(log));
            _handler = CreateHandler();
            _authHandler = new DeviceAuthHandler(_handler);

            _client = new HttpClient(_authHandler, disposeHandler: false)
            {
                Timeout = TimeSpan.FromSeconds(SecondsTen)
            };

            _client.DefaultRequestHeaders.ConnectionClose = false;
            _client.DefaultRequestHeaders.TryAddWithoutValidation(UserAgentKey, UserAgentValue);
            _client.DefaultRequestHeaders.TryAddWithoutValidation(ConnectionTypeKey, ConnectionTypePersistent);

            _queue = Channel.CreateBounded<ConfigCommand>(new BoundedChannelOptions(ConfigConnectionQueueCapacity)
            {
                FullMode = BoundedChannelFullMode.DropOldest
            });

            _cts = new CancellationTokenSource();
            _baseUrl = string.Empty;
            _consumerTask = Task.Run(() => ConsumeQueueAsync(_cts.Token));
        }

        public void Configure(string ipAddress, int port)
        {
            _baseUrl = $"http://{ipAddress}:{port}";
            _client.BaseAddress = new Uri(_baseUrl + ForwardSlash);
        }

        /// <summary>Update authentication credentials used by the handler pipeline.</summary>
        public void ConfigureAuth(string apiKey, string hmacKey)
        {
            _authHandler.Configure(apiKey, hmacKey);
        }

        public bool IsConfigured => !string.IsNullOrEmpty(_baseUrl);

        public bool IsQueueEmpty => _queue.Reader.Count == 0;

        public int QueueLength => _queue.Reader.Count;

        public void EnqueueCommand(ConfigCommand command)
        {
            _queue.Writer.TryWrite(command);
        }

        private async Task ConsumeQueueAsync(CancellationToken ct)
        {
            while (!ct.IsCancellationRequested)
            {
                try
                {
                    ConfigCommand command = await _queue.Reader.ReadAsync(ct);
                    _log.LogDebug(LogConfigExecuting, command.Description);
                    bool success = await command.ExecuteAsync(_client, ct);

                    if (success)
                    {
                        _log.LogDebug(LogConfigSucceeded, command.Description);
                        TryPublishSuccess(command);
                    }
                    else
                    {
                        _log.LogWarning(LogConfigFailed, command.Description);
                        TryPublishFailure(command, LogConfigFailureResponse);
                    }
                }

                catch (OperationCanceledException) when (ct.IsCancellationRequested)
                {
                    break;
                }
                catch (Exception ex)
                {
                    _log.LogError(ex, LogConfigConsumerError);
                }
            }
        }

        private void TryPublishSuccess(ConfigCommand command)
        {
            try
            {
                if (command.SuccessMessageType == ConfigSuccessRelay && command.Context is int relayIndex)
                {
                    _messageBus.Publish(new RelayConfigSucceeded(relayIndex));
                }
                else if (command.SuccessMessageType == ConfigSuccessSensor && command.Context is Tuple<int, string> sensorCtx)
                {
                    _messageBus.Publish(new SensorConfigSucceeded(sensorCtx.Item1, sensorCtx.Item2));
                }
            }
            catch (Exception ex)
            {
                _log.LogWarning(ex, LogPublishSuccessFailed, command.Description);
            }
        }

        private void TryPublishFailure(ConfigCommand command, string error)
        {
            try
            {
                _messageBus.Publish(new ConfigCommandFailed(command.Description, error));
            }
            catch (Exception ex)
            {
                _log.LogWarning(ex, LogPublishFailureFailed, command.Description);
            }
        }

        private static SocketsHttpHandler CreateHandler()
        {
            SocketsHttpHandler handler = new SocketsHttpHandler
            {
                PooledConnectionIdleTimeout = TimeSpan.FromSeconds(SecondsSixty),
                PooledConnectionLifetime = Timeout.InfiniteTimeSpan,
                ConnectTimeout = TimeSpan.FromSeconds(SecondsFive),
                MaxConnectionsPerServer = 1,
                ConnectCallback = async (context, ct) =>
                {
                    Socket socket = new Socket(SocketType.Stream, ProtocolType.Tcp)
                    {
                        NoDelay = true
                    };
                    socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.KeepAlive, true);
                    try
                    {
                        socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.TcpKeepAliveTime, SecondsTen);
                        socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.TcpKeepAliveInterval, SecondsFive);
                        socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.TcpKeepAliveRetryCount, SecondsThree);
                    }
                    catch
                    {
                    }
                    await socket.ConnectAsync(context.DnsEndPoint, ct);
                    return new NetworkStream(socket, ownsSocket: true);
                }
            };
            return handler;
        }

        private static readonly JsonSerializerOptions JsonOptions = new()
        {
            PropertyNameCaseInsensitive = true,
            NumberHandling = JsonNumberHandling.AllowReadingFromString,
            DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
        };

        public async Task<List<SensorTypeDescriptorModel>> GetSensorMetaAsync(CancellationToken ct = default)
        {
            string json = await _client.GetStringAsync($"{RouteLocalSensorConfig}{SensorConfigGetAll}?meta=1", ct);
            using JsonDocument doc = JsonDocument.Parse(json);

            if (doc.RootElement.TryGetProperty(JsonMeta, out JsonElement meta) &&
                meta.TryGetProperty(JsonMetaDescriptors, out JsonElement descriptors) &&
                descriptors.ValueKind == JsonValueKind.Array)
            {
                return JsonSerializer.Deserialize<List<SensorTypeDescriptorModel>>(descriptors.GetRawText(), JsonOptions) ?? [];
            }

            return [];
        }

        public async Task<List<ExternalSensorConfigModel>> GetExternalSensorsAsync(CancellationToken ct = default)
        {
            string json = await _client.GetStringAsync(RouteExternalSensor, ct);
            using JsonDocument doc = JsonDocument.Parse(json);

            List<ExternalSensorConfigModel> list = [];

            if (doc.RootElement.TryGetProperty(JsonSensors, out JsonElement sensors) && sensors.ValueKind == JsonValueKind.Array)
            {
                foreach (JsonElement el in sensors.EnumerateArray())
                {
                    ExternalSensorConfigModel m = new ExternalSensorConfigModel
                    {
                        Index = el.GetProperty(JsonSensorIndex).GetInt32(),
                        SensorId = el.GetProperty(JsonSensorId).GetInt32(),
                        Name = el.GetProperty(JsonSensorName).GetString() ?? string.Empty,
                        MqttName = el.GetProperty(JsonSensorMqttName).GetString() ?? string.Empty,
                        MqttSlug = el.GetProperty(JsonSensorMqttSlug).GetString() ?? string.Empty,
                        MqttTypeSlug = el.GetProperty(JsonSensorMqttType).GetString() ?? string.Empty,
                        MqttDeviceClass = el.GetProperty(JsonSensorMqttDeviceClass).GetString() ?? string.Empty,
                        MqttUnit = el.GetProperty(JsonSensorMqttUnit).GetString() ?? string.Empty,
                        MqttIsBinary = el.GetProperty(JsonSensorMqttBinary).GetBoolean()
                    };

                    list.Add(m);
                }
            }

            return list;
        }

        public async Task<bool> SetExternalSensorCoreAsync(int index, int sensorId, string name, string mqttName, string mqttSlug, CancellationToken ct = default)
        {
            string url = $"api/externalsensor/E1?i={index}&id={sensorId}&n={Uri.EscapeDataString(name)}&mn={Uri.EscapeDataString(mqttName)}&ms={Uri.EscapeDataString(mqttSlug)}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> SetExternalSensorMqttAsync(int index, string typeSlug, string deviceClass, string unit, bool isBinary, CancellationToken ct = default)
        {
            string url = $"api/externalsensor/E2?i={index}&mt={Uri.EscapeDataString(typeSlug)}&md={Uri.EscapeDataString(deviceClass)}&mu={Uri.EscapeDataString(unit)}&bin={(isBinary ? 1 : 0)}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> RemoveExternalSensorAsync(int index, CancellationToken ct = default)
        {
            string url = $"api/externalsensor/E3?v={index}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> RenameExternalSensorAsync(int index, string name, CancellationToken ct = default)
        {
            string url = $"api/externalsensor/E4?{index}={Uri.EscapeDataString(name)}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<List<LocalSensorConfigModel>> GetLocalSensorsAsync(CancellationToken ct = default)
        {
            string json = await _client.GetStringAsync(RouteLocalSensorConfig, ct);
            using JsonDocument doc = JsonDocument.Parse(json);

            List<LocalSensorConfigModel> list = [];

            if (doc.RootElement.TryGetProperty(JsonSensors, out JsonElement sensors) && sensors.ValueKind == JsonValueKind.Array)
            {
                foreach (JsonElement el in sensors.EnumerateArray())
                {
                    LocalSensorConfigModel m = new LocalSensorConfigModel
                    {
                        Index = el.GetProperty(JsonLocalSensorIndex).GetInt32(),
                        SensorType = el.GetProperty(JsonLocalSensorType).GetInt32(),
                        Name = el.GetProperty(JsonLocalSensorName).GetString() ?? string.Empty,
                        Pin0 = el.GetProperty(JsonLocalSensorPin0).GetInt32(),
                        Pin1 = el.GetProperty(JsonLocalSensorPin1).GetInt32(),
                        Opt1_0 = el.GetProperty(JsonLocalSensorOpt1_0).GetInt32(),
                        Opt1_1 = el.GetProperty(JsonLocalSensorOpt1_1).GetInt32(),
                        Opt2_0 = el.GetProperty(JsonLocalSensorOpt2_0).GetInt32(),
                        Opt2_1 = el.GetProperty(JsonLocalSensorOpt2_1).GetInt32(),
                        Enabled = el.GetProperty(JsonLocalSensorEnabled).GetInt32() != 0
                    };

                    list.Add(m);
                }
            }

            return list;
        }

        public async Task<bool> AddUpdateLocalSensorAsync(int index, int type, sbyte opt0, sbyte opt1, CancellationToken ct = default)
        {
            string url = $"api/sensor/S1?i={index}&t={type}&o0={opt0}&o1={opt1}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> RemoveLocalSensorAsync(int index, CancellationToken ct = default)
        {
            string url = $"api/sensor/S2?v={index}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> RenameLocalSensorAsync(int index, string name, CancellationToken ct = default)
        {
            string url = $"api/sensor/S3?{index}={Uri.EscapeDataString(name)}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> SetLocalSensorPinAsync(int index, int slot, byte pin, CancellationToken ct = default)
        {
            string url = $"api/sensor/S4?i={index}&s={slot}&v={pin}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> SetLocalSensorEnabledAsync(int index, bool enabled, CancellationToken ct = default)
        {
            string url = $"api/sensor/S5?{index}={(enabled ? 1 : 0)}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> SetLocalSensorOptionAsync(int index, int slot, int group, int value, CancellationToken ct = default)
        {
            string url = $"api/sensor/S6?i={index}&s={slot}&o={group}&v={value}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> SetRelayStateAsync(int relayIndex, bool on, CancellationToken ct = default)
        {
            int state = on ? 1 : 0;
            string url = $"api/relay/R3?{relayIndex}={state}";
            HttpResponseMessage response = await _client.GetAsync(url, ct);
            return response.IsSuccessStatusCode;
        }

        public async Task<bool> RenameRelayAsync(int index, string shortName, string longName, CancellationToken ct = default)
        {
            string value = string.IsNullOrWhiteSpace(longName) ? shortName : $"{shortName}|{longName}";
            string url = $"api/relay/R6?{index}={Uri.EscapeDataString(value)}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> SetRelayColorAsync(int index, int colorIndex, CancellationToken ct = default)
        {
            int nextionId = colorIndex <= RelayColorYellow ? colorIndex + NextionImageIdMin : UnconfiguredPin;
            string url = $"api/relay/R7?{index}={nextionId}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> SetRelayDefaultStateAsync(int index, int defaultState, CancellationToken ct = default)
        {
            string url = $"api/relay/R8?{index}={defaultState}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> LinkRelayAsync(int index, int linkedIndex, CancellationToken ct = default)
        {
            string url = $"api/relay/R9?{index}={linkedIndex}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> SetRelayActionTypeAsync(int index, int actionType, CancellationToken ct = default)
        {
            string url = $"api/relay/R10?{index}={actionType}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> SetRelayPinAsync(int index, int pin, CancellationToken ct = default)
        {
            string url = $"api/relay/R11?{index}={pin}";
            HttpResponseMessage response = await _client.PostAsync(url, null, ct);
            return await IsSuccessResponseAsync(response, ct);
        }

        public async Task<bool> SaveSettingsAsync(CancellationToken ct = default)
        {
            HttpResponseMessage response = await _client.PostAsync(RouteSaveConfig, null, ct);
            return response.IsSuccessStatusCode;
        }

        public async Task<OtaStatusModel> GetOtaStatusAsync(CancellationToken ct = default)
        {
            try
            {
                HttpResponseMessage response = await _client.GetAsync(RouteOtaUpdate, ct);

                if (!response.IsSuccessStatusCode)
                    return null;

                string body = await response.Content.ReadAsStringAsync(ct);
                using JsonDocument doc = JsonDocument.Parse(body);

                if (doc.RootElement.TryGetProperty(ErrorKey, out _))
                    return null;

                return JsonSerializer.Deserialize<OtaStatusModel>(body, JsonOptions);
            }
            catch
            {
                return null;
            }
        }

        public async Task<bool> TriggerOtaInstallAsync(CancellationToken ct = default)
        {
            try
            {
                HttpResponseMessage response = await _client.PostAsync(RouteUpdateOta, null, ct);
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        public async Task<DateTimeOffset?> GetDateTimeAsync(CancellationToken ct = default)
        {
            try
            {
                string json = await _client.GetStringAsync(RouteSystemGetDateTime, ct);
                using JsonDocument doc = JsonDocument.Parse(json);

                if (doc.RootElement.TryGetProperty(ResultSuccess, out JsonElement success) &&
                    success.GetBoolean() &&
                    doc.RootElement.TryGetProperty(JsonValueKey, out JsonElement v))
                {
                    string dtStr = v.GetString();

                    if (DateTimeOffset.TryParse(dtStr, out DateTimeOffset dt))
                        return dt;
                }
            }
            catch
            {
                // fall through
            }
            return null;
        }

        public async Task<bool> SetDateTimeAsync(long unixTimestamp, CancellationToken ct = default)
        {
            try
            {
                string url = $"{RouteSystemSetDateTime}?{JsonValueKey}={unixTimestamp}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        public async Task<int?> GetTimezoneOffsetAsync(CancellationToken ct = default)
        {
            try
            {
                string json = await _client.GetStringAsync(RouteConfigTimezoneOffset, ct);
                using JsonDocument doc = JsonDocument.Parse(json);

                if (doc.RootElement.TryGetProperty(ResultSuccess, out JsonElement success) &&
                    success.GetBoolean() &&
                    doc.RootElement.TryGetProperty(JsonValueKey, out JsonElement v))
                {
                    if (v.ValueKind == JsonValueKind.Number && v.TryGetInt32(out int val))
                        return val;

                    if (v.ValueKind == JsonValueKind.String && int.TryParse(v.GetString(), out val))
                        return val;
                }
            }
            catch
            {
                // fall through
            }
            return null;
        }

        public async Task<bool> SetTimezoneOffsetAsync(int offsetHours, CancellationToken ct = default)
        {
            try
            {
                string url = $"{RouteConfigTimezoneOffset}?{JsonValueKey}={offsetHours}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        public async Task<bool> SetLocationTypeAsync(int type, CancellationToken ct = default)
        {
            return await PostConfigValueAsync(RouteConfigBoatType, type.ToString(), ct);
        }

        public async Task<bool> SetMmsiAsync(string mmsi, CancellationToken ct = default)
        {
            return await PostConfigValueAsync(RouteConfigMmsi, mmsi, ct);
        }

        public async Task<bool> SetCallSignAsync(string callSign, CancellationToken ct = default)
        {
            return await PostConfigValueAsync(RouteConfigCallSign, callSign, ct);
        }

        public async Task<bool> SetHomePortAsync(string homePort, CancellationToken ct = default)
        {
            return await PostConfigValueAsync(RouteConfigHomePort, homePort, ct);
        }

        public async Task<bool> SetLocationNameAsync(string name, CancellationToken ct = default)
        {
            return await PostConfigValueAsync(RouteConfigRename, name, ct);
        }

        public async Task<bool?> GetMqttEnabledAsync(CancellationToken ct = default)
        {
            return await GetMqttBoolAsync(MqttConfigEnabled, ct);
        }

        public async Task<bool> SetMqttEnabledAsync(bool enabled, CancellationToken ct = default)
        {
            return await SetMqttStringAsync(MqttConfigEnabled, enabled ? BoolTrueString : BoolFalseString, ct);
        }

        public async Task<string> GetMqttBrokerAsync(CancellationToken ct = default)
        {
            return await GetMqttStringAsync(MqttConfigBroker, ct) ?? string.Empty;
        }

        public async Task<bool> SetMqttBrokerAsync(string broker, CancellationToken ct = default)
        {
            return await SetMqttStringAsync(MqttConfigBroker, broker, ct);
        }

        public async Task<int?> GetMqttPortAsync(CancellationToken ct = default)
        {
            return await GetMqttIntAsync(MqttConfigPort, ct);
        }

        public async Task<bool> SetMqttPortAsync(int port, CancellationToken ct = default)
        {
            return await SetMqttStringAsync(MqttConfigPort, port.ToString(), ct);
        }

        public async Task<string> GetMqttUsernameAsync(CancellationToken ct = default)
        {
            return await GetMqttStringAsync(MqttConfigUsername, ct) ?? string.Empty;
        }

        public async Task<bool> SetMqttUsernameAsync(string username, CancellationToken ct = default)
        {
            return await SetMqttStringAsync(MqttConfigUsername, username, ct);
        }

        public async Task<bool> SetMqttPasswordAsync(string password, CancellationToken ct = default)
        {
            return await SetMqttStringAsync(MqttConfigPassword, password, ct);
        }

        public async Task<string> GetMqttDeviceIdAsync(CancellationToken ct = default)
        {
            return await GetMqttStringAsync(MqttConfigDeviceId, ct) ?? string.Empty;
        }

        public async Task<bool> SetMqttDeviceIdAsync(string deviceId, CancellationToken ct = default)
        {
            return await SetMqttStringAsync(MqttConfigDeviceId, deviceId, ct);
        }

        public async Task<bool?> GetMqttHADiscoveryAsync(CancellationToken ct = default)
        {
            return await GetMqttBoolAsync(MqttConfigHADiscovery, ct);
        }

        public async Task<bool> SetMqttHADiscoveryAsync(bool enabled, CancellationToken ct = default)
        {
            return await SetMqttStringAsync(MqttConfigHADiscovery, enabled ? BoolTrueString : BoolFalseString, ct);
        }

        public async Task<int?> GetMqttKeepAliveAsync(CancellationToken ct = default)
        {
            return await GetMqttIntAsync(MqttConfigKeepAlive, ct);
        }

        public async Task<bool> SetMqttKeepAliveAsync(int seconds, CancellationToken ct = default)
        {
            return await SetMqttStringAsync(MqttConfigKeepAlive, seconds.ToString(), ct);
        }

        public async Task<bool?> GetMqttConnectionStateAsync(CancellationToken ct = default)
        {
            return await GetMqttBoolAsync(MqttConfigState, ct);
        }

        public async Task<string> GetMqttDiscoveryPrefixAsync(CancellationToken ct = default)
        {
            return await GetMqttStringAsync(MqttConfigDiscoveryPrefix, ct) ?? string.Empty;
        }

        public async Task<bool> SetMqttDiscoveryPrefixAsync(string prefix, CancellationToken ct = default)
        {
            return await SetMqttStringAsync(MqttConfigDiscoveryPrefix, prefix, ct);
        }

        public async Task<(int Sck, int Mosi, int Miso)?> GetSdCardSpiPinsAsync(CancellationToken ct = default)
        {
            try
            {
                string json = await _client.GetStringAsync(RouteConfigSdCardSpiPins, ct);
                using JsonDocument doc = JsonDocument.Parse(json);

                if (doc.RootElement.TryGetProperty(ResultSuccess, out JsonElement success) &&
                    success.GetBoolean() &&
                    doc.RootElement.TryGetProperty(JsonSpiSckKey, out JsonElement sck) &&
                    doc.RootElement.TryGetProperty(JsonSpiMosiKey, out JsonElement mosi) &&
                    doc.RootElement.TryGetProperty(JsonSpiMisoKey, out JsonElement miso))
                {
                    return (sck.GetInt32(), mosi.GetInt32(), miso.GetInt32());
                }
            }
            catch
            {
                // fall through
            }
            return null;
        }

        public async Task<bool> SetSdCardSpiPinsAsync(int sck, int mosi, int miso, CancellationToken ct = default)
        {
            try
            {
                string url = $"{RouteConfigSdCardSpiPins}?sck={sck}&mosi={mosi}&miso={miso}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        public async Task<int?> GetSdCardInitSpeedAsync(CancellationToken ct = default)
        {
            return await GetConfigIntAsync(RouteConfigSdCardInitSpeed, ct);
        }

        public async Task<bool> SetSdCardInitSpeedAsync(int speed, CancellationToken ct = default)
        {
            return await SetConfigValueAsync(RouteConfigSdCardInitSpeed, speed, ct);
        }

        public async Task<int?> GetSdCardCsPinAsync(CancellationToken ct = default)
        {
            return await GetConfigIntAsync(RouteConfigSdCardCsPin, ct);
        }

        public async Task<bool> SetSdCardCsPinAsync(int pin, CancellationToken ct = default)
        {
            return await SetConfigValueAsync(RouteConfigSdCardCsPin, pin, ct);
        }

        public async Task<(int DataPin, int ClockPin, int ResetPin)?> GetRtcPinsAsync(CancellationToken ct = default)
        {
            try
            {
                string json = await _client.GetStringAsync(RouteConfigRtc, ct);
                using JsonDocument doc = JsonDocument.Parse(json);

                if (doc.RootElement.TryGetProperty(ResultSuccess, out JsonElement success) &&
                    success.GetBoolean())
                {
                    int? data = null;
                    int? clock = null;
                    int? reset = null;

                    if (doc.RootElement.TryGetProperty(JsonRtcDataPin, out JsonElement d) && d.ValueKind == JsonValueKind.Number && d.TryGetInt32(out int dv))
                        data = dv;

                    if (doc.RootElement.TryGetProperty(JsonRtcClockPin, out JsonElement c) && c.ValueKind == JsonValueKind.Number && c.TryGetInt32(out int cv))
                        clock = cv;

                    if (doc.RootElement.TryGetProperty(JsonRtcResetPin, out JsonElement r) && r.ValueKind == JsonValueKind.Number && r.TryGetInt32(out int rv))
                        reset = rv;

                    if (data.HasValue && clock.HasValue && reset.HasValue)
                        return (data.Value, clock.Value, reset.Value);
                }
            }
            catch
            {
                // fall through
            }

            return null;
        }

        public async Task<bool> SetRtcPinsAsync(int dataPin, int clockPin, int resetPin, CancellationToken ct = default)
        {
            try
            {
                string url = $"{RouteConfigRtc}?{JsonRtcDataPin}={dataPin}&{JsonRtcClockPin}={clockPin}&{JsonRtcResetPin}={resetPin}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        public async Task<bool> SetXpdzTonePinAsync(int pin, CancellationToken ct = default)
        {
            try
            {
                string url = $"{RouteConfigXpdzTone}?v={pin}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        public async Task<AuthConfigModel> GetAuthConfigAsync(CancellationToken ct = default)
        {
            try
            {
                string json = await _client.GetStringAsync(RouteConfigAuth, ct);
                return JsonSerializer.Deserialize<AuthConfigModel>(json, JsonOptions) ?? new AuthConfigModel();
            }
            catch
            {
                return new AuthConfigModel();
            }
        }

        public async Task<bool> SetAuthEnabledAsync(bool enabled, CancellationToken ct = default)
        {
            try
            {
                string url = $"{RouteConfigAuth}?{ConfigAuthEnabledParam}={(enabled ? BoolTrueString : BoolFalseString)}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);

                if (!await IsSuccessResponseAsync(response, ct))
                    return false;

                await SyncAuthHandlerFromDeviceAsync(ct);
                return true;
            }
            catch
            {
                return false;
            }
        }

        public async Task<bool> SetAuthApiKeyAsync(string apiKey, CancellationToken ct = default)
        {
            try
            {
                string url = $"{RouteConfigAuth}?{ConfigAuthApiKeyParam}={Uri.EscapeDataString(apiKey)}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);

                if (!await IsSuccessResponseAsync(response, ct))
                    return false;

                await SyncAuthHandlerFromDeviceAsync(ct);
                return true;
            }
            catch
            {
                return false;
            }
        }

        public async Task<bool> SetAuthHmacKeyAsync(string hmacKey, CancellationToken ct = default)
        {
            try
            {
                string url = $"{RouteConfigAuth}?{ConfigAuthHmacKeyParam}={Uri.EscapeDataString(hmacKey)}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);

                if (!await IsSuccessResponseAsync(response, ct))
                    return false;

                await SyncAuthHandlerFromDeviceAsync(ct);
                return true;
            }
            catch
            {
                return false;
            }
        }

        public async Task<bool> GenerateAuthKeysAsync(CancellationToken ct = default)
        {
            try
            {
                string url = $"{RouteConfigAuth}?{ConfigAuthGenerateParam}={BoolTrueString}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);

                if (!await IsSuccessResponseAsync(response, ct))
                    return false;

                await SyncAuthHandlerFromDeviceAsync(ct);
                return true;
            }
            catch
            {
                return false;
            }
        }

        public async Task<NextionConfigModel> GetNextionConfigAsync(CancellationToken ct = default)
        {
            try
            {
                string json = await _client.GetStringAsync(RouteConfigNextionGet, ct);
                using JsonDocument doc = JsonDocument.Parse(json);

                NextionConfigModel model = new NextionConfigModel();

                if (doc.RootElement.ValueKind == JsonValueKind.Object)
                {
                    foreach (JsonProperty prop in doc.RootElement.EnumerateObject())
                    {
                        string name = prop.Name.ToLowerInvariant();

                        if (name == NextionJsonN1 || name == NextionJsonEnabled || name == NextionJsonEn)
                        {
                            if (prop.Value.ValueKind == JsonValueKind.Number && prop.Value.TryGetInt32(out int v))
                                model.Enabled = v != 0;
                        }
                        else if (name == NextionJsonN2 || name == NextionJsonHardwareSerial || name == NextionJsonHw)
                        {
                            if (prop.Value.ValueKind == JsonValueKind.Number && prop.Value.TryGetInt32(out int v))
                                model.HardwareSerial = v != 0;
                        }
                        else if (name == NextionJsonN3 || name == NextionJsonRxPin || name == NextionJsonRx)
                        {
                            if (prop.Value.ValueKind == JsonValueKind.Number && prop.Value.TryGetInt32(out int v))
                                model.RxPin = v;
                        }
                        else if (name == NextionJsonN4 || name == NextionJsonTxPin || name == NextionJsonTx)
                        {
                            if (prop.Value.ValueKind == JsonValueKind.Number && prop.Value.TryGetInt32(out int v))
                                model.TxPin = v;
                        }
                        else if (name == NextionJsonN5 || name == NextionJsonBaudRate || name == NextionJsonBaud)
                        {
                            if (prop.Value.ValueKind == JsonValueKind.Number && prop.Value.TryGetInt32(out int v))
                                model.BaudRate = v;
                        }
                        else if (name == NextionJsonN6 || name == NextionJsonUartNumber || name == NextionJsonUart)
                        {
                            if (prop.Value.ValueKind == JsonValueKind.Number && prop.Value.TryGetInt32(out int v))
                                model.UartNumber = v;
                        }
                    }
                }

                if (model.Enabled == null || model.HardwareSerial == null || model.RxPin == null || model.TxPin == null || model.BaudRate == null || model.UartNumber == null)
                {
                    int? n1 = await GetConfigIntAsync(NextionRouteN1, ct);
                    int? n2 = await GetConfigIntAsync(NextionRouteN2, ct);
                    int? n3 = await GetConfigIntAsync(NextionRouteN3, ct);
                    int? n4 = await GetConfigIntAsync(NextionRouteN4, ct);
                    int? n5 = await GetConfigIntAsync(NextionRouteN5, ct);
                    int? n6 = await GetConfigIntAsync(NextionRouteN6, ct);

                    model.Enabled ??= (n1.HasValue ? n1.Value != 0 : (bool?)null);
                    model.HardwareSerial ??= (n2.HasValue ? n2.Value != 0 : (bool?)null);
                    model.RxPin ??= n3;
                    model.TxPin ??= n4;
                    model.BaudRate ??= n5;
                    model.UartNumber ??= n6;
                }

                return model;
            }
            catch
            {
                return new NextionConfigModel();
            }
        }

        public async Task<bool> SetNextionEnabledAsync(bool enabled, CancellationToken ct = default)
            => await SetConfigValueAsync(NextionRouteN1, enabled ? 1 : 0, ct);

        public async Task<bool> SetNextionHardwareSerialAsync(bool hardwareSerial, CancellationToken ct = default)
            => await SetConfigValueAsync(NextionRouteN2, hardwareSerial ? 1 : 0, ct);

        public async Task<bool> SetNextionRxPinAsync(int pin, CancellationToken ct = default)
            => await SetConfigValueAsync(NextionRouteN3, pin, ct);

        public async Task<bool> SetNextionTxPinAsync(int pin, CancellationToken ct = default)
            => await SetConfigValueAsync(NextionRouteN4, pin, ct);

        public async Task<bool> SetNextionBaudRateAsync(int baudRate, CancellationToken ct = default)
            => await SetConfigValueAsync(NextionRouteN5, baudRate, ct);

        public async Task<bool> SetNextionUartNumberAsync(int uartNumber, CancellationToken ct = default)
            => await SetConfigValueAsync(NextionRouteN6, uartNumber, ct);

        private async Task SyncAuthHandlerFromDeviceAsync(CancellationToken ct)
        {
            try
            {
                AuthConfigModel config = await GetAuthConfigAsync(ct);
                _authHandler.Configure(config.ApiKey, config.HmacKey);
                _messageBus.Publish(new AuthConfigChanged(config));
            }
            catch
            {
                // best-effort sync; if the GET fails the handler keeps its previous state
            }
        }

        private async Task<int?> GetConfigIntAsync(string route, CancellationToken ct)
        {
            try
            {
                string json = await _client.GetStringAsync(route, ct);
                using JsonDocument doc = JsonDocument.Parse(json);

                if (doc.RootElement.TryGetProperty(ResultSuccess, out JsonElement success) &&
                    success.GetBoolean() &&
                    doc.RootElement.TryGetProperty(JsonValueKey, out JsonElement v))
                {
                    if (v.ValueKind == JsonValueKind.Number && v.TryGetInt32(out int val))
                        return val;

                    if (v.ValueKind == JsonValueKind.String && int.TryParse(v.GetString(), out val))
                        return val;
                }
            }
            catch
            {
                // fall through
            }
            return null;
        }

        private async Task<bool> PostConfigValueAsync(string route, string value, CancellationToken ct)
        {
            try
            {
                string url = $"{route}?v={Uri.EscapeDataString(value)}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);
                return await IsSuccessResponseAsync(response, ct);
            }
            catch
            {
                return false;
            }
        }

        private async Task<bool> SetConfigValueAsync(string route, int value, CancellationToken ct)
        {
            try
            {
                string url = $"{route}?{JsonValueKey}={value}";
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        private async Task<string> GetMqttStringAsync(string command, CancellationToken ct)
        {
            try
            {
                string url = string.Format(RouteConfigMqttGet, command);
                string json = await _client.GetStringAsync(url, ct);
                using JsonDocument doc = JsonDocument.Parse(json);

                if (doc.RootElement.TryGetProperty(ResultSuccess, out JsonElement success) &&
                    success.GetBoolean() &&
                    doc.RootElement.TryGetProperty(JsonValueKey, out JsonElement v))
                {
                    return v.GetString();
                }
            }
            catch
            {
                // fall through
            }
            return null;
        }

        private async Task<int?> GetMqttIntAsync(string command, CancellationToken ct)
        {
            string raw = await GetMqttStringAsync(command, ct);

            if (raw != null && int.TryParse(raw, out int val))
                return val;

            return null;
        }

        private async Task<bool?> GetMqttBoolAsync(string command, CancellationToken ct)
        {
            string raw = await GetMqttStringAsync(command, ct);

            if (raw == null)
                return null;

            return raw == BoolTrueString;
        }

        private async Task<bool> SetMqttStringAsync(string command, string value, CancellationToken ct)
        {
            try
            {
                string url = string.Format(RouteConfigMqttSet, command, Uri.EscapeDataString(value));
                HttpResponseMessage response = await _client.PostAsync(url, null, ct);
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        private static async Task<bool> IsSuccessResponseAsync(HttpResponseMessage response, CancellationToken ct)
        {
            if (!response.IsSuccessStatusCode)
                return false;

            string body = await response.Content.ReadAsStringAsync(ct);

            try
            {
                using JsonDocument doc = JsonDocument.Parse(body);
                return doc.RootElement.TryGetProperty(ResultSuccess, out JsonElement s) && s.GetBoolean();
            }
            catch
            {
                return response.IsSuccessStatusCode;
            }
        }

        public void Dispose()
        {
            if (_disposed)
            {
                return;
            }

            _cts.Cancel();

            _consumerTask.ContinueWith(_ => { });

            _client?.Dispose();
            _handler?.Dispose();
            _cts.Dispose();
            _disposed = true;
        }
    }
}
