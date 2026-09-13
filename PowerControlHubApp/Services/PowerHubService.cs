using PowerControlHubApp.Messages;
using PowerControlHubApp.Models;
using PowerControlHubApp.Models.Json;

namespace PowerControlHubApp.Services;

/// <summary>
/// Thrown when the device responded but its payload could not be parsed.
/// Distinct from a network failure so callers can preserve the last known data.
/// </summary>
public class DeviceResponseException : Exception
{
    public DeviceResponseException(string message, Exception inner)
        : base(message, inner) { }
}

/// <summary>
/// Wraps all HTTP communication with the PowerControlHub ESP32 device.
/// Endpoints used:
///   GET  /api/index               — combined relays + sensors JSON
///   GET  /api/relay               — relay states only
///   GET  /api/relay/R3?{i}={0|1}  — set relay state (index i to on/off)
/// </summary>
public class PowerHubService
{
    private readonly IDashboardConnection _dashboardConnection;
    private readonly IConfigConnection _configConnection;
    private readonly IDashboardProvider _dashboardProvider;

    public PowerHubService(IDashboardConnection dashboardConnection,
        IConfigConnection configConnection,
        IMessageBus messageBus,
        IDashboardProvider dashboardProvider)
    {
        _dashboardConnection = dashboardConnection ?? throw new ArgumentNullException(nameof(dashboardConnection));
        _configConnection = configConnection ?? throw new ArgumentNullException(nameof(configConnection));
        _dashboardProvider = dashboardProvider ?? throw new ArgumentNullException(nameof(dashboardProvider));

        messageBus.Subscribe<AuthConfigChanged>(OnAuthConfigChanged);
    }

    public bool IsConfigured => _dashboardConnection.IsConfigured;

    public string BaseUrl => _dashboardConnection is DashboardConnection dc ? dc.BaseUrl : string.Empty;

    public IndexModel CurrentIndex => _dashboardProvider?.CurrentIndex;

    public void Configure(string ipAddress, int port, string apiKey = "", string hmacKey = "")
    {
        if (_dashboardConnection is DashboardConnection dc)
        {
            dc.Configure(ipAddress, port);
            dc.ConfigureAuth(apiKey ?? string.Empty, hmacKey ?? string.Empty);
        }

        if (_configConnection is ConfigConnection cc)
        {
            cc.Configure(ipAddress, port);
            cc.ConfigureAuth(apiKey ?? string.Empty, hmacKey ?? string.Empty);
        }
    }

    public Task<SystemPinsResponseModel> GetSystemPinsAsync(CancellationToken ct = default)
    {
        return _dashboardConnection.GetSystemPinsAsync(ct);
    }

    public Task<SystemLocationTypesResponseModel> GetSystemLocationTypesAsync(CancellationToken ct = default)
    {
        return _dashboardConnection.GetSystemLocationTypesAsync(ct);
    }

    public Task<SystemPinRestrictionsResponseModel> GetSystemPinRestrictionsAsync(CancellationToken ct = default)
    {
        return _dashboardConnection.GetSystemPinRestrictionsAsync(ct);
    }

    public Task<IndexModel> GetDashboardDataAsync(CancellationToken ct = default)
    {
        return _dashboardConnection.GetDashboardDataAsync(ct);
    }

    public Task<List<string>> GetWarningsAsync(CancellationToken ct = default)
    {
        return _dashboardConnection.GetWarningsAsync(ct);
    }

    public Task<List<SensorTypeDescriptorModel>> GetSensorMetaAsync(CancellationToken ct = default)
    {
        return _configConnection.GetSensorMetaAsync(ct);
    }

    public Task<bool> SetRelayStateAsync(int relayIndex, bool on, CancellationToken ct = default)
    {
        return _configConnection.SetRelayStateAsync(relayIndex, on, ct);
    }

    public Task<bool> RenameRelayAsync(int index, string shortName, string longName, CancellationToken ct = default)
    {
        return _configConnection.RenameRelayAsync(index, shortName, longName, ct);
    }

    public Task<bool> SetRelayColorAsync(int index, int colorIndex, CancellationToken ct = default)
    {
        return _configConnection.SetRelayColorAsync(index, colorIndex, ct);
    }

    public Task<bool> SetRelayDefaultStateAsync(int index, int defaultState, CancellationToken ct = default)
    {
        return _configConnection.SetRelayDefaultStateAsync(index, defaultState, ct);
    }

