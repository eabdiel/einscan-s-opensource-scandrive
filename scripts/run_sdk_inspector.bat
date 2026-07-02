@echo off
cd /d %~dp0\..
python tools\inspect_sdk.py
start notepad docs\SDK_WIRING_REPORT.md
