# EinScan-S Python Bridge - real native SDK build

This package is now wired for the deprecated SHINING 3D EinScan-S SDK instead of mock-only mode.

## What is included

- `main.py` - opens the PySide6 UI from PyCharm.
- `app/` - Python UI and backend that call the native bridge executable.
- `bridge_cpp/einscan_legacy_bridge.cpp` - real C++ CLI bridge using `sn3d_pro_sdk.h`.
- `bridge_cpp/build_vs2013_x64.bat` - CMake build script for Visual Studio 2013 Win64.
- `scripts/test_native_detect.bat` - direct hardware detection test.
- `scripts/test_native_scan.bat` - direct 15-second scan test.

## SDK location expected

Place or clone the deprecated SDK here:

```text
third_party/deprecated-EinScan-SDK
```

The bridge expects these files:

```text
third_party/deprecated-EinScan-SDK/SDK/include/sn3d_pro_sdk.h
third_party/deprecated-EinScan-SDK/SDK/x64_lib/sn3d_pro_sdk.lib
third_party/deprecated-EinScan-SDK/SDK/x64_dll/sn3d_pro_sdk.dll
```

## Build

Open a Visual Studio 2013 x64 developer command prompt when possible, then run:

```bat
bridge_cpp\build_windows_x64.bat
```

The output should be:

```text
bridge_cpp\build\Release\einscan_legacy_bridge.exe
```

The script also copies the SDK `x64_dll` folder beside the executable, which is important because the SDK has many runtime DLL dependencies.

## Run the UI

```bat
pip install -r requirements.txt
python main.py
```

The UI defaults to native mode and calls:

```text
bridge_cpp/build/Release/einscan_legacy_bridge.exe
```

## Test order

1. Build the bridge.
2. Run `scripts\test_native_detect.bat`.
3. If detection succeeds, run the UI and press **Detect Scanner**.
4. Try **Calibrate**.
5. Try **Start Scan + Export** with OBJ or PLY output.

## Current native behavior

The bridge now calls the real SDK functions:

- `sn3d_Initialize`
- `sn3d_regist_callback`
- `sn3d_start_calibrate`
- `sn3d_set_scan_param`
- `sn3d_start_scan`
- `sn3d_finish_scan`
- `sn3d_get_current_scan_whole_point_cloud`
- `sn3d_close`

For the first working version, scan export is a point-cloud OBJ/PLY export. Mesh processing can be added after we confirm scanner detection and scan capture work on your machine.

## Important first error to watch

If detection returns `SN3D_RET_DEVICE_LICENSE_ERROR (-7)`, the SDK is loading but the scanner/license check is failing. If it returns `SN3D_RET_NO_DEVICE_ERROR (-6)`, the SDK loaded but cannot see the scanner.


## Build script troubleshooting update

If `build_vs2013_x64.bat` opens and closes immediately, use the newer wrapper instead:

```bat
scripts\diagnose_build_env.bat
bridge_cpp\build_windows_x64.bat
```

The script now pauses on failure and writes:

```text
bridge_cpp\build_windows_x64.log
```

Common failures:

- `MISSING header/lib/dll`: copy the deprecated SDK to `third_party\deprecated-EinScan-SDK`.
- `CMake was not found`: install CMake and check "Add to PATH".
- `cl.exe still not found`: install Visual Studio Build Tools with the C++ desktop workload, or run from an x64 Native Tools Command Prompt.


## GPU / CUDA init-mode probe

If `scripts\test_native_detect.bat` returns `SN3D_RET_GPU_ERROR (-8)`, run:

```bat
scripts\test_init_modes.bat
```

This tests all four SDK initialization modes:

- `SN3D_INIT_CALIBRATE`
- `SN3D_INIT_RAPIDSCAN`
- `SN3D_INIT_HD_SCAN`
- `SN3D_INIT_FIX_SCAN`

If every mode returns `-8`, the deprecated SDK is most likely blocked by its legacy NVIDIA CUDA dependency. AMD-only devices such as the ASUS Ally X are expected to fail here.


## AMD / ZLUDA + config-path test

This build adds `--config` and two test scripts:

```bat
scripts\test_init_modes_with_config.bat
scripts\test_zluda_probe.bat
```

See `docs/ZLUDA_AMD_TEST_PLAN.md`.

## v1 device-type/resource probe notes
The official SDK demo initializes `SN3D_INIT_DATA` with `device_type` set to `EinScan-Pro` or `EinScan-Plus` and leaves `config_path` blank. This build now mirrors that behavior and copies the SDK `res`, `plugins`, and `optional` folders beside the bridge EXE. The bridge also sets its working directory to its own folder before calling `sn3d_Initialize`, so the SDK can find `res\...` relative resources.

Recommended AMD/ZLUDA test:

```bat
bridge_cpp\build_windows_x64.bat
scripts\test_zluda_probe.bat
```

The probe now tests both `EinScan-Pro` and `EinScan-Plus` device types across the SDK init modes.