    public Task<bool> LinkRelayAsync(int index, int linkedIndex, CancellationToken ct = default)
    {
        return _configConnection.LinkRelayAsync(index, linkedIndex, ct);
    }

    public Task<bool> SetRelayActionTypeAsync(int index, int actionType, CancellationToken ct = default)
    {
        return _configConnection.SetRelayActionTypeAsync(index, actionType, ct);
    }

    public Task<bool> SetRelayPinAsync(int index, int pin, CancellationToken ct = default)
    {
        return _configConnection.SetRelayPinAsync(index, pin, ct);
    }

    public Task<List<ExternalSensorConfigModel>> GetExternalSensorsAsync(CancellationToken ct = default)
    {
        return _configConnection.GetExternalSensorsAsync(ct);
    }

    public Task<bool> SetExternalSensorCoreAsync(int index, int sensorId, string name, string mqttName, string mqttSlug, CancellationToken ct = default)
    {
        return _configConnection.SetExternalSensorCoreAsync(index, sensorId, name, mqttName, mqttSlug, ct);
    }

    public Task<bool> SetExternalSensorMqttAsync(int index, string typeSlug, string deviceClass, string unit, bool isBinary, CancellationToken ct = default)
    {
        return _configConnection.SetExternalSensorMqttAsync(index, typeSlug, deviceClass, unit, isBinary, ct);
    }

    public Task<bool> RemoveExternalSensorAsync(int index, CancellationToken ct = default)
    {
        return _configConnection.RemoveExternalSensorAsync(index, ct);
    }

    public Task<bool> RenameExternalSensorAsync(int index, string name, CancellationToken ct = default)
    {
        return _configConnection.RenameExternalSensorAsync(index, name, ct);
    }

    public Task<List<LocalSensorConfigModel>> GetLocalSensorsAsync(CancellationToken ct = default)
    {
        return _configConnection.GetLocalSensorsAsync(ct);
    }

    public Task<bool> AddUpdateLocalSensorAsync(int index, int type, sbyte opt0, sbyte opt1, CancellationToken ct = default)
    {
        return _configConnection.AddUpdateLocalSensorAsync(index, type, opt0, opt1, ct);
    }

    public Task<bool> RemoveLocalSensorAsync(int index, CancellationToken ct = default)
    {
        return _configConnection.RemoveLocalSensorAsync(index, ct);
    }

    public Task<bool> RenameLocalSensorAsync(int index, string name, CancellationToken ct = default)
    {
        return _configConnection.RenameLocalSensorAsync(index, name, ct);
    }

    public Task<bool> SetLocalSensorPinAsync(int index, int slot, byte pin, CancellationToken ct = default)
    {
        return _configConnection.SetLocalSensorPinAsync(index, slot, pin, ct);
    }

    public Task<bool> SetLocalSensorEnabledAsync(int index, bool enabled, CancellationToken ct = default)
    {
        return _configConnection.SetLocalSensorEnabledAsync(index, enabled, ct);
    }

    public Task<bool> SetLocalSensorOptionAsync(int index, int slot, int group, int value, CancellationToken ct = default)
    {
        return _configConnection.SetLocalSensorOptionAsync(index, slot, group, value, ct);
    }

    public Task<bool> SaveSettingsAsync(CancellationToken ct = default)
    {
        return _configConnection.SaveSettingsAsync(ct);
    }

    public Task<OtaStatusModel> GetOtaStatusAsync(CancellationToken ct = default)
    {
        return _configConnection.GetOtaStatusAsync(ct);
    }

    public Task<bool> TriggerOtaInstallAsync(CancellationToken ct = default)
    {
        return _configConnection.TriggerOtaInstallAsync(ct);
    }

    public Task<DateTimeOffset?> GetDateTimeAsync(CancellationToken ct = default)
    {
        return _configConnection.GetDateTimeAsync(ct);
    }

    public Task<bool> SetDateTimeAsync(long unixTimestamp, CancellationToken ct = default)
    {
        return _configConnection.SetDateTimeAsync(unixTimestamp, ct);
    }

    public Task<int?> GetTimezoneOffsetAsync(CancellationToken ct = default)
    {
        return _configConnection.GetTimezoneOffsetAsync(ct);
    }

