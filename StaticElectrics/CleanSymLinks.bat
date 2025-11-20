@echo off
REM ============================================================================
REM Delete symbolic links for shared code between projects and Shared directory
REM
REM This script removes all symbolic links created by SymLinks.bat, allowing
REM you to recreate them fresh or switch between symlink and non-symlink modes.
REM
REM IMPORTANT: This script requires administrator privileges to run.
REM Right-click and select "Run as administrator" when executing.
REM ============================================================================

echo Cleaning symbolic links for shared code...
echo.

REM Check for administrator privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: This script requires administrator privileges.
    echo Please right-click and select "Run as administrator"
    echo.
    pause
    exit /b 1
)

REM Delete shared root files
if exist WarningType.h (
    del WarningType.h
    echo Deleted: WarningType.h
) else (
    echo Not found: WarningType.h
)

if exist Config.h (
    del Config.h
    echo Deleted: Config.h
) else (
    echo Not found: Config.h
)

if exist SharedConstants.h (
    del SharedConstants.h
    echo Deleted: SharedConstants.h
) else (
    echo Not found: SharedConstants.h
)

if exist ConfigManager.h (
    del ConfigManager.h
    echo Deleted: ConfigManager.h
) else (
    echo Not found: ConfigManager.h
)

if exist ConfigManager.cpp (
    del ConfigManager.cpp
    echo Deleted: ConfigManager.cpp
) else (
    echo Not found: ConfigManager.cpp
)

if exist BroadcastManager.h (
    del BroadcastManager.h
    echo Deleted: BroadcastManager.h
) else (
    echo Not found: BroadcastManager.h
)

if exist BroadcastManager.cpp (
    del BroadcastManager.cpp
    echo Deleted: BroadcastManager.cpp
) else (
    echo Not found: BroadcastManager.cpp
)

echo.
echo CommandHandler files:

REM Delete CommandHandler files
if exist SharedBaseCommandHandler.h (
    del SharedBaseCommandHandler.h
    echo Deleted: SharedBaseCommandHandler.h
) else (
    echo Not found: SharedBaseCommandHandler.h
)

if exist AckCommandHandler.h (
    del AckCommandHandler.h
    echo Deleted: AckCommandHandler.h
) else (
    echo Not found: AckCommandHandler.h
)

if exist AckCommandHandler.cpp (
    del AckCommandHandler.cpp
    echo Deleted: AckCommandHandler.cpp
) else (
    echo Not found: AckCommandHandler.cpp
)

if exist SystemCommandHandler.h (
    del SystemCommandHandler.h
    echo Deleted: SystemCommandHandler.h
) else (
    echo Not found: SystemCommandHandler.h
)

if exist SystemCommandHandler.cpp (
    del SystemCommandHandler.cpp
    echo Deleted: SystemCommandHandler.cpp
) else (
    echo Not found: SystemCommandHandler.cpp
)

echo.
echo All symbolic links cleaned successfully!
echo.
echo To recreate symbolic links, run: SymLinks.bat
echo.
pause
exit /b 0