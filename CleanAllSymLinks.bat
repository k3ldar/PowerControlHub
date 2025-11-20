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

REM Check for administrator privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: This script requires administrator privileges.
    echo Please right-click and select "Run as administrator"
    echo.
    pause
    exit /b 1
)

REM Store the root directory
set ROOT_DIR=%~dp0

echo ======================================
echo Cleaning BoatControlPanel symlinks...
echo ======================================
cd /d "%ROOT_DIR%BoatControlPanel"
if exist CleanSymLinks.bat (
    call CleanSymLinks.bat
) else (
    echo ERROR: CleanSymLinks.bat not found in BoatControlPanel
)

echo.
echo ======================================
echo Cleaning StaticElectrics symlinks...
echo ======================================
cd /d "%ROOT_DIR%StaticElectrics"
if exist CleanSymLinks.bat (
    call CleanSymLinks.bat
) else (
    echo ERROR: CleanSymLinks.bat not found in StaticElectrics
)

echo.
echo ======================================
echo All symbolic links cleaned!
echo ======================================
echo.
echo To recreate symbolic links:
echo   1. cd BoatControlPanel ^&^& SymLinks.bat
echo   2. cd StaticElectrics ^&^& SymLinks.bat
echo.
pause
exit /b 0