    public Task<bool> SetTimezoneOffsetAsync(int offsetHours, CancellationToken ct = default)
    {
        return _configConnection.SetTimezoneOffsetAsync(offsetHours, ct);
    }

    public Task<bool> SetLocationTypeAsync(int type, CancellationToken ct = default)
    {
        return _configConnection.SetLocationTypeAsync(type, ct);
    }

    public Task<bool> SetMmsiAsync(string mmsi, CancellationToken ct = default)
    {
        return _configConnection.SetMmsiAsync(mmsi, ct);
    }

    public Task<bool> SetCallSignAsync(string callSign, CancellationToken ct = default)
    {
        return _configConnection.SetCallSignAsync(callSign, ct);
    }

    public Task<bool> SetHomePortAsync(string homePort, CancellationToken ct = default)
    {
        return _configConnection.SetHomePortAsync(homePort, ct);
    }

    public Task<bool> SetLocationNameAsync(string name, CancellationToken ct = default)
    {
        return _configConnection.SetLocationNameAsync(name, ct);
    }

    public Task<bool?> GetMqttEnabledAsync(CancellationToken ct = default)
    {
        return _configConnection.GetMqttEnabledAsync(ct);
    }

    public Task<bool> SetMqttEnabledAsync(bool enabled, CancellationToken ct = default)
    {
        return _configConnection.SetMqttEnabledAsync(enabled, ct);
    }

    public Task<string> GetMqttBrokerAsync(CancellationToken ct = default)
    {
        return _configConnection.GetMqttBrokerAsync(ct);
    }

    public Task<bool> SetMqttBrokerAsync(string broker, CancellationToken ct = default)
    {
        return _configConnection.SetMqttBrokerAsync(broker, ct);
    }

    public Task<int?> GetMqttPortAsync(CancellationToken ct = default)
    {
        return _configConnection.GetMqttPortAsync(ct);
    }

    public Task<bool> SetMqttPortAsync(int port, CancellationToken ct = default)
    {
        return _configConnection.SetMqttPortAsync(port, ct);
    }

    public Task<string> GetMqttUsernameAsync(CancellationToken ct = default)
    {
        return _configConnection.GetMqttUsernameAsync(ct);
    }

    public Task<bool> SetMqttUsernameAsync(string username, CancellationToken ct = default)
    {
        return _configConnection.SetMqttUsernameAsync(username, ct);
    }

    public Task<bool> SetMqttPasswordAsync(string password, CancellationToken ct = default)
    {
        return _configConnection.SetMqttPasswordAsync(password, ct);
    }

    public Task<string> GetMqttDeviceIdAsync(CancellationToken ct = default)
    {
        return _configConnection.GetMqttDeviceIdAsync(ct);
    }

    public Task<bool> SetMqttDeviceIdAsync(string deviceId, CancellationToken ct = default)
    {
        return _configConnection.SetMqttDeviceIdAsync(deviceId, ct);
    }

    public Task<bool?> GetMqttHADiscoveryAsync(CancellationToken ct = default)
    {
        return _configConnection.GetMqttHADiscoveryAsync(ct);
    }

    public Task<bool> SetMqttHADiscoveryAsync(bool enabled, CancellationToken ct = default)
    {
        return _configConnection.SetMqttHADiscoveryAsync(enabled, ct);
    }

    public Task<int?> GetMqttKeepAliveAsync(CancellationToken ct = default)
    {
        return _configConnection.GetMqttKeepAliveAsync(ct);
    }

    public Task<bool> SetMqttKeepAliveAsync(int seconds, CancellationToken ct = default)
    {
        return _configConnection.SetMqttKeepAliveAsync(seconds, ct);
    }

    public Task<bool?> GetMqttConnectionStateAsync(CancellationToken ct = default)
    {
        return _configConnection.GetMqttConnectionStateAsync(ct);
    }

    public Task<string> GetMqttDiscoveryPrefixAsync(CancellationToken ct = default)
    {
        return _configConnection.GetMqttDiscoveryPrefixAsync(ct);
    }

    public Task<bool> SetMqttDiscoveryPrefixAsync(string prefix, CancellationToken ct = default)
    {
        return _configConnection.SetMqttDiscoveryPrefixAsync(prefix, ct);
    }

    public Task<(int Sck, int Mosi, int Miso)?> GetSdCardSpiPinsAsync(CancellationToken ct = default)
    {
        return _configConnection.GetSdCardSpiPinsAsync(ct);
    }

