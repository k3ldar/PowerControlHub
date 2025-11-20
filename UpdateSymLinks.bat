@echo off
REM Main script to update symbolic links for all projects
REM Note: Must be run with administrator privileges

cd BoatControlPanel || (echo Failed to navigate to BoatControlPanel && exit /b 1)
call SymLinks.bat
cd ..\StaticElectrics || (echo Failed to navigate to StaticElectrics && exit /b 1)
call SymLinks.bat
cd ..\