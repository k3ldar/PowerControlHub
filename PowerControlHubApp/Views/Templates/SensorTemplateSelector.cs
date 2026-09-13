using PowerControlHubApp.Models;
using PowerControlHubApp.Models.Json;

namespace PowerControlHubApp.Views.Templates;

/// <summary>
/// Selects the correct sensor card DataTemplate based on the sensor's IdType.
/// Each template property is set from DashboardPage.xaml resources.
/// </summary>
public class SensorTemplateSelector : DataTemplateSelector
{
    public DataTemplate Dht11Template { get; set; }
    public DataTemplate GpsTemplate { get; set; }
    public DataTemplate LightTemplate { get; set; }
    public DataTemplate VoltageTemplate { get; set; }
    public DataTemplate WaterTemplate { get; set; }
    public DataTemplate SystemTemplate { get; set; }
    public DataTemplate BinaryPresenceTemplate { get; set; }
    public DataTemplate GenericTemplate { get; set; }

    protected override DataTemplate OnSelectTemplate(object item, BindableObject container)
    {
        if (item is not SensorsModel sensor)
            return GenericTemplate;

        return sensor.SensorType switch
        {
            SensorType.Dht11 => Dht11Template ?? GenericTemplate,
            SensorType.Gps => GpsTemplate ?? GenericTemplate,
            SensorType.Light => LightTemplate ?? GenericTemplate,
            SensorType.Voltage => VoltageTemplate ?? GenericTemplate,
            SensorType.Water => WaterTemplate ?? GenericTemplate,
            SensorType.System => SystemTemplate ?? GenericTemplate,
            SensorType.BinaryPresence => BinaryPresenceTemplate ?? GenericTemplate,
            SensorType.ReedSwitch => BinaryPresenceTemplate ?? GenericTemplate,
            _ => GenericTemplate
        };
    }
}
