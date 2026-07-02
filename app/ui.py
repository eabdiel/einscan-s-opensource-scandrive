import subprocess
import sys
from pathlib import Path
from PySide6.QtCore import QThread, Signal
from PySide6.QtWidgets import (
    QApplication, QCheckBox, QComboBox, QFileDialog, QGridLayout, QGroupBox,
    QLabel, QLineEdit, QMainWindow, QPushButton, QTextEdit, QVBoxLayout,
    QWidget, QMessageBox, QHBoxLayout, QSpinBox
)
from app.scanner_backend import ScanSettings, load_backend, load_config, save_config, ROOT

class Worker(QThread):
    log_signal = Signal(str)
    done_signal = Signal(bool, str)
    def __init__(self, action, settings=None):
        super().__init__(); self.action = action; self.settings = settings; self.backend, _ = load_backend()
    def run(self):
        try:
            if self.action == "detect":
                ok = self.backend.detect(self.log_signal.emit); self.done_signal.emit(ok, "Detection completed." if ok else "Scanner not detected.")
            elif self.action == "calibrate":
                ok = self.backend.calibrate(self.log_signal.emit); self.done_signal.emit(ok, "Calibration completed." if ok else "Calibration failed.")
            elif self.action == "scan":
                result = self.backend.scan(self.settings, self.log_signal.emit); self.done_signal.emit(result is not None, f"Scan exported: {result}" if result else "Scan failed.")
        except Exception as exc:
            self.log_signal.emit(f"ERROR: {exc}"); self.done_signal.emit(False, str(exc))

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__(); self.config = load_config(); self.worker = None
        self.setWindowTitle("EinScan-S Legacy Scanner Controller")
        self.resize(980, 700); self._build_ui()
        self.log(f"Ready. Mode: {self.config.get('mode', 'native')}")
        self.log("Native mode expects bridge_cpp/build/Release/einscan_legacy_bridge.exe.")

    def _build_ui(self):
        root = QWidget(); layout = QVBoxLayout(root)

        cfg = QGroupBox("Runtime")
        cfg_grid = QGridLayout(cfg)
        self.runtime_combo = QComboBox(); self.runtime_combo.addItems(["native", "mock"])
        self.runtime_combo.setCurrentText(self.config.get("mode", "native"))
        self.bridge_edit = QLineEdit(self.config.get("bridge_exe", "bridge_cpp/build/Release/einscan_legacy_bridge.exe"))
        self.sdk_edit = QLineEdit(self.config.get("sdk_root", "third_party/deprecated-EinScan-SDK"))
        save_btn = QPushButton("Save Runtime Settings"); save_btn.clicked.connect(self.save_runtime)
        build_btn = QPushButton("Build Native Bridge"); build_btn.clicked.connect(self.build_bridge)
        cfg_grid.addWidget(QLabel("Mode"),0,0); cfg_grid.addWidget(self.runtime_combo,0,1)
        cfg_grid.addWidget(QLabel("Bridge exe"),1,0); cfg_grid.addWidget(self.bridge_edit,1,1,1,3)
        cfg_grid.addWidget(QLabel("SDK root"),2,0); cfg_grid.addWidget(self.sdk_edit,2,1,1,3)
        cfg_grid.addWidget(save_btn,3,2); cfg_grid.addWidget(build_btn,3,3)
        layout.addWidget(cfg)

        settings_box = QGroupBox("Scan Settings")
        grid = QGridLayout(settings_box)
        self.mode_combo = QComboBox(); self.mode_combo.addItems(["turntable", "fixed", "manual"])
        self.quality_combo = QComboBox(); self.quality_combo.addItems(["low", "medium", "high"])
        self.format_combo = QComboBox(); self.format_combo.addItems(["obj", "ply"])
        self.texture_check = QCheckBox("Capture texture / color"); self.texture_check.setChecked(True)
        self.duration_spin = QSpinBox(); self.duration_spin.setRange(3, 300); self.duration_spin.setValue(int(self.config.get("default_scan_duration_seconds", 15)))
        self.output_edit = QLineEdit(self.config.get("default_output_dir", "output"))
        browse_btn = QPushButton("Browse"); browse_btn.clicked.connect(self.pick_output_dir)
        grid.addWidget(QLabel("Scan mode"),0,0); grid.addWidget(self.mode_combo,0,1)
        grid.addWidget(QLabel("Quality"),0,2); grid.addWidget(self.quality_combo,0,3)
        grid.addWidget(QLabel("Export format"),1,0); grid.addWidget(self.format_combo,1,1)
        grid.addWidget(QLabel("Scan duration sec"),1,2); grid.addWidget(self.duration_spin,1,3)
        grid.addWidget(self.texture_check,3,0,1,2)
        grid.addWidget(QLabel("Output folder"),2,0); grid.addWidget(self.output_edit,2,1,1,2); grid.addWidget(browse_btn,2,3)
        layout.addWidget(settings_box)

        actions = QGroupBox("Scanner Actions")
        action_layout = QHBoxLayout(actions)
        for text, handler in [("Detect Scanner", self.detect), ("Calibrate", self.calibrate), ("Start Scan + Export", self.scan), ("Open Output Folder", self.open_output)]:
            btn = QPushButton(text); btn.clicked.connect(handler); action_layout.addWidget(btn)
        layout.addWidget(actions)

        self.log_view = QTextEdit(); self.log_view.setReadOnly(True); layout.addWidget(self.log_view)
        self.setCentralWidget(root)

    def save_runtime(self):
        self.config["mode"] = self.runtime_combo.currentText()
        self.config["bridge_exe"] = self.bridge_edit.text().strip()
        self.config["sdk_root"] = self.sdk_edit.text().strip()
        self.config["default_output_dir"] = self.output_edit.text().strip()
        self.config["default_scan_duration_seconds"] = self.duration_spin.value()
        save_config(self.config); self.log("Saved runtime settings.")

    def build_bridge(self):
        self.save_runtime()
        script = ROOT / "bridge_cpp" / "build_vs2013_x64.bat"
        if not script.exists():
            QMessageBox.warning(self,"Missing script",str(script)); return
        self.log("Launching bridge build script. Use a VS2013 x64 environment if CMake cannot find the compiler.")
        try:
            subprocess.Popen(["cmd", "/c", str(script)], cwd=str(ROOT))
        except Exception as exc:
            self.log(f"ERROR launching build: {exc}")

    def pick_output_dir(self):
        chosen = QFileDialog.getExistingDirectory(self, "Choose output folder", self.output_edit.text())
        if chosen: self.output_edit.setText(chosen)

    def settings(self):
        return ScanSettings(self.mode_combo.currentText(), self.quality_combo.currentText(), self.texture_check.isChecked(), self.format_combo.currentText(), self.output_edit.text(), self.duration_spin.value())
    def log(self, msg): self.log_view.append(msg)
    def run_action(self, action):
        if self.worker and self.worker.isRunning(): QMessageBox.information(self,"Busy","Scanner action is already running."); return
        self.save_runtime(); self.worker = Worker(action, self.settings()); self.worker.log_signal.connect(self.log); self.worker.done_signal.connect(self.done); self.worker.start()
    def detect(self): self.run_action("detect")
    def calibrate(self): self.run_action("calibrate")
    def scan(self): self.run_action("scan")
    def done(self, ok, message):
        self.log(message)
        if not ok: QMessageBox.warning(self,"EinScan",message)
    def open_output(self):
        p = Path(self.output_edit.text()); p = p if p.is_absolute() else ROOT / p; p.mkdir(parents=True, exist_ok=True)
        subprocess.Popen(["explorer", str(p)]) if sys.platform.startswith("win") else None

def main():
    app = QApplication(sys.argv); w = MainWindow(); w.show(); sys.exit(app.exec())
