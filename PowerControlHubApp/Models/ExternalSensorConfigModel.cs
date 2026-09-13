using System.Text.Json.Serialization;
using static PowerControlHubApp.Internal.Constants;
using static PowerControlHubApp.Resources.Localization.AppResources;

namespace PowerControlHubApp.Models
{
    public sealed class ExternalSensorConfigModel
    {
        // SensorIdList enum values mapped to display names
        private static readonly string[] SensorIdNames = new[]
        {
            SensorTypeWater,       // 0  Water
            SensorTypeDht11,       // 1  DHT11
            SensorTypeLight,       // 2  Light
            SensorTypeGps,         // 3  GPS
            SensorTypeSystem,      // 4  System
            SensorTypeBinaryPresence, // 5  Binary Presence
            SensorTypeVoltage,     // 6  Voltage
            SensorTypeReed,        // 7  Reed
        };

        [JsonPropertyName(JsonSensorIndex)]
        public int Index { get; set; }

        [JsonPropertyName(JsonSensorId)]
        public int SensorId { get; set; }

        /// <summary>
        /// Human-readable name corresponding to the SensorIdList enum value.
        /// </summary>
        [JsonIgnore]
        public string SensorTypeName =>
            SensorId >= 0 && SensorId < SensorIdNames.Length
                ? SensorIdNames[SensorId]
                : SensorTypeUnknown;

        /// <summary>
        /// True if this slot has been configured (has a name or valid SensorId).
        /// </summary>
        [JsonIgnore]
        public bool IsConfigured => !string.IsNullOrWhiteSpace(Name) || SensorId > 0;

        /// <summary>
        /// Display name for the list: uses Name, or falls back to sensor type.
        /// </summary>
        [JsonIgnore]
        public string DisplayName => !string.IsNullOrWhiteSpace(Name) ? Name : SensorTypeName;

        [JsonPropertyName(JsonSensorName)]
        public string Name { get; set; }

        [JsonPropertyName(JsonSensorMqttName)]
        public string MqttName { get; set; }

        [JsonPropertyName(JsonSensorMqttSlug)]
        public string MqttSlug { get; set; }

        [JsonPropertyName(JsonSensorMqttType)]
        public string MqttTypeSlug { get; set; }

        [JsonPropertyName(JsonSensorMqttDeviceClass)]
        public string MqttDeviceClass { get; set; }

        [JsonPropertyName(JsonSensorMqttUnit)]
        public string MqttUnit { get; set; }

        [JsonPropertyName(JsonSensorMqttBinary)]
        public bool MqttIsBinary { get; set; }
    }
}
