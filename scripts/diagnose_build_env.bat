@echo off
setlocal
cd /d "%~dp0\.."
echo === EinScan-S build environment diagnostic ===
echo Project: %CD%
echo.
echo Checking SDK files...
if exist "third_party\deprecated-EinScan-SDK\SDK\include\sn3d_pro_sdk.h" (echo OK header) else echo MISSING header
if exist "third_party\deprecated-EinScan-SDK\SDK\x64_lib\sn3d_pro_sdk.lib" (echo OK lib) else echo MISSING lib
if exist "third_party\deprecated-EinScan-SDK\SDK\x64_dll\sn3d_pro_sdk.dll" (echo OK dll) else echo MISSING dll
echo.
echo Checking tools...
where cmake
where cl
where msbuild
echo.
echo Visual Studio locator:
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
  "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
) else echo vswhere not found
echo.
pause
