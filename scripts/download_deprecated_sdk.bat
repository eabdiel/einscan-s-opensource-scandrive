@echo off
setlocal

REM Optional helper. Requires Git. If Git LFS is needed for large files, install it first.
cd /d %~dp0\..
if not exist third_party mkdir third_party
cd third_party
if exist deprecated-EinScan-SDK (
  echo Folder already exists: third_party\deprecated-EinScan-SDK
  exit /b 0
)
git clone https://github.com/Shining3D/deprecated-EinScan-SDK.git deprecated-EinScan-SDK
