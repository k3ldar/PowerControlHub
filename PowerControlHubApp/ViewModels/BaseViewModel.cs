using PowerControlHubApp.Models;
using PowerControlHubApp.Models.Json;
using PowerControlHubApp.Services;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Input;
using static PowerControlHubApp.Internal.Constants;
using static PowerControlHubApp.Resources.Localization.AppResources;

namespace PowerControlHubApp.ViewModels;

public abstract class BaseViewModel : INotifyPropertyChanged
{
    private readonly LogService _log;
    private readonly PowerHubService _service;
    private CancellationTokenSource _refreshCts;
    private bool _isBusy;
    private bool _isConnected;
    private string _deviceUrl = string.Empty;
    private string _statusMessage = string.Empty;

    // System properties
    private string _systemFreeMemory = DoubleDash;
    private string _systemCpuUsage = DoubleDash;
    private string _systemTime = DoubleDash;
    private string _systemFirmware = DoubleDash;
    private string _systemUptime = DoubleDash;
    private bool _hasWarnings;
    private bool _isApplyingRemoteState = true;
    private bool _hasData;

    protected BaseViewModel(PowerHubService service, LogService log)
    {
        _service = service ?? throw new ArgumentNullException(nameof(service));
        _log = log ?? throw new ArgumentNullException(nameof(log));
        RefreshCommand = new Command(async () => await ExecuteRefreshAsync(CancellationToken.None));
        SystemLabelTappedCommand = new Command(async () => await Shell.Current.GoToAsync(RouteSystemPage));
        FirmwareLabelTappedCommand = new Command(async () => await Shell.Current.GoToAsync(RouteSystemPage));
    }

    protected LogService Log => _log;
    protected PowerHubService Service => _service;
    protected IndexModel LastKnownIndex => _service.CurrentIndex;

    public ObservableCollection<LogEntryViewModel> LogEntries => _log.Entries;

    public virtual ICommand RefreshCommand { get; protected set; }
    public ICommand SystemLabelTappedCommand { get; }
    public ICommand FirmwareLabelTappedCommand { get; }

