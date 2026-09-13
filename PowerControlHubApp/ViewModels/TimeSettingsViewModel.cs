using PowerControlHubApp.Models.Json;
using PowerControlHubApp.Services;
using System.Collections.ObjectModel;
using System.Windows.Input;
using static PowerControlHubApp.Internal.Constants;
using static PowerControlHubApp.Resources.Localization.AppResources;

namespace PowerControlHubApp.ViewModels;

public sealed class TimeSettingsViewModel : BaseViewModel
{
    private const int DstCheckIntervalMinutes = 30;
    private const string PlusSign = "+";
    private const string MsgTimeSyncedToDevice = "Time synced to device";
    private const string MsgSyncFailedDeviceUnreachable = "Sync failed — device unreachable";
    private const string MsgTimezoneSaved = "Timezone saved (UTC";
    private const string MsgSaveFailedDeviceUnreachable = "Save failed — device unreachable";
    private const string MsgTimezoneNotAvailable = "Timezone not available on this device";
    private const string MsgDstAdjusted = "DST adjusted (UTC";

    private string _deviceTime = DoubleDash;
    private int _selectedTimezoneIndex = -1;
    private bool _isRefreshing;
    private PeriodicTimer _dstTimer;
    private CancellationTokenSource _dstCts;
    private PeriodicTimer _clockTimer;
    private CancellationTokenSource _clockCts;
    private DateTime _deviceTimeAtCapture;
    private long _captureUtcTicks;
    private const int ClockTickIntervalSeconds = 1;

    public sealed class TimeZoneOption
    {
        public string DisplayName { get; init; }
        public string TimeZoneId { get; init; }
        public override string ToString() => DisplayName;
    }

#pragma warning disable CC0009
    public static readonly ReadOnlyCollection<TimeZoneOption> TimeZoneOptionsList = new(
    [
        new() { DisplayName = "UTC-12 (Baker Island)", TimeZoneId = "Etc/GMT+12" },
        new() { DisplayName = "UTC-11 (Midway)", TimeZoneId = "Etc/GMT+11" },
        new() { DisplayName = "UTC-10 (Hawaii)", TimeZoneId = "Pacific/Honolulu" },
        new() { DisplayName = "UTC-9 (Alaska)", TimeZoneId = "America/Anchorage" },
        new() { DisplayName = "UTC-8 (Pacific)", TimeZoneId = "America/Los_Angeles" },
        new() { DisplayName = "UTC-7 (Mountain)", TimeZoneId = "America/Denver" },
        new() { DisplayName = "UTC-6 (Central)", TimeZoneId = "America/Chicago" },
        new() { DisplayName = "UTC-5 (Eastern)", TimeZoneId = "America/New_York" },
        new() { DisplayName = "UTC-4 (Atlantic)", TimeZoneId = "America/Halifax" },
        new() { DisplayName = "UTC-3 (Sao Paulo)", TimeZoneId = "America/Sao_Paulo" },
        new() { DisplayName = "UTC-2 (Fernando de Noronha)", TimeZoneId = "Etc/GMT+2" },
        new() { DisplayName = "UTC-1 (Azores)", TimeZoneId = "Atlantic/Azores" },
        new() { DisplayName = "UTC+0 (London)", TimeZoneId = "Europe/London" },
        new() { DisplayName = "UTC+1 (Paris / Copenhagen)", TimeZoneId = "Europe/Paris" },
        new() { DisplayName = "UTC+2 (Helsinki / Athens)", TimeZoneId = "Europe/Helsinki" },
        new() { DisplayName = "UTC+3 (Moscow)", TimeZoneId = "Europe/Moscow" },
        new() { DisplayName = "UTC+4 (Dubai)", TimeZoneId = "Asia/Dubai" },
        new() { DisplayName = "UTC+5 (Karachi)", TimeZoneId = "Asia/Karachi" },
        new() { DisplayName = "UTC+6 (Dhaka)", TimeZoneId = "Asia/Dhaka" },
        new() { DisplayName = "UTC+7 (Bangkok)", TimeZoneId = "Asia/Bangkok" },
        new() { DisplayName = "UTC+8 (Hong Kong / Singapore)", TimeZoneId = "Asia/Singapore" },
        new() { DisplayName = "UTC+9 (Tokyo)", TimeZoneId = "Asia/Tokyo" },
        new() { DisplayName = "UTC+10 (Sydney)", TimeZoneId = "Australia/Sydney" },
        new() { DisplayName = "UTC+11 (Solomon Islands)", TimeZoneId = "Pacific/Guadalcanal" },
        new() { DisplayName = "UTC+12 (Auckland)", TimeZoneId = "Pacific/Auckland" },
        new() { DisplayName = "UTC+13 (Apia)", TimeZoneId = "Pacific/Apia" },
        new() { DisplayName = "UTC+14 (Kiritimati)", TimeZoneId = "Pacific/Kiritimati" },
    ]);
#pragma warning restore CC0009