    public Task<bool> SetSdCardSpiPinsAsync(int sck, int mosi, int miso, CancellationToken ct = default)
    {
        return _configConnection.SetSdCardSpiPinsAsync(sck, mosi, miso, ct);
    }

    public Task<int?> GetSdCardInitSpeedAsync(CancellationToken ct = default)
    {
        return _configConnection.GetSdCardInitSpeedAsync(ct);
    }

    public Task<bool> SetSdCardInitSpeedAsync(int speed, CancellationToken ct = default)
    {
        return _configConnection.SetSdCardInitSpeedAsync(speed, ct);
    }

    public Task<int?> GetSdCardCsPinAsync(CancellationToken ct = default)
    {
        return _configConnection.GetSdCardCsPinAsync(ct);
    }

    public Task<bool> SetSdCardCsPinAsync(int pin, CancellationToken ct = default)
    {
        return _configConnection.SetSdCardCsPinAsync(pin, ct);
    }

    public Task<AuthConfigModel> GetAuthConfigAsync(CancellationToken ct = default)
    {
        return _configConnection.GetAuthConfigAsync(ct);
    }

    public Task<(int DataPin, int ClockPin, int ResetPin)?> GetRtcPinsAsync(CancellationToken ct = default)
    {
        return _configConnection.GetRtcPinsAsync(ct);
    }

    public Task<bool> SetRtcPinsAsync(int dataPin, int clockPin, int resetPin, CancellationToken ct = default)
    {
        return _configConnection.SetRtcPinsAsync(dataPin, clockPin, resetPin, ct);
    }

    public Task<bool> SetXpdzTonePinAsync(int pin, CancellationToken ct = default)
    {
        return _configConnection.SetXpdzTonePinAsync(pin, ct);
    }

    public Task<bool> SetAuthEnabledAsync(bool enabled, CancellationToken ct = default)
    {
        return _configConnection.SetAuthEnabledAsync(enabled, ct);
    }

    public Task<bool> SetAuthApiKeyAsync(string apiKey, CancellationToken ct = default)
    {
        return _configConnection.SetAuthApiKeyAsync(apiKey, ct);
    }

    public Task<bool> SetAuthHmacKeyAsync(string hmacKey, CancellationToken ct = default)
    {
        return _configConnection.SetAuthHmacKeyAsync(hmacKey, ct);
    }

    public Task<bool> GenerateAuthKeysAsync(CancellationToken ct = default)
    {
        return _configConnection.GenerateAuthKeysAsync(ct);
    }

    public Task<NextionConfigModel> GetNextionConfigAsync(CancellationToken ct = default)
    {
        return _configConnection.GetNextionConfigAsync(ct);
    }

    public Task<bool> SetNextionEnabledAsync(bool enabled, CancellationToken ct = default)
    {
        return _configConnection.SetNextionEnabledAsync(enabled, ct);
    }

    public Task<bool> SetNextionHardwareSerialAsync(bool hardwareSerial, CancellationToken ct = default)
    {
        return _configConnection.SetNextionHardwareSerialAsync(hardwareSerial, ct);
    }

    public Task<bool> SetNextionRxPinAsync(int pin, CancellationToken ct = default)
    {
        return _configConnection.SetNextionRxPinAsync(pin, ct);
    }

    public Task<bool> SetNextionTxPinAsync(int pin, CancellationToken ct = default)
    {
        return _configConnection.SetNextionTxPinAsync(pin, ct);
    }

    public Task<bool> SetNextionBaudRateAsync(int baudRate, CancellationToken ct = default)
    {
        return _configConnection.SetNextionBaudRateAsync(baudRate, ct);
    }

    public Task<bool> SetNextionUartNumberAsync(int uartNumber, CancellationToken ct = default)
    {
        return _configConnection.SetNextionUartNumberAsync(uartNumber, ct);
    }

    private void OnAuthConfigChanged(AuthConfigChanged msg)
    {
        if (_dashboardConnection is DashboardConnection dc)
            dc.ConfigureAuth(msg.Config.ApiKey, msg.Config.HmacKey);

        if (_configConnection is ConfigConnection cc)
            cc.ConfigureAuth(msg.Config.ApiKey, msg.Config.HmacKey);
    }
}
