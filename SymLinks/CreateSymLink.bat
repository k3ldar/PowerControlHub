@echo off
REM ============================================================================
REM Helper script to create a single symbolic link with error checking
REM Usage: CreateSymLink.bat <link_name> <target_path>
REM ============================================================================

set LINK_NAME=%~1
set TARGET_PATH=%~2

if exist "%LINK_NAME%" (
    echo Warning: %LINK_NAME% already exists, skipping
) else (
    mklink "%LINK_NAME%" "%TARGET_PATH%"
    if errorlevel 1 (
        echo ERROR: Failed to create %LINK_NAME% symlink
        exit /b 1
    )
    echo Created: %LINK_NAME%
)
exit /b 0