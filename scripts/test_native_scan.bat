@echo off
cd /d "%~dp0\.."
bridge_cpp\build\Release\einscan_legacy_bridge.exe scan --quality medium --texture 0 --duration 15 --out output\einscan_scan.obj
pause
