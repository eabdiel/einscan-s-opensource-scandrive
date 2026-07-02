# SDK integration notes

Use this as the integration checklist when you test the real scanner.

## Compatibility check

- If SESPSDK detects the scanner, use SESPSDK.
- If SESPSDK does not detect the scanner, test the deprecated EinScan SDK.
- Keep the original vendor software installed if the SDK depends on drivers or runtime DLLs.

## C++ bridge implementation map

Replace the TODOs in `bridge_cpp/einscan_bridge.cpp`:

1. `detect`
   - initialize SDK
   - enumerate scanner/camera/projector/turntable
   - return 0 only when a compatible scanner is detected

2. `calibrate`
   - initialize SDK
   - run calibration board workflow or load existing calibration
   - return 0 on success

3. `scan`
   - initialize SDK
   - apply mode/quality/texture settings
   - capture frames
   - align/fuse point cloud or mesh
   - export OBJ/STL/PLY to `--out`

## Why not pure Python first?

Many legacy hardware SDKs expose C++ classes, old runtime DLLs, camera drivers, and device handles. A CLI bridge keeps those dependencies outside Python and gives us a clean contract to call from the UI.