    public bool IsBusy
    {
        get => _isBusy;
        set
        {
            _isBusy = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(IsNotBusy));
        }
    }

    public bool IsNotBusy => !_isBusy;

    public string StatusMessage
    {
        get => _statusMessage;
        set
        {
            _statusMessage = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(HasStatus));
        }
    }

    public bool HasStatus => !string.IsNullOrEmpty(_statusMessage);

    public bool IsConnected
    {
        get => _isConnected;
        set
        {
            _isConnected = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(IsDisconnected));
            OnPropertyChanged(nameof(StatusColor));
        }
    }

    public bool IsDisconnected => !_isConnected;

    public Color StatusColor => _isConnected ? Color.FromArgb(ColorAsHex1) : Color.FromArgb(ColorError);

    public string DeviceUrl
    {
        get => _deviceUrl;
        set
        {
            _deviceUrl = value;
            OnPropertyChanged();
        }
    }

    public string SystemFreeMemory
    {
        get => _systemFreeMemory;
        private set
        {
            _systemFreeMemory = value;
            OnPropertyChanged();
        }
    }

    public string SystemCpuUsage
    {
        get => _systemCpuUsage;
        private set
        {
            _systemCpuUsage = value;
            OnPropertyChanged();
        }
    }

    public string SystemTime
    {
        get => _systemTime;
        private set
        {
            _systemTime = value;
            OnPropertyChanged();
        }
    }

    public string SystemFirmware
    {
        get => _systemFirmware;
        private set
        {
            _systemFirmware = value;
            OnPropertyChanged();
        }
    }

    public string SystemUptime
    {
        get => _systemUptime;
        private set
        {
            _systemUptime = value;
            OnPropertyChanged();
        }
    }

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Performance", "CA1822", Justification = "Exposed to XAML bindings as instance property")]
    public Color SystemFirmwareColor => Color.FromArgb(ColorAsHex1);

    public bool HasWarnings
    {
        get => _hasWarnings;
        private set
        {
            _hasWarnings = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(SystemStatusColor));
        }
    }

    public Color SystemStatusColor => HasWarnings ? Color.FromArgb(ColorError) : Color.FromArgb(ColorAsHex1);

    /// <summary>
    /// When true the view should ignore events raised by the UI because the
    /// viewmodel is applying authoritative state from the remote device.
    /// </summary>
    public bool IsApplyingRemoteState
    {
        get => _isApplyingRemoteState;
        private set
        {
            _isApplyingRemoteState = value;
            OnPropertyChanged();
        }
    }

    public void ClearLog() => _log.Clear();

    public void StartAutoRefresh()
    {
        StopAutoRefresh();
        _refreshCts = new CancellationTokenSource();
        _ = AutoRefreshLoopAsync(_refreshCts.Token);
    }

    public void StopAutoRefresh()
    {
        _refreshCts?.Cancel();
        _refreshCts?.Dispose();
        _refreshCts = null;
    }

    private async Task AutoRefreshLoopAsync(CancellationToken ct)
    {
        while (!ct.IsCancellationRequested)
        {
            try
            {
                await ExecuteRefreshAsync(ct);
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (Exception ex)
            {
                _log.Error($"Auto-refresh failed: {ex.Message}");
            }

            try
            {
                await Task.Delay(TimeSpan.FromSeconds(1), ct);
            }
            catch (TaskCanceledException)
            {
                break;
            }
        }
    }

    // ── Refresh pipeline ──────────────────────────────────────────────────

    /// <summary>
    /// Common refresh flow: checks configuration, fetches <see cref="IndexModel"/>,
    /// updates system status, then calls <see cref="OnDataFetched"/> for
    /// view-model-specific processing. Handles error cases consistently.
    /// </summary>
    protected virtual async Task ExecuteRefreshAsync(CancellationToken ct)
    {
        if (!_service.IsConfigured)
        {
            StatusMessage = MessageNotConfigured;
            IsConnected = false;
            return;
        }

        if (IsBusy)
            return;

        ApplyCachedIfNeeded();

        IsBusy = true;
        DeviceUrl = _service.BaseUrl;

        try
        {
            IndexModel index = await _service.GetDashboardDataAsync(ct);

            MainThread.BeginInvokeOnMainThread(() =>
            {
                IsApplyingRemoteState = true;
                try
                {
                    ApplyIndex(index);
                    _hasData = true;
                    IsConnected = true;
                    StatusMessage = $"Updated {DateTime.Now:HH:mm:ss}";
                }
                finally
                {
                    IsApplyingRemoteState = false;
                }
            });
        }
        catch (DeviceResponseException ex)
        {
            // Device is reachable but sent bad data — keep last known values visible
            _log.Warning(ex.Message);
            MainThread.BeginInvokeOnMainThread(() =>
            {
                StatusMessage = $"Bad data {DateTime.Now:HH:mm:ss} — see log";
            });
        }
        catch (Exception ex)
        {
            // Network / timeout failure — device unreachable
            _log.Error($"Connection failed: {ex.Message}");
            MainThread.BeginInvokeOnMainThread(() =>
            {
                IsConnected = false;
                StatusMessage = MessageDeviceUnreachable;
            });
        }
        finally
        {
            IsBusy = false;
        }
    }

    private void ApplyIndex(IndexModel index)
    {
        UpdateSystem(index.System);
        UpdateHasWarnings(index.Warning);
        OnDataFetched(index);
    }

    private void ApplyCachedIfNeeded()
    {
        if (_hasData)
            return;

        IndexModel cached = _service.CurrentIndex;

        if (cached is null)
            return;

        _hasData = true;

        MainThread.BeginInvokeOnMainThread(() =>
        {
            IsApplyingRemoteState = true;
            try
            {
                ApplyIndex(cached);
            }
            finally
            {
                IsApplyingRemoteState = false;
            }
        });
    }

    /// <summary>
    /// Override to apply view-model-specific updates after the common data
    /// has been fetched and system status has been written.
    /// </summary>
    protected abstract void OnDataFetched(IndexModel index);

    private void UpdateSystem(SystemModel system)
    {
        SystemFreeMemory = $"{Math.Round((double)system.Mem / KilobyteBytes, DefaultDecimalPlaces)} kb";
        SystemCpuUsage = $"{system.Cpu}%";
        SystemFirmware = string.IsNullOrEmpty(system.Fw) ? DoubleDash : system.Fw;
        SystemUptime = string.IsNullOrEmpty(system.Uptime) ? DoubleDash : system.Uptime;

        // Device date/time — if firmware didn't set time, show placeholder
        try
        {
            if (system.Time.Year >= MinimumValidDateTimeYear)
                SystemTime = system.Time.ToString(DeviceTimeFormat);
            else
                SystemTime = DoubleDash;
        }
        catch
        {
            SystemTime = DoubleDash;
        }
    }

    private void UpdateHasWarnings(WarningModel warning)
    {
        HasWarnings = warning != null && !string.IsNullOrEmpty(warning.Active) &&
                      !warning.Active.Equals(NullByte, StringComparison.OrdinalIgnoreCase) &&
                      !warning.Active.Equals(NibbleZero, StringComparison.OrdinalIgnoreCase);
    }

    protected static Task<bool> ConfirmAsync(string title, string message, string accept, string cancel)
    {
        Page page = CurrentPage();

        if (page is null)
            return Task.FromResult(false);

        return page.DisplayAlertAsync(title, message, accept, cancel);
    }

    protected static Task AlertAsync(string title, string message, string cancel)
    {
        Page page = CurrentPage();

        if (page is null)
            return Task.CompletedTask;

        return page.DisplayAlertAsync(title, message, cancel);
    }

    private static Page CurrentPage()
    {
        return Application.Current?.Windows.FirstOrDefault()?.Page ?? Shell.Current?.CurrentPage;
    }

    public event PropertyChangedEventHandler PropertyChanged;

    protected void OnPropertyChanged([CallerMemberName] string name = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
