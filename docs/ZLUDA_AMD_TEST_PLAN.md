# AMD / ZLUDA Test Plan for the deprecated EinScan-S SDK

Your current result means the SDK can load, but Rapid Scan and HD Scan fail at `SN3D_RET_GPU_ERROR (-8)` on the AMD GPU. Calibration mode reaches the config-file check instead, which is why this version also adds `--config` support.

## What this package adds

- `--config <path>` argument for `detect`, `probe`, `calibrate`, and `scan`.
- `EINSCAN_CONFIG_PATH` environment variable fallback.
- `scripts\test_init_modes_with_config.bat` to test the calibration/config path.
- `scripts\test_zluda_probe.bat` to test ZLUDA DLL injection next to the bridge executable.

## Test order

1. Rebuild the bridge:

```bat
cd C:\Users\abdie\PycharmProjects\einscan-model-s-opensource
bridge_cpp\build_windows_x64.bat
```

2. Search the SDK/software folders for configuration files. Good places to try as `--config`:

```text
third_party\deprecated-EinScan-SDK\SDK
third_party\deprecated-EinScan-SDK\SDK\sdk_demo_pro
C:\Program Files\SHINING3D
C:\ProgramData\SHINING3D
C:\ProgramData\EinScan
```

3. Run config-aware init probe:

```bat
scripts\test_init_modes_with_config.bat
```

4. If Rapid Scan and HD Scan still return `-8`, try ZLUDA:

```text
third_party\zluda\
```

Place the ZLUDA DLLs in that folder, then run:

```bat
scripts\test_zluda_probe.bat
```

## Expected outcomes

- If `SN3D_INIT_CALIBRATE` changes from `-11` to `0`, config path is fixed.
- If Rapid Scan / HD Scan change from `-8` to another code, ZLUDA is at least hooking the CUDA path.
- If Rapid Scan / HD Scan stay at `-8`, the closed SDK is still blocked by the AMD GPU path.

## Important limitation

HIP/HIPIFY does not apply directly because we do not have SHINING3D's CUDA source code. ZLUDA is the only realistic AMD experiment because it targets existing CUDA binaries.