    private static readonly TimeSpan DstCheckInterval = TimeSpan.FromMinutes(DstCheckIntervalMinutes);
    private static readonly TimeSpan ClockTickInterval = TimeSpan.FromSeconds(ClockTickIntervalSeconds);

    public TimeSettingsViewModel(PowerHubService service, LogService log)
        : base(service, log)
    {
        RefreshCommand = new Command(async () => await RefreshAsync());
        SyncTimeCommand = new Command(async () => await SyncTimeAsync());
        SaveTimezoneCommand = new Command(async () => await SaveTimezoneAsync());
    }

    public ICommand SyncTimeCommand { get; }
    public ICommand SaveTimezoneCommand { get; }

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Performance", "CA1822", Justification = "Exposed to XAML bindings as instance property")]
    public ReadOnlyCollection<TimeZoneOption> TimeZoneOptions => TimeZoneOptionsList;

    public string DeviceTime
    {
        get => _deviceTime;
        set
        {
            _deviceTime = value;
            OnPropertyChanged();
        }
    }

    public int SelectedTimezoneIndex
    {
        get => _selectedTimezoneIndex;
        set
        {
            _selectedTimezoneIndex = value;
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

    public bool HasStatusMessage => !string.IsNullOrEmpty(StatusMessage);

    public async Task RefreshAsync()
    {
        if (!Service.IsConfigured || _isRefreshing)
            return;

        IndexModel cached = LastKnownIndex;

        if (cached is not null)
            ApplyTimeAndTimezone(cached);

        IsRefreshing = true;

        try
        {
            IndexModel index = await Service.GetDashboardDataAsync();

            MainThread.BeginInvokeOnMainThread(() =>
            {
                ApplyTimeAndTimezone(index);
                StatusMessage = $"Updated {DateTime.Now:HH:mm:ss}";
                OnPropertyChanged(nameof(HasStatusMessage));
            });

            StartClock();
        }
        catch
        {
            MainThread.BeginInvokeOnMainThread(() =>
            {
                DeviceTime = DoubleDash;
                StatusMessage = MessageDeviceUnreachable;
                OnPropertyChanged(nameof(HasStatusMessage));
            });
        }
        finally
        {
            IsRefreshing = false;
        }
    }

    private void ApplyTimeAndTimezone(IndexModel index)
    {
        if (index?.System?.Time.Year >= MinimumValidDateTimeYear)
        {
            _deviceTimeAtCapture = index.System.Time;
            _captureUtcTicks = DateTime.UtcNow.Ticks;
            DeviceTime = _deviceTimeAtCapture.ToString(DeviceTimeFormat);
        }
        else
        {
            DeviceTime = DoubleDash;
        }

        if (index?.Config != null)
            MatchTimezoneIndexByOffset(index.Config.TimezoneOffset);
    }

    private void MatchTimezoneIndexByOffset(int offsetHours)
    {
        for (int i = 0; i < TimeZoneOptionsList.Count; i++)
        {
            try
            {
                TimeZoneInfo tz = TimeZoneInfo.FindSystemTimeZoneById(TimeZoneOptionsList[i].TimeZoneId);
                int currentOffset = (int)tz.GetUtcOffset(new DateTime(DateTime.UtcNow.Ticks, DateTimeKind.Utc)).TotalHours;

                if (currentOffset == offsetHours)
                {
                    SelectedTimezoneIndex = i;
                    return;
                }
            }
            catch
            {
                // timezone not on this platform
            }
        }
    }

    public async Task SyncTimeAsync()
    {
        if (!Service.IsConfigured)
            return;

        long nowUnix = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        bool ok = await Service.SetDateTimeAsync(nowUnix);

        MainThread.BeginInvokeOnMainThread(() =>
        {
            if (ok)
            {
                DateTime utcNow = DateTime.UtcNow;
                int offsetHours = GetSelectedTimezoneOffset();
                _deviceTimeAtCapture = utcNow.AddHours(offsetHours);
                _captureUtcTicks = utcNow.Ticks;
                DeviceTime = _deviceTimeAtCapture.ToString(DeviceTimeFormat);
                StatusMessage = MsgTimeSyncedToDevice;
            }
            else
            {
                StatusMessage = MsgSyncFailedDeviceUnreachable;
            }

            OnPropertyChanged(nameof(HasStatusMessage));
        });

        if (ok)
            StartClock();
    }

    private int GetSelectedTimezoneOffset()
    {
        if (SelectedTimezoneIndex < 0 || SelectedTimezoneIndex >= TimeZoneOptionsList.Count)
            return TimezoneOffsetMin;

        try
        {
            TimeZoneInfo tz = TimeZoneInfo.FindSystemTimeZoneById(TimeZoneOptionsList[SelectedTimezoneIndex].TimeZoneId);
            return (int)tz.GetUtcOffset(new DateTime(DateTime.UtcNow.Ticks, DateTimeKind.Utc)).TotalHours;
        }
        catch
        {
            return TimezoneOffsetMin;
        }
    }

    public async Task SaveTimezoneAsync()
    {
        if (!Service.IsConfigured || SelectedTimezoneIndex < 0 || SelectedTimezoneIndex >= TimeZoneOptionsList.Count)
            return;

        TimeZoneOption option = TimeZoneOptionsList[SelectedTimezoneIndex];

        try
        {
            TimeZoneInfo tz = TimeZoneInfo.FindSystemTimeZoneById(option.TimeZoneId);
            TimeSpan offset = tz.GetUtcOffset(new DateTime(DateTime.UtcNow.Ticks, DateTimeKind.Utc));
            int offsetHours = (int)offset.TotalHours;

            bool ok = await Service.SetTimezoneOffsetAsync(offsetHours);

            MainThread.BeginInvokeOnMainThread(() =>
            {
                if (ok)
                {
                    StatusMessage = $"{MsgTimezoneSaved}{(offsetHours >= 0 ? PlusSign : string.Empty)}{offsetHours})";
                    StartDstAutoAdjust(option.TimeZoneId);
                }
                else
                {
                    StatusMessage = MsgSaveFailedDeviceUnreachable;
                }

                OnPropertyChanged(nameof(HasStatusMessage));
            });
        }
        catch (TimeZoneNotFoundException)
        {
            StatusMessage = MsgTimezoneNotAvailable;
            OnPropertyChanged(nameof(HasStatusMessage));
        }
    }

    private void StartClock()
    {
        StopClock();

        _clockCts = new CancellationTokenSource();
        CancellationToken ct = _clockCts.Token;
        _clockTimer = new PeriodicTimer(ClockTickInterval);

        _ = RunClockLoopAsync(ct);
    }

    private async Task RunClockLoopAsync(CancellationToken ct)
    {
        while (await _clockTimer.WaitForNextTickAsync(ct))
        {
            try
            {
                long elapsedTicks = DateTime.UtcNow.Ticks - _captureUtcTicks;
                DateTime now = _deviceTimeAtCapture.AddTicks(elapsedTicks);

                MainThread.BeginInvokeOnMainThread(() =>
                {
                    DeviceTime = now.ToString(DeviceTimeFormat);
                });
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch
            {
                // silently retry
            }
        }
    }

    private void StopClock()
    {
        _clockCts?.Cancel();
        _clockCts?.Dispose();
        _clockCts = null;
        _clockTimer?.Dispose();
        _clockTimer = null;
    }

    private void StartDstAutoAdjust(string timeZoneId)
    {
        StopDstAutoAdjust();

        _dstCts = new CancellationTokenSource();
        CancellationToken ct = _dstCts.Token;
        _dstTimer = new PeriodicTimer(DstCheckInterval);

        _ = RunDstLoopAsync(timeZoneId, ct);
    }

    private async Task RunDstLoopAsync(string timeZoneId, CancellationToken ct)
    {
        int lastOffset = int.MinValue;

        try
        {
            TimeZoneInfo tz = TimeZoneInfo.FindSystemTimeZoneById(timeZoneId);
            lastOffset = (int)tz.GetUtcOffset(new DateTime(DateTime.UtcNow.Ticks, DateTimeKind.Utc)).TotalHours;
        }
        catch
        {
            return;
        }

        while (await _dstTimer.WaitForNextTickAsync(ct))
        {
            try
            {
                TimeZoneInfo tz = TimeZoneInfo.FindSystemTimeZoneById(timeZoneId);
                int currentOffset = (int)tz.GetUtcOffset(new DateTime(DateTime.UtcNow.Ticks, DateTimeKind.Utc)).TotalHours;

                if (currentOffset != lastOffset)
                {
                    bool ok = await Service.SetTimezoneOffsetAsync(currentOffset, ct);

                    if (ok)
                    {
                        lastOffset = currentOffset;
                        int displayOffset = currentOffset;
                        MainThread.BeginInvokeOnMainThread(() =>
                        {
                            StatusMessage = $"{MsgDstAdjusted}{(displayOffset >= 0 ? PlusSign : string.Empty)}{displayOffset})";
                            OnPropertyChanged(nameof(HasStatusMessage));
                        });
                    }
                }
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch
            {
                // silently retry next tick
            }
        }
    }

    private void StopDstAutoAdjust()
    {
        _dstCts?.Cancel();
        _dstCts?.Dispose();
        _dstCts = null;
        _dstTimer?.Dispose();
        _dstTimer = null;
    }

    public void Cleanup()
    {
        StopClock();
        StopDstAutoAdjust();
    }

    protected override void OnDataFetched(Models.Json.IndexModel index)
    {
        // Not used — this page does its own refresh
    }
}
