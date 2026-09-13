using Android.App;
using Android.Runtime;
using PowerControlHubApp.Internal;
using System;
using System.Threading.Tasks;

namespace PowerControlHubApp
{
    [Application]
    public class MainApplication : MauiApplication
    {
        public MainApplication(IntPtr handle, JniHandleOwnership ownership)
            : base(handle, ownership)
        {
        }

        protected override MauiApp CreateMauiApp()
        {
            AndroidEnvironment.UnhandledExceptionRaiser += OnAndroidUnhandledException;
            AppDomain.CurrentDomain.UnhandledException += OnUnhandledException;
            TaskScheduler.UnobservedTaskException += OnUnobservedTaskException;

            return MauiProgram.CreateMauiApp();
        }

        private static void OnAndroidUnhandledException(object sender, RaiseThrowableEventArgs e)
        {
            LogException(e.Exception);
        }

        private static void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            LogException(e.ExceptionObject as Exception);
        }

        private static void OnUnobservedTaskException(object sender, UnobservedTaskExceptionEventArgs e)
        {
            LogException(e.Exception);
            e.SetObserved();
        }

        private static void LogException(Exception exception)
        {
            Android.Util.Log.Error(Constants.AndroidLogTag, exception?.ToString() ?? Constants.UnknownExceptionMessage);
        }
    }
}