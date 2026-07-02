# Native SDK bridge placeholder

The Python app is ready to call a native executable at:

`bridge_cpp/build/Release/einscan_bridge.exe`

The bridge should support these commands:

```bat
einscan_bridge.exe detect
einscan_bridge.exe calibrate
einscan_bridge.exe scan --mode turntable --quality medium --texture 1 --format obj --out output/einscan_scan.obj
```

Why this exists: Shining3D's legacy SDKs are C/C++ oriented and may depend on older Visual Studio runtimes. Keeping the vendor SDK calls in a small C++ bridge makes the Python UI stable and easy to maintain.

Next step after you confirm which SDK detects your scanner: replace the TODO sections in `einscan_bridge.cpp` with the matching SDK initialization, calibration, scan, and export calls.
