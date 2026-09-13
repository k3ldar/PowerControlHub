using PowerControlHubApp.Models.Json;
using PowerControlHubApp.Services;
using System.Collections.ObjectModel;
using System.Windows.Input;
using static PowerControlHubApp.Internal.Constants;
using static PowerControlHubApp.Resources.Localization.AppResources;

namespace PowerControlHubApp.ViewModels;

public class SystemViewModel : BaseViewModel
{
    private bool _isRefreshing;
    private string _pinsInUseText = string.Empty;
    private bool _isPinsRefreshing;
    private int _pinsCount;
    private string _selectedLanguage;

    // F16 Pin Restrictions (compile-time — fetched once, never changes)
    private string _hardPinsText = string.Empty;
    private string _advisoryPinsText = string.Empty;
    private bool _hasPinRestrictions;
    private bool _pinRestrictionsLoaded;

    // OTA
    private OtaStatusModel _otaStatus;
    private bool _isCheckingOta;

    public ObservableCollection<string> Warnings { get; } = [];

    public string PinsInUseText
    {
        get => _pinsInUseText;
        set
        {
            _pinsInUseText = value;
            OnPropertyChanged();
        }
    }

    public int PinsCount
    {
        get => _pinsCount;
        set
        {
            _pinsCount = value;
            OnPropertyChanged();
        }
    }

