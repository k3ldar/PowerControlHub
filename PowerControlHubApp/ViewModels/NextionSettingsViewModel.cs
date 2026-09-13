using PowerControlHubApp.Models.Json;
using PowerControlHubApp.Services;
using System.Collections.ObjectModel;
using System.Windows.Input;
using static PowerControlHubApp.Internal.Constants;
using static PowerControlHubApp.Resources.Localization.AppResources;

namespace PowerControlHubApp.ViewModels;

public sealed class NextionSettingsViewModel : BaseViewModel
{
    private bool _isEnabled;
    private bool _isHardwareSerial;
    private int _rxPin = UnconfiguredPin;
    private int _txPin = UnconfiguredPin;
    private int _baudRate;
    private int _uartNumber;
    private int _uartSelectedIndex;
    private bool _isRefreshing;
    private bool _isSaving;

    public NextionSettingsViewModel(PowerHubService service, LogService log)
        : base(service, log)
    {
        RefreshCommand = new Command(async () => await RefreshAsync());
        SaveAllCommand = new Command(async () => await SaveAllAsync());
        RebootCommand = new Command(async () => await RebootAsync());
        UartOptions = new ObservableCollection<string>(new[] { NextionUart1, NextionUart2 });
    }

    public ICommand SaveAllCommand { get; }

    public ICommand RebootCommand { get; }

    public bool IsEnabled
    {
        get => _isEnabled;

        set
        {
            _isEnabled = value;
            OnPropertyChanged();
        }
    }

    public bool IsHardwareSerial
    {
        get => _isHardwareSerial;

        set
        {
            _isHardwareSerial = value;
            OnPropertyChanged();
        }
    }

    public int RxPin
    {
        get => _rxPin;

        set
        {
            if (_rxPin == value)
                return;

            _rxPin = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(RxPinDisplay));
        }
    }

    public string RxPinDisplay
    {
        get => _rxPin == UnconfiguredPin ? string.Empty : _rxPin.ToString();

        set
        {
            if (int.TryParse(value, out int p))
                RxPin = p;
            else if (string.IsNullOrWhiteSpace(value))
                RxPin = UnconfiguredPin;
        }
    }

    public int TxPin
    {
        get => _txPin;

        set
        {
            if (_txPin == value)
                return;

            _txPin = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(TxPinDisplay));
        }
    }

    public string TxPinDisplay
    {
        get => _txPin == UnconfiguredPin ? string.Empty : _txPin.ToString();

        set
        {
            if (int.TryParse(value, out int p))
                TxPin = p;
            else if (string.IsNullOrWhiteSpace(value))
                TxPin = UnconfiguredPin;
        }
    }

    public string BaudRate
    {
        get => _baudRate == 0 ? string.Empty : _baudRate.ToString();

        set
        {
            if (int.TryParse(value, out int v))
                _baudRate = v;
            else
                _baudRate = 0;
            OnPropertyChanged();
        }
    }

    public int UartNumber
    {
        get => _uartNumber;

        set
        {
            _uartNumber = value;
            _uartSelectedIndex = value >= 1 ? value - 1 : -1;
            OnPropertyChanged();
            OnPropertyChanged(nameof(UartSelectedIndex));
        }
    }

    public ObservableCollection<string> UartOptions { get; }

    public int UartSelectedIndex
    {
        get => _uartSelectedIndex;

        set
        {
            _uartSelectedIndex = value;
            UartNumber = value >= 0 ? value + 1 : 0;
            OnPropertyChanged();
        }
    }

    public bool IsRefreshing
    {
        get => _isRefreshing;

        set
        {
            _isRefreshing = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(IsNotRefreshing));
        }
    }

    public bool IsNotRefreshing => !_isRefreshing;

    public bool IsSaving
    {
        get => _isSaving;

        set
        {
            _isSaving = value;
            OnPropertyChanged();
        }
    }

    public bool HasStatusMessage => !string.IsNullOrEmpty(StatusMessage);

    public async Task RefreshAsync()
    {
        if (!Service.IsConfigured || _isRefreshing)
            return;

        IsRefreshing = true;

        try
        {
            NextionConfigModel cfg = await Service.GetNextionConfigAsync();

            MainThread.BeginInvokeOnMainThread(() =>
            {
                IsEnabled = cfg.Enabled ?? false;
                IsHardwareSerial = cfg.HardwareSerial ?? false;
                RxPin = cfg.RxPin ?? UnconfiguredPin;
                TxPin = cfg.TxPin ?? UnconfiguredPin;
                _baudRate = cfg.BaudRate ?? 0;
                UartNumber = cfg.UartNumber ?? 0;

                BaudRate = _baudRate == 0 ? string.Empty : _baudRate.ToString();

                IsConnected = true;
                StatusMessage = $"{Refreshed} {DateTime.Now:HH:mm:ss}";
                OnPropertyChanged(nameof(HasStatusMessage));
            });
        }
        catch
        {
            MainThread.BeginInvokeOnMainThread(() =>
            {
                IsConnected = false;
                StatusMessage = MessageDeviceUnreachable;
                OnPropertyChanged(nameof(HasStatusMessage));
            });
        }
        finally
        {
            IsRefreshing = false;
        }
    }

    public async Task SaveAllAsync()
    {
        if (!Service.IsConfigured || _isSaving)
            return;

        _isSaving = true;
        bool anyFailed = false;

        try
        {
            anyFailed |= !await Service.SetNextionEnabledAsync(IsEnabled);
            anyFailed |= !await Service.SetNextionHardwareSerialAsync(IsHardwareSerial);
            anyFailed |= !await Service.SetNextionRxPinAsync(RxPin == UnconfiguredPin ? UnconfiguredPin : RxPin);
            anyFailed |= !await Service.SetNextionTxPinAsync(TxPin == UnconfiguredPin ? UnconfiguredPin : TxPin);

            if (int.TryParse(BaudRate, out int br) && br > 0)
                anyFailed |= !await Service.SetNextionBaudRateAsync(br);

            if (UartNumber > 0)
                anyFailed |= !await Service.SetNextionUartNumberAsync(UartNumber);

            await Service.SaveSettingsAsync();

            MainThread.BeginInvokeOnMainThread(() =>
            {
                if (anyFailed)
                    StatusMessage = SaveFailed;
                else
                    StatusMessage = NextionSettingsSaved;

                OnPropertyChanged(nameof(HasStatusMessage));
            });
        }
        catch
        {
            MainThread.BeginInvokeOnMainThread(() =>
            {
                StatusMessage = SaveFailed;
                OnPropertyChanged(nameof(HasStatusMessage));
            });
        }
        finally
        {
            _isSaving = false;
        }
    }

    public async Task RebootAsync()
    {
        bool confirmed = await ConfirmAsync(NextionRebootTitle, NextionRebootMessage, Reboot, Cancel);

        if (!confirmed)
            return;

        try
        {
            await Service.SaveSettingsAsync();
            await AlertAsync(Reboot, NextionRebootSavedMessage, OK);
        }
        catch
        {
            await AlertAsync(Reboot, NextionRebootFailed, OK);
        }
    }

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Performance", "CA1822", Justification = "Called polymorphically from OnDisappearing")]
    public void Cleanup() { }

    protected override async Task ExecuteRefreshAsync(CancellationToken ct)
    {
        await RefreshAsync();
    }

    protected override void OnDataFetched(IndexModel index) { }
}
