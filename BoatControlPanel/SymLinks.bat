@echo off
REM ============================================================================
REM Create symbolic links for shared code between StaticElectrics and Shared
REM
REM This script creates symbolic links to shared header and implementation files
REM from the Shared directory, allowing code reuse across projects.
REM
REM IMPORTANT: This script requires administrator privileges to run.
REM Right-click and select "Run as administrator" when executing.
REM ============================================================================

echo Creating symbolic links for shared code...
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

REM Helper function to create symlink with error checking
REM This is implemented inline for each file due to batch file limitations

REM Shared root files
if exist WarningType.h (
    echo Warning: WarningType.h already exists, skipping
) else (
    mklink WarningType.h "..\Shared\WarningType.h"
    if errorlevel 1 (
        echo ERROR: Failed to create WarningType.h symlink
        exit /b 1
    )
    echo Created: WarningType.h
)

if exist Config.h (
    echo Warning: Config.h already exists, skipping
) else (
    mklink Config.h "..\Shared\Config.h"
    if errorlevel 1 (
        echo ERROR: Failed to create Config.h symlink
        exit /b 1
    )
    echo Created: Config.h
)

if exist SharedConstants.h (
    echo Warning: SharedConstants.h already exists, skipping
) else (
    mklink SharedConstants.h "..\Shared\SharedConstants.h"
    if errorlevel 1 (
        echo ERROR: Failed to create SharedConstants.h symlink
        exit /b 1
    )
    echo Created: SharedConstants.h
)

if exist ConfigManager.h (
    echo Warning: ConfigManager.h already exists, skipping
) else (
    mklink ConfigManager.h "..\Shared\ConfigManager.h"
    if errorlevel 1 (
        echo ERROR: Failed to create ConfigManager.h symlink
        exit /b 1
    )
    echo Created: ConfigManager.h
)

if exist ConfigManager.cpp (
    echo Warning: ConfigManager.cpp already exists, skipping
) else (
    mklink ConfigManager.cpp "..\Shared\ConfigManager.cpp"
    if errorlevel 1 (
        echo ERROR: Failed to create ConfigManager.cpp symlink
        exit /b 1
    )
    echo Created: ConfigManager.cpp
)

if exist BroadcastManager.h (
    echo Warning: BroadcastManager.h already exists, skipping
) else (
    mklink BroadcastManager.h "..\Shared\BroadcastManager.h"
    if errorlevel 1 (
        echo ERROR: Failed to create BroadcastManager.h symlink
        exit /b 1
    )
    echo Created: BroadcastManager.h
)

if exist BroadcastManager.cpp (
    echo Warning: BroadcastManager.cpp already exists, skipping
) else (
    mklink BroadcastManager.cpp "..\Shared\BroadcastManager.cpp"
    if errorlevel 1 (
        echo ERROR: Failed to create BroadcastManager.cpp symlink
        exit /b 1
    )
    echo Created: BroadcastManager.cpp
)

echo.
echo CommandHandler files:

REM CommandHandler files
if exist SharedBaseCommandHandler.h (
    echo Warning: SharedBaseCommandHandler.h already exists, skipping
) else (
    mklink SharedBaseCommandHandler.h "..\Shared\CommandHandlers\SharedBaseCommandHandler.h"
    if errorlevel 1 (
        echo ERROR: Failed to create SharedBaseCommandHandler.h symlink
        exit /b 1
    )
    echo Created: SharedBaseCommandHandler.h
)

if exist AckCommandHandler.h (
    echo Warning: AckCommandHandler.h already exists, skipping
) else (
    mklink AckCommandHandler.h "..\Shared\CommandHandlers\AckCommandHandler.h"
    if errorlevel 1 (
        echo ERROR: Failed to create AckCommandHandler.h symlink
        exit /b 1
    )
    echo Created: AckCommandHandler.h
)

if exist AckCommandHandler.cpp (
    echo Warning: AckCommandHandler.cpp already exists, skipping
) else (
    mklink AckCommandHandler.cpp "..\Shared\CommandHandlers\AckCommandHandler.cpp"
    if errorlevel 1 (
        echo ERROR: Failed to create AckCommandHandler.cpp symlink
        exit /b 1
    )
    echo Created: AckCommandHandler.cpp
)

if exist SystemCommandHandler.h (
    echo Warning: SystemCommandHandler.h already exists, skipping
) else (
    mklink SystemCommandHandler.h "..\Shared\CommandHandlers\SystemCommandHandler.h"
    if errorlevel 1 (
        echo ERROR: Failed to create SystemCommandHandler.h symlink
        exit /b 1
    )
    echo Created: SystemCommandHandler.h
)

if exist SystemCommandHandler.cpp (
    echo Warning: SystemCommandHandler.cpp already exists, skipping
) else (
    mklink SystemCommandHandler.cpp "..\Shared\CommandHandlers\SystemCommandHandler.cpp"
    if errorlevel 1 (
        echo ERROR: Failed to create SystemCommandHandler.cpp symlink
        exit /b 1
    )
    echo Created: SystemCommandHandler.cpp
)

echo.
echo All symbolic links created successfully!
echo.
pause
exit /b 0