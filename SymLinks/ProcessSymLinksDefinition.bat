@echo off
REM ============================================================================
REM Create symbolic links for shared code between projects and Shared
REM
REM This script creates symbolic links to shared header and implementation files
REM from the Shared directory, allowing code reuse across projects.
REM
REM IMPORTANT: This script requires administrator privileges to run.
REM Right-click and select "Run as administrator" when executing.
REM
REM Usage: ProcessSymLinksDefinition.bat <SymLinkDefinitionsFile> <TargetDirectory>
REM ============================================================================

set DEFINITIONS_FILE=%~1
set TARGET_DIR=%~2

REM Validate parameters
if "%DEFINITIONS_FILE%"=="" (
    echo ERROR: Definitions file not specified
    echo Usage: ProcessSymLinksDefinition.bat ^<SymLinkDefinitionsFile^> ^<TargetDirectory^>
    echo.
    pause
    exit /b 1
)

if "%TARGET_DIR%"=="" (
    echo ERROR: Target directory not specified
    echo Usage: ProcessSymLinksDefinition.bat ^<SymLinkDefinitionsFile^> ^<TargetDirectory^>
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

echo Creating symbolic links using: %DEFINITIONS_FILE%
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
for /f "usebackq tokens=1,2 delims=|" %%a in ("%DEFINITIONS_FILE%") do (
    call CreateSymLink.bat "%TARGET_DIR%\%%a" "%%b"
    if errorlevel 1 (
        echo.
        echo Symlink creation failed. Exiting...
        pause
        exit /b 1
    )
)

echo.
echo All symbolic links created successfully!
echo.
exit /b 0