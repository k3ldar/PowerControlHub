using Microsoft.Extensions.Logging;
using PowerControlHubApp.Services;
using PowerControlHubApp.ViewModels;
using PowerControlHubApp.Views;
using static PowerControlHubApp.Internal.Constants;

namespace PowerControlHubApp
{
    public static class MauiProgram
    {

        public static MauiApp CreateMauiApp()
        {
            MauiAppBuilder builder = MauiApp.CreateBuilder();
            builder
                .UseMauiApp<App>()
                .ConfigureFonts(fonts =>
                {
                    fonts.AddFont(FontOpenSansRegular, FontOpenSansRegularName);
                    fonts.AddFont(FontSansSemiBold, FontSansSemiBoldName);
                });

            // Infrastructure
            builder.Services.AddSingleton<IMessageBus, MessageBus>();

            // Connections — each owns its own HttpClient (one persistent TCP socket)
            builder.Services.AddSingleton<DashboardConnection>(sp =>
            {
                DashboardConnection connection = new DashboardConnection();
                string ip = Preferences.Get(KeyDeviceIpAddress, string.Empty);
                string port = Preferences.Get(KeyDeviceIpPort, DefaultDeviceIpPort);

                if (!string.IsNullOrEmpty(ip) && int.TryParse(port, out int p))
                {
                    connection.Configure(ip, p);
                    string apiKey = Preferences.Get(KeyAuthApiKey, string.Empty);
                    string hmacKey = Preferences.Get(KeyAuthHmacKey, string.Empty);
                    connection.ConfigureAuth(apiKey, hmacKey);
                }

                return connection;
            });

            builder.Services.AddSingleton<IDashboardConnection>(sp =>
                sp.GetRequiredService<DashboardConnection>());


            // Register ConfigConnection as the inner singleton
            builder.Services.AddSingleton<ConfigConnection>(sp =>
            {
                ConfigConnection connection = new ConfigConnection(
                    sp.GetRequiredService<IMessageBus>(),
                    sp.GetRequiredService<ILogger<ConfigConnection>>());

                string ip = Preferences.Get(KeyDeviceIpAddress, string.Empty);
                string port = Preferences.Get(KeyDeviceIpPort, DefaultDeviceIpPort);

                if (!string.IsNullOrEmpty(ip) && int.TryParse(port, out int p))
                {
                    connection.Configure(ip, p);
                    string apiKey = Preferences.Get(KeyAuthApiKey, string.Empty);
                    string hmacKey = Preferences.Get(KeyAuthHmacKey, string.Empty);
                    connection.ConfigureAuth(apiKey, hmacKey);
                }

                return connection;
            });

            // Register ConfigPoller as singleton and as IConfigConnection
            builder.Services.AddSingleton<ConfigPoller>(sp =>
                new ConfigPoller(
                    sp.GetRequiredService<ConfigConnection>(),
                    sp.GetRequiredService<IMessageBus>(),
                    sp.GetRequiredService<ILogger<ConfigPoller>>(),
                    sp.GetRequiredService<SensorMetaCache>()));
            builder.Services.AddSingleton<IConfigConnection>(sp => sp.GetRequiredService<ConfigPoller>());

            // Legacy service — kept as thin facade that delegates to the two connections
            builder.Services.AddSingleton<PowerHubService>(sp =>
            {
                PowerHubService service = new PowerHubService(
                    sp.GetRequiredService<IDashboardConnection>(),
                    sp.GetRequiredService<IConfigConnection>(),
                    sp.GetRequiredService<IMessageBus>(),
                    sp.GetRequiredService<IDashboardProvider>());

                return service;
            });

            // Dashboard poller — single instance
            builder.Services.AddSingleton<DashboardPoller>();
            builder.Services.AddSingleton<IDashboardProvider>(sp => sp.GetRequiredService<DashboardPoller>());
            builder.Services.AddSingleton<SensorMetaCache>();
            builder.Services.AddSingleton<LogService>();
            builder.Services.AddSingleton<ThemeService>();
            builder.Services.AddSingleton<LanguageService>();
            builder.Services.AddSingleton<RelayStore>();
            builder.Services.AddSingleton<TimeSyncService>();

            // ViewModels
            builder.Services.AddSingleton<DashboardViewModel>();
            builder.Services.AddSingleton<SettingsViewModel>();
            builder.Services.AddSingleton<RelayConfigViewModel>();
            builder.Services.AddSingleton<SystemViewModel>();
            builder.Services.AddTransient<RelayDetailViewModel>();
            builder.Services.AddSingleton<ExternalSensorConfigViewModel>();
            builder.Services.AddTransient<ExternalSensorDetailViewModel>();
            builder.Services.AddSingleton<LocalSensorConfigViewModel>();
            builder.Services.AddTransient<LocalSensorDetailViewModel>();
            builder.Services.AddTransient<TimeSettingsViewModel>();
            builder.Services.AddTransient<LocationSettingsViewModel>();
            builder.Services.AddTransient<MqttSettingsViewModel>();
            builder.Services.AddTransient<SdCardSettingsViewModel>();
            builder.Services.AddTransient<RtcSettingsViewModel>();
            builder.Services.AddTransient<NetworkSecurityViewModel>();
            builder.Services.AddTransient<NextionSettingsViewModel>();
            builder.Services.AddTransient<XpdzToneSettingsViewModel>();

            builder.Services.AddTransient<DashboardPage>();
            builder.Services.AddTransient<SettingsPage>();
            builder.Services.AddTransient<RelayConfigPage>();
            builder.Services.AddTransient<SystemPage>();
            builder.Services.AddTransient<RelayDetailPage>();
            builder.Services.AddTransient<ExternalSensorConfigPage>();
            builder.Services.AddTransient<ExternalSensorDetailPage>();
            builder.Services.AddTransient<LocalSensorConfigPage>();
            builder.Services.AddTransient<LocalSensorDetailPage>();
            builder.Services.AddTransient<TimeSettingsPage>();
            builder.Services.AddTransient<LocationSettingsPage>();
            builder.Services.AddTransient<MqttSettingsPage>();
            builder.Services.AddTransient<SdCardSettingsPage>();
            builder.Services.AddTransient<RtcSettingsPage>();
            builder.Services.AddTransient<NetworkSecurityPage>();
            builder.Services.AddTransient<NextionSettingsPage>();
            builder.Services.AddTransient<XpdzToneSettingsPage>();

#if DEBUG
            builder.Logging.AddDebug();
#endif

            return builder.Build();
        }
    }
}
