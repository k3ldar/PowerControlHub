@echo off
REM ============================================================================
REM Delete ALL symbolic links in both BoatControlPanel and StaticElectrics
REM
REM This master script removes symbolic links from both project directories.
REM Useful for a complete reset before recreating symlinks.
REM
REM IMPORTANT: This script requires administrator privileges to run.
REM Right-click and select "Run as administrator" when executing.
REM ============================================================================

echo Cleaning symbolic links in ALL project directories...
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

echo ======================================
echo Cleaning BoatControlPanel symlinks...
echo ======================================
call CleanSymLinksDefinition.bat SymLinkDefinitionsBoatControlPanel.txt ..\BoatControlPanel
if errorlevel 1 (
    echo Failed to clean BoatControlPanel symbolic links
    pause
    exit /b 1
)

echo.
echo ======================================
echo Cleaning StaticElectrics symlinks...
echo ======================================
call CleanSymLinksDefinition.bat SymLinkDefinitionsStaticElectrics.txt ..\StaticElectrics
if errorlevel 1 (
    echo Failed to clean StaticElectrics symbolic links
    pause
    exit /b 1
)

echo.
echo ======================================
echo All symbolic links cleaned!
echo ======================================
echo.
echo To recreate symbolic links, run: UpdateSymLinks.bat
echo.

exit /b 0