using PowerControlHubApp.ViewModels;

namespace PowerControlHubApp.Views;

public partial class StatusBarView : ContentView
{
    private const double WideBreakpoint = 800;
    private const double MediumBreakpoint = 550;

    public StatusBarView()
    {
        InitializeComponent();
    }

    private void OnSystemStatusGridSizeChanged(object sender, EventArgs e)
    {
        var grid = sender as Grid;

        if (grid == null)
            return;

        double width = GetAvailableWidth(grid);

        if (width <= 0)
            return;

        bool showWarning = width >= WideBreakpoint;
        bool showSystem = width >= WideBreakpoint;
        bool showFirmware = width >= WideBreakpoint;
        bool showTime = width >= MediumBreakpoint;

        WarningLabel.IsVisible = showWarning && (WarningLabel.BindingContext as BaseViewModel)?.HasWarnings == true;
        SystemLabel.IsVisible = showSystem;
        FirmwareLabel.IsVisible = showFirmware;
        TimeLabel.IsVisible = showTime;
    }

    private double GetAvailableWidth(Grid grid)
    {
        if (grid.Width > 0)
            return grid.Width;

        if (this.Window != null && this.Window.Width > 0)
            return this.Window.Width;

        var parent = grid.Parent as VisualElement;

        if (parent?.Width > 0)
            return parent.Width;

        return -1;
    }

    private void OnToggleLogClicked(object sender, EventArgs e)
    {
        LogPanel.IsVisible = !LogPanel.IsVisible;
    }

    private void OnClearLogClicked(object sender, EventArgs e)
    {
        if (BindingContext is BaseViewModel vm)
            vm.ClearLog();
    }
}
