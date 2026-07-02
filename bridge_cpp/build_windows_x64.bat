@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set LOG=%CD%\build_windows_x64.log
if exist "%LOG%" del "%LOG%"
call :log ============================================================
call :log EinScan-S Legacy Bridge Build - Windows x64
call :log Started: %DATE% %TIME%
call :log Working dir: %CD%
call :log ============================================================

set SDK_ROOT=%CD%\..\third_party\deprecated-EinScan-SDK
if not exist "%SDK_ROOT%\SDK\include\sn3d_pro_sdk.h" (
  call :log ERROR: SDK header not found.
  call :log Expected: %SDK_ROOT%\SDK\include\sn3d_pro_sdk.h
  call :log.
  call :log Fix: copy or clone the deprecated SDK into:
  call :log   third_party\deprecated-EinScan-SDK
  goto fail
)
if not exist "%SDK_ROOT%\SDK\x64_lib\sn3d_pro_sdk.lib" (
  call :log ERROR: SDK lib not found.
  call :log Expected: %SDK_ROOT%\SDK\x64_lib\sn3d_pro_sdk.lib
  goto fail
)
if not exist "%SDK_ROOT%\SDK\x64_dll\sn3d_pro_sdk.dll" (
  call :log ERROR: SDK DLL not found.
  call :log Expected: %SDK_ROOT%\SDK\x64_dll\sn3d_pro_sdk.dll
  goto fail
)

where cmake >nul 2>nul
if errorlevel 1 (
  call :log ERROR: CMake was not found in PATH.
  call :log Install CMake or add it to PATH, then run this script again.
  goto fail
)

where cl >nul 2>nul
if errorlevel 1 (
  call :log cl.exe not currently on PATH. Trying to load Visual Studio environment...
  call :load_vs_env
)

where cl >nul 2>nul
if errorlevel 1 (
  call :log ERROR: cl.exe still not found.
  call :log Fix options:
  call :log  1. Install Visual Studio Build Tools with C++ desktop workload.
  call :log  2. Or run this from an x64 Native Tools Command Prompt.
  goto fail
)

for /f "tokens=*" %%i in ('cl 2^>^&1 ^| findstr /C:"Version"') do call :log Compiler: %%i
for /f "tokens=*" %%i in ('cmake --version ^| findstr /C:"cmake version"') do call :log %%i

if not exist build mkdir build
cd build

call :log.
call :log Configuring CMake...
cmake .. -A x64 -DSDK_ROOT="%SDK_ROOT%" >> "%LOG%" 2>&1
if errorlevel 1 (
  call :log CMake configure failed with -A x64. Trying Visual Studio 12 2013 Win64 generator...
  rmdir /s /q . 2>nul
  mkdir . 2>nul
  cmake .. -G "Visual Studio 12 2013 Win64" -DSDK_ROOT="%SDK_ROOT%" >> "%LOG%" 2>&1
  if errorlevel 1 goto buildfail
)

call :log Building Release...
cmake --build . --config Release >> "%LOG%" 2>&1
if errorlevel 1 goto buildfail

if not exist "Release\einscan_legacy_bridge.exe" (
  call :log ERROR: Build completed but EXE was not found at build\Release\einscan_legacy_bridge.exe
  goto fail
)

call :log.
call :log SUCCESS: Built %CD%\Release\einscan_legacy_bridge.exe
call :log Copying SDK DLLs next to EXE...
xcopy /Y /I "%SDK_ROOT%\SDK\x64_dll\*.dll" "%CD%\Release\" >> "%LOG%" 2>&1
if exist "%SDK_ROOT%\SDK\x64_dll\Device" xcopy /Y /I /E "%SDK_ROOT%\SDK\x64_dll\Device" "%CD%\Release\Device" >> "%LOG%" 2>&1
call :log.
call :log Done. Next run: ..\scripts\test_native_detect.bat from the project root.
goto done

:load_vs_env
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSINSTALL=%%i
  if defined VSINSTALL (
    call :log Found Visual Studio: !VSINSTALL!
    if exist "!VSINSTALL!\VC\Auxiliary\Build\vcvars64.bat" call "!VSINSTALL!\VC\Auxiliary\Build\vcvars64.bat" >> "%LOG%" 2>&1
  )
)
if exist "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" (
  call :log Found VS2013 tools through VS120COMNTOOLS.
  call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" amd64 >> "%LOG%" 2>&1
)
exit /b 0

:buildfail
call :log.
call :log ERROR: Build failed. Last log lines:
powershell -NoProfile -Command "Get-Content '%LOG%' -Tail 80" 2>nul
goto fail

:fail
call :log.
call :log FAILED. Full log: %LOG%
echo.
echo FAILED. Full log: %LOG%
echo.
pause
exit /b 1

:done
echo.
echo SUCCESS. Full log: %LOG%
echo.
pause
exit /b 0

:log
echo %*
echo %*>> "%LOG%"
exit /b 0
