# Deprecated EinScan-S SDK Integration Notes

## Target SDK

Use `Shining3D/deprecated-EinScan-SDK`, not SESPSDK. SESPSDK is for newer EinScan SE/SP models.

## What we need from the SDK folder

To wire the bridge fully, inspect:

- Header files: classes/functions for device manager, scanner init, calibration, scan capture, mesh fusion, and export.
- Library files: `.lib` names and architecture.
- Runtime DLLs: copy beside `einscan_legacy_bridge.exe` or add to PATH.
- Demo source: fastest way to map call order.

## Expected native lifecycle

Pseudo-flow:

```cpp
sdk_initialize();
scanner = sdk_find_first_scanner();
scanner.open();
scanner.load_or_run_calibration();
scanner.set_scan_mode(turntable/fixed/manual);
scanner.set_texture_enabled(true/false);
scanner.capture_scan_sequence();
scanner.align_frames();
scanner.fuse_mesh();
scanner.export_mesh(out_file);
scanner.close();
sdk_shutdown();
```

## Python remains unchanged

Once the bridge executable returns standard process exit codes, the Python UI does not need to know vendor-specific SDK details.

- Exit code `0`: success
- Exit code `1`: scanner or SDK operation failed
- Exit code `2`: invalid CLI use

## Common problems

- Wrong architecture: SDK is x64; Python app can be any architecture, but the bridge must match SDK libs.
- DLL not found: put vendor DLLs beside the bridge exe.
- Old runtime missing: install Visual C++ runtime matching the SDK/demo.
- Scanner not detected: install old driver package and test with official software first.
