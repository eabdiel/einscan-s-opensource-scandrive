@echo off
cd /d "%~dp0\.."
bridge_cpp\build\Release\einscan_legacy_bridge.exe detect
pause
