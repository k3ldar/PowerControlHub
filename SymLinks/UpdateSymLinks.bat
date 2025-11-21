@echo off
REM ============================================================================
REM Main script to update symbolic links for all projects
REM
REM This script calls the ProcessSymLinksDefinition.bat script with the
REM appropriate SymLinkDefinitions*.txt file and target directory for each
REM project, reducing code duplication across project-specific batch files.
REM
REM IMPORTANT: This script must be run with administrator privileges because
REM the mklink command requires elevated permissions on Windows.
REM
REM Usage: Right-click this file and select "Run as administrator"
REM ============================================================================

echo Updating symbolic links for all projects...
echo.

REM Navigate to the SymLinks directory
cd /d "%~dp0" || (echo Failed to navigate to SymLinks directory && pause && exit /b 1)

REM Check for administrator privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: This script requires administrator privileges.
    echo Please right-click and select "Run as administrator"
    echo.
    pause
    exit /b 1
)

REM Process BoatControlPanel symbolic links
echo Processing BoatControlPanel symbolic links...
call ProcessSymLinksDefinition.bat SymLinkDefinitionsBoatControlPanel.txt ..\BoatControlPanel
if errorlevel 1 (
    echo Failed to create BoatControlPanel symbolic links
    pause
    exit /b 1
)

echo.

REM Process SmartFuseBox symbolic links
echo Processing SmartFuseBox symbolic links...
call ProcessSymLinksDefinition.bat SymLinkDefinitionsSmartFuseBox.txt ..\SmartFuseBox
if errorlevel 1 (
    echo Failed to create SmartFuseBox symbolic links
    pause
    exit /b 1
)

echo.
echo All projects updated successfully!
echo.


exit /b 0

