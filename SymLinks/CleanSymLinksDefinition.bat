@echo off
setlocal enabledelayedexpansion
REM ============================================================================
REM Delete symbolic links for shared code based on definitions file
REM
REM This script removes all symbolic links listed in a definitions file,
REM allowing you to recreate them fresh or switch between symlink modes.
REM
REM IMPORTANT: This script requires administrator privileges to run.
REM Right-click and select "Run as administrator" when executing.
REM
REM Usage: CleanSymLinksDefinition.bat <SymLinkDefinitionsFile> <TargetDirectory>
REM ============================================================================

set DEFINITIONS_FILE=%~1
set TARGET_DIR=%~2

REM Validate parameters
if "%DEFINITIONS_FILE%"=="" (
    echo ERROR: Definitions file not specified
    echo Usage: CleanSymLinksDefinition.bat ^<SymLinkDefinitionsFile^> ^<TargetDirectory^>
    echo.
    pause
    exit /b 1
)

if "%TARGET_DIR%"=="" (
    echo ERROR: Target directory not specified
    echo Usage: CleanSymLinksDefinition.bat ^<SymLinkDefinitionsFile^> ^<TargetDirectory^>
    echo.
    pause
    exit /b 1
)

REM Verify definitions file exists
if not exist "%DEFINITIONS_FILE%" (
    echo ERROR: Definitions file not found: %DEFINITIONS_FILE%
    echo.
    pause
    exit /b 1
)

REM Verify target directory exists
if not exist "%TARGET_DIR%" (
    echo ERROR: Target directory not found: %TARGET_DIR%
    echo.
    pause
    exit /b 1
)

echo Cleaning symbolic links using: %DEFINITIONS_FILE%
echo Target directory: %TARGET_DIR%
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

REM Process each line from the definitions file
for /f "usebackq tokens=1 delims=|" %%a in ("%DEFINITIONS_FILE%") do (
    set LINK_PATH=%TARGET_DIR%\%%a
    if exist "!LINK_PATH!" (
        del "!LINK_PATH!"
        echo Deleted: %%a
    ) else (
        echo Not found: %%a
    )
)

echo.
echo All symbolic links cleaned successfully!
echo.
exit /b 0