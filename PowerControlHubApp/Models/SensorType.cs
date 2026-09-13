namespace PowerControlHubApp.Models;

/// <summary>
/// Mirrors the firmware's SensorIdList enum (SystemDefinitions.h).
/// Used to drive DataTemplateSelector without relying on the user-configurable Name string.
/// </summary>
public enum SensorType : byte
{
    Water = 0x0,
    Dht11 = 0x1,
    Light = 0x2,
    Gps = 0x3,
    System = 0x4,
    BinaryPresence = 0x5,
    Voltage = 0x6,
    ReedSwitch = 0x7,
    Unknown = 0xFF
}
