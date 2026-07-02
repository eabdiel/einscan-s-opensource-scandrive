import json
import os
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Optional

LogFn = Callable[[str], None]
ROOT = Path(__file__).resolve().parents[1]
CONFIG_PATH = ROOT / "config" / "scanner_config.json"

@dataclass
class ScanSettings:
    scan_mode: str = "turntable"
    quality: str = "medium"
    texture: bool = True
    export_format: str = "obj"
    output_dir: str = "output"
    duration_seconds: int = 15

class ScannerBackend:
    def detect(self, log: LogFn) -> bool: raise NotImplementedError
    def calibrate(self, log: LogFn) -> bool: raise NotImplementedError
    def scan(self, settings: ScanSettings, log: LogFn) -> Optional[Path]: raise NotImplementedError

class MockScannerBackend(ScannerBackend):
    def detect(self, log: LogFn) -> bool:
        log("Mock: scanner detected.")
        return True
    def calibrate(self, log: LogFn) -> bool:
        for step in ["projector warm-up", "camera exposure", "turntable", "calibration board"]:
            log(f"Mock calibration: {step}"); time.sleep(0.25)
        return True
    def scan(self, settings: ScanSettings, log: LogFn) -> Optional[Path]:
        output_dir = _resolve_path(settings.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        out_file = output_dir / f"mock_scan.{settings.export_format.lower()}"
        for i in range(1, 7):
            log(f"Mock scan pass {i}/6 captured."); time.sleep(0.2)
        _write_placeholder_mesh(out_file, settings.export_format)
        log(f"Mock scan exported: {out_file}")
        return out_file

class BridgeCliBackend(ScannerBackend):
    def __init__(self, bridge_exe: str, sdk_root: str = ""):
        self.bridge_exe = _resolve_path(bridge_exe)
        self.sdk_root = _resolve_path(sdk_root) if sdk_root else None

    def _env(self):
        env = os.environ.copy()
        if self.sdk_root and self.sdk_root.exists():
            dll_dirs = []
            for pattern in ["**/bin", "**/Bin", "**/x64", "**/Release", "**/dll", "**/DLL"]:
                dll_dirs += [str(p) for p in self.sdk_root.glob(pattern) if p.is_dir()]
            if dll_dirs:
                env["PATH"] = os.pathsep.join(dll_dirs + [env.get("PATH", "")])
        return env

    def _run(self, args: list[str], log: LogFn) -> subprocess.CompletedProcess:
        if not self.bridge_exe.exists():
            raise FileNotFoundError(
                f"Native bridge not found: {self.bridge_exe}\n"
                "Run bridge_cpp/build_vs2013_x64.bat after placing the SDK under third_party/deprecated-EinScan-SDK."
            )
        cmd = [str(self.bridge_exe), *args]
        log("Running native bridge: " + " ".join(cmd))
        proc = subprocess.run(cmd, cwd=str(ROOT), text=True, capture_output=True, check=False, env=self._env())
        if proc.stdout:
            for line in proc.stdout.splitlines(): log(line)
        if proc.stderr:
            for line in proc.stderr.splitlines(): log("ERR: " + line)
        log(f"Exit code: {proc.returncode}")
        return proc

    def detect(self, log: LogFn) -> bool:
        return self._run(["detect"], log).returncode == 0

    def calibrate(self, log: LogFn) -> bool:
        return self._run(["calibrate"], log).returncode == 0

    def scan(self, settings: ScanSettings, log: LogFn) -> Optional[Path]:
        output_dir = _resolve_path(settings.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        out_file = output_dir / f"einscan_scan.{settings.export_format.lower()}"
        proc = self._run([
            "scan", "--mode", settings.scan_mode, "--quality", settings.quality,
            "--texture", "1" if settings.texture else "0",
            "--format", settings.export_format.lower(), "--duration", str(settings.duration_seconds), "--out", str(out_file)
        ], log)
        return out_file if proc.returncode == 0 and out_file.exists() else None

def _resolve_path(value: str | Path) -> Path:
    p = Path(value)
    return p if p.is_absolute() else (ROOT / p)

def _write_placeholder_mesh(out_file: Path, fmt: str):
    fmt = fmt.lower()
    if fmt == "obj":
        out_file.write_text("# placeholder OBJ\no Mesh\nv 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n", encoding="utf-8")
    elif fmt == "ply":
        out_file.write_text("ply\nformat ascii 1.0\nelement vertex 3\nproperty float x\nproperty float y\nproperty float z\nelement face 1\nproperty list uchar int vertex_indices\nend_header\n0 0 0\n1 0 0\n0 1 0\n3 0 1 2\n", encoding="utf-8")
    else:
        out_file.write_text("solid placeholder\nendsolid placeholder\n", encoding="utf-8")

def load_config() -> dict:
    if CONFIG_PATH.exists():
        return json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    return {"mode": "native"}

def save_config(config: dict) -> None:
    CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
    CONFIG_PATH.write_text(json.dumps(config, indent=2), encoding="utf-8")

def load_backend():
    config = load_config()
    mode = config.get("mode", "native").lower()
    if mode == "mock":
        return MockScannerBackend(), config
    return BridgeCliBackend(config.get("bridge_exe", "bridge_cpp/build/Release/einscan_legacy_bridge.exe"), config.get("sdk_root", "third_party/deprecated-EinScan-SDK")), config
