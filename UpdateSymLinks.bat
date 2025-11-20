@echo off
REM Main script to update symbolic links for all projects
REM This script navigates to each project directory and executes their SymLinks.bat files
REM to create symbolic links to shared code files in the Shared directory.
REM 
REM IMPORTANT: This script must be run with administrator privileges because
REM the mklink command requires elevated permissions on Windows.
REM
REM Usage: Right-click this file and select "Run as administrator"

cd BoatControlPanel || (echo Failed to navigate to BoatControlPanel && exit /b 1)
call SymLinks.bat
cd ..\StaticElectrics || (echo Failed to navigate to StaticElectrics && exit /b 1)
call SymLinks.bat
cd ..\

