using System.Text.Json.Serialization;
using static PowerControlHubApp.Internal.Constants;
using static PowerControlHubApp.Resources.Localization.AppResources;

namespace PowerControlHubApp.Models
{
    /// <summary>
    /// Mirrors the JSON returned by GET /api/sensorconfig/ for a single local sensor entry.
    /// Field names match the firmware's SensorEntry broadcast format (S0 response).
    /// </summary>
    public sealed class LocalSensorConfigModel
    {
        [JsonPropertyName(JsonLocalSensorIndex)]
        public int Index { get; set; }

        [JsonPropertyName(JsonLocalSensorType)]
        public int SensorType { get; set; }

        [JsonPropertyName(JsonLocalSensorName)]
        public string Name { get; set; } = string.Empty;

        [JsonPropertyName(JsonLocalSensorPin0)]
        public int Pin0 { get; set; } = UnconfiguredPin;

        [JsonPropertyName(JsonLocalSensorPin1)]
        public int Pin1 { get; set; } = UnconfiguredPin;

        [JsonPropertyName(JsonLocalSensorOpt1_0)]
        public int Opt1_0 { get; set; }

        [JsonPropertyName(JsonLocalSensorOpt1_1)]
        public int Opt1_1 { get; set; }

        [JsonPropertyName(JsonLocalSensorOpt2_0)]
        public int Opt2_0 { get; set; }

        [JsonPropertyName(JsonLocalSensorOpt2_1)]
        public int Opt2_1 { get; set; }

        [JsonPropertyName(JsonLocalSensorEnabled)]
        public bool Enabled { get; set; }

        /// <summary>
        /// Returns true if this sensor slot has been configured (has a meaningful pin or type).
        /// </summary>
        [JsonIgnore]
        public bool IsConfigured => Pin0 < UnconfiguredPin || Pin1 < UnconfiguredPin || SensorType > 0;

        /// <summary>
        /// Human-readable type name matching SensorIdList enum values.
        /// </summary>
        [JsonIgnore]
        public string TypeName
        {
            get
            {
                return SensorType switch
                {
                    SensorEnumWater => SensorTypeWater,
                    SensorEnumDht11 => SensorTypeDht11,
                    SensorEnumLight => SensorTypeLight,
                    SensorEnumGps => SensorTypeGps,
                    SensorEnumSystem => SensorTypeSystem,
                    SensorEnumBinaryPresence => SensorTypeBinaryPresence,
                    SensorEnumVoltage => SensorTypeVoltage,
                    SensorEnumReed => SensorTypeReed,
                    _ => SensorTypeUnknown
                };
            }
        }

        /// <summary>
        /// Display name: uses the configured name, or falls back to type name.
        /// </summary>
        [JsonIgnore]
        public string DisplayName => !string.IsNullOrWhiteSpace(Name) ? Name : SensorNotConfigured;

        /// <summary>
        /// Pin summary for the sub-label.
        /// </summary>
        [JsonIgnore]
        public string PinSummary
        {
            get
            {
                if (!IsConfigured || !Enabled)
                    return SensorNotConfigured;

                if (Pin0 < UnconfiguredPin && Pin1 < UnconfiguredPin)
                    return string.Format(PinPairFormat, Pin0, Pin1);

                if (Pin0 < UnconfiguredPin)
                    return string.Format(PinFormat, Pin0);

                return TypeName;
            }
        }

        /// <summary>
        /// True if the sensor is present and enabled in config.
        /// </summary>
        [JsonIgnore]
        public bool IsEnabled => Enabled && IsConfigured;
    }
}
