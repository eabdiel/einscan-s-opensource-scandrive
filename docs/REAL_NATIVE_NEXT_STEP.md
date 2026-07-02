# Real native next step

The package is now set to native mode by default. The bridge executable is the boundary between Python and the old SHINING 3D SDK.

What is done:

- PySide6 UI launches from `main.py`.
- Runtime mode is editable in the UI.
- Native bridge is called for detect, calibration, scan, and export.
- SDK root is added to PATH at runtime so DLLs can be found.
- Build script is included for Visual Studio 2013 x64.
- SDK inspector generates a symbol/header/lib report.

What still requires the actual SDK files on your machine:

- Exact `#include` names.
- Exact `.lib` names.
- Exact init/device/calibration/scan/export function calls.

After you run `scripts/run_sdk_inspector.bat`, paste or upload `docs/SDK_WIRING_REPORT.md` and I can map the exact C++ calls into the bridge.