    public bool IsPinsRefreshing
    {
        get => _isPinsRefreshing;
        set
        {
            _isPinsRefreshing = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(IsNotPinsRefreshing));
        }
    }

    public bool IsNotPinsRefreshing => !_isPinsRefreshing;

    public bool HasPins => PinsCount > 0;

    public string HardPinsText
    {
        get => _hardPinsText;

        set
        {
            _hardPinsText = value;
            OnPropertyChanged();
        }
    }

    public string AdvisoryPinsText
    {
        get => _advisoryPinsText;

        set
        {
            _advisoryPinsText = value;
            OnPropertyChanged();
        }
    }

    public bool HasPinRestrictions
    {
        get => _hasPinRestrictions;

        set
        {
            _hasPinRestrictions = value;
            OnPropertyChanged();
        }
    }

    public IReadOnlyList<string> LanguageOptions => LanguageService.LanguageOptions;

    public string SelectedLanguage
    {
        get => _selectedLanguage;
        set
        {
            if (_selectedLanguage == value)
                return;

            _selectedLanguage = value;
            OnPropertyChanged();
            LanguageService.ApplyOption(value);
        }
    }

    public bool HasStatusMessage => !string.IsNullOrEmpty(StatusMessage);

    public ICommand CheckForUpdateCommand { get; }

    public ICommand RefreshPinsCommand { get; }

    public ICommand InstallFirmwareCommand { get; }

    public ICommand NavigateToTimeSettingsCommand { get; }

    public ICommand NavigateToMqttSettingsCommand { get; }

    public ICommand NavigateToSdCardSettingsCommand { get; }

    public ICommand NavigateToRtcSettingsCommand { get; }

    public ICommand NavigateToNetworkSecurityCommand { get; }

    public ICommand NavigateToNextionSettingsCommand { get; }

    public ICommand NavigateToXpdzToneSettingsCommand { get; }

    public ICommand NavigateToLocationSettingsCommand { get; }

    public SystemViewModel(PowerHubService service, LogService log)
        : base(service, log)
    {
        _selectedLanguage = LanguageService.CurrentOption;
        CheckForUpdateCommand = new Command(async () => await CheckForUpdateAsync());
        RefreshPinsCommand = new Command(async () => await RefreshPinsAsync());
        InstallFirmwareCommand = new Command(async () => await InstallFirmwareAsync(), () => CanInstallFirmware);
        NavigateToTimeSettingsCommand = new Command(async () => await Shell.Current.GoToAsync(RouteTimeSettingsPage));
        NavigateToMqttSettingsCommand = new Command(async () => await Shell.Current.GoToAsync(RouteMqttSettingsPage));
        NavigateToSdCardSettingsCommand = new Command(async () => await Shell.Current.GoToAsync(RouteSdCardSettingsPage));
        NavigateToRtcSettingsCommand = new Command(async () => await Shell.Current.GoToAsync(RouteRtcSettingsPage));
        NavigateToNetworkSecurityCommand = new Command(async () => await Shell.Current.GoToAsync(RouteNetworkSecurityPage));
        NavigateToNextionSettingsCommand = new Command(async () => await Shell.Current.GoToAsync(RouteNextionSettingsPage));
        NavigateToXpdzToneSettingsCommand = new Command(async () => await Shell.Current.GoToAsync(RouteXpdzToneSettingsPage));
        NavigateToLocationSettingsCommand = new Command(async () => await Shell.Current.GoToAsync(RouteLocationSettingsPage));
    }

    protected override void OnDataFetched(IndexModel index)
    {
        // System page uses its own RefreshAsync flow, not the dashboard auto-refresh
    }

    public async Task RefreshPinsAsync()
    {
        if (!Service.IsConfigured || _isPinsRefreshing)
            return;

        IsPinsRefreshing = true;

        try
        {
            SystemPinsResponseModel result = await Service.GetSystemPinsAsync();

            if (result?.Success == true && result.Pins?.Count > 0)
            {
                MainThread.BeginInvokeOnMainThread(() =>
                {
                    PinsCount = result.Pins.Count;
                    PinsInUseText = string.Join(CommaSpace, result.Pins);
                });
            }
            else
            {
                MainThread.BeginInvokeOnMainThread(() =>
                {
                    PinsCount = 0;
                    PinsInUseText = DoubleDash;
                });
            }
        }
        catch
        {
            MainThread.BeginInvokeOnMainThread(() =>
            {
                PinsCount = 0;
                PinsInUseText = MessageDeviceUnreachable;
            });
        }
        finally
        {
            IsPinsRefreshing = false;
        }
    }

    public async Task RefreshPinRestrictionsAsync()
    {
        if (!Service.IsConfigured || _pinRestrictionsLoaded)
            return;

        try
        {
            SystemPinRestrictionsResponseModel result = await Service.GetSystemPinRestrictionsAsync();

            if (result?.Success == true)
            {
                MainThread.BeginInvokeOnMainThread(() =>
                {
                    HardPinsText = result.Hard?.Count > 0
                        ? string.Join(CommaSpace, result.Hard)
                        : DoubleDash;

                    AdvisoryPinsText = result.Advisory?.Count > 0
                        ? string.Join(CommaSpace, result.Advisory)
                        : DoubleDash;

                    HasPinRestrictions = result.Hard?.Count > 0 || result.Advisory?.Count > 0;
                    _pinRestrictionsLoaded = true;
                });
            }
        }
        catch
        {
            // Silently ignore — pin restrictions are optional info
        }
    }

    public async Task RefreshAsync()
    {
        if (!Service.IsConfigured || _isRefreshing)
            return;

        _isRefreshing = true;

        try
        {
            // Fetch pin restrictions once (compile-time data, doesn't change)
            if (!_pinRestrictionsLoaded)
                await RefreshPinRestrictionsAsync();

            List<string> list = await Service.GetWarningsAsync();
            MainThread.BeginInvokeOnMainThread(() =>
            {
                Warnings.Clear();

                if (list != null && list.Count > 0)
                {
                    foreach (string w in list) Warnings.Add(w);
                    StatusMessage = $"Updated {DateTime.Now:HH:mm:ss}";
                }
                else
                {
                    StatusMessage = MessageNoActiveWarnings;
                }

                OnPropertyChanged(nameof(HasStatusMessage));
            });
        }
        catch (Exception ex)
        {
            Log.Error($"Fetching warnings failed: {ex.Message}");
            MainThread.BeginInvokeOnMainThread(() =>
            {
                Warnings.Clear();
                StatusMessage = MessageDeviceUnreachable;
                OnPropertyChanged(nameof(HasStatusMessage));
            });
        }
        finally
        {
            _isRefreshing = false;
        }
    }

    public string CurrentFirmwareVersion => _otaStatus?.CurrentVersion ?? DoubleDash;

    public string AvailableFirmwareVersion => string.IsNullOrEmpty(_otaStatus?.AvailableVersion) ? DoubleDash : _otaStatus.AvailableVersion;

    public bool IsUpdateAvailable => _otaStatus?.UpdateAvailable == true;

    public bool IsOtaBusy => (_otaStatus?.IsBusy == true) || _isCheckingOta;

    public bool CanInstallFirmware => _otaStatus?.UpdateAvailable == true && !_isCheckingOta;

    public async Task CheckForUpdateAsync()
    {
        if (!Service.IsConfigured || _isCheckingOta)
            return;

        _isCheckingOta = true;
        try
        {
            OtaStatusModel ota = await Service.GetOtaStatusAsync();
            _otaStatus = ota;
            NotifyOtaProperties();
        }
        catch
        {
            StatusMessage = MessageDeviceUnreachable;
        }
        finally
        {
            _isCheckingOta = false;
            OnPropertyChanged(nameof(IsOtaBusy));
            ((Command)InstallFirmwareCommand).ChangeCanExecute();
        }
    }

    public async Task InstallFirmwareAsync()
    {
        if (!CanInstallFirmware)
            return;

        string message = string.Format(OtaDialogMessage, AvailableFirmwareVersion);
        bool confirmed = await ConfirmAsync(OtaDialogTitle, message, OtaDialogAccept, MsgCancel);

        if (!confirmed)
            return;

        try
        {
            bool ok = await Service.TriggerOtaInstallAsync();

            if (ok)
            {
                // give the device a moment then refresh status
                await Task.Delay(TimeSpan.FromSeconds(SecondsTwo));
                await CheckForUpdateAsync();
            }
            else
            {
                StatusMessage = OtaTriggerFailed;
            }
        }
        catch
        {
            StatusMessage = OtaTriggerFailed;
        }
    }

    private void NotifyOtaProperties()
    {
        OnPropertyChanged(nameof(CurrentFirmwareVersion));
        OnPropertyChanged(nameof(AvailableFirmwareVersion));
        OnPropertyChanged(nameof(IsUpdateAvailable));
        OnPropertyChanged(nameof(IsOtaBusy));
        OnPropertyChanged(nameof(CanInstallFirmware));
        ((Command)InstallFirmwareCommand).ChangeCanExecute();
    }
}
