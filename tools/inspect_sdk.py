"""Inspect the deprecated EinScan SDK folder and produce a wiring report.
Run after cloning/copying the SDK:
    python tools/inspect_sdk.py
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SDK = ROOT / "third_party" / "deprecated-EinScan-SDK"
REPORT = ROOT / "docs" / "SDK_WIRING_REPORT.md"
KEYWORDS = [
    "init", "initialize", "device", "scanner", "camera", "projector", "turntable",
    "calib", "scan", "capture", "mesh", "point", "cloud", "align", "fuse", "export", "save"
]

def main():
    lines = ["# SDK Wiring Report", ""]
    if not SDK.exists():
        lines += [f"SDK folder not found: `{SDK}`", "", "Run `scripts/download_deprecated_sdk.bat` or copy the SDK there."]
        REPORT.write_text("\n".join(lines), encoding="utf-8")
        print(REPORT)
        return
    headers = list(SDK.rglob("*.h")) + list(SDK.rglob("*.hpp"))
    libs = list(SDK.rglob("*.lib"))
    dlls = list(SDK.rglob("*.dll"))
    slns = list(SDK.rglob("*.sln"))
    vcx = list(SDK.rglob("*.vcxproj"))
    lines += [f"SDK root: `{SDK}`", "", f"Headers: {len(headers)}", f"LIBs: {len(libs)}", f"DLLs: {len(dlls)}", f"Solutions: {len(slns)}", f"VCXPROJ: {len(vcx)}", ""]
    for title, files in [("Libraries", libs), ("DLLs", dlls), ("Solutions", slns), ("Projects", vcx)]:
        lines += [f"## {title}", ""]
        for p in files[:80]: lines.append(f"- `{p.relative_to(ROOT)}`")
        if len(files) > 80: lines.append(f"- ... {len(files)-80} more")
        lines.append("")
    lines += ["## Likely SDK symbols / methods", ""]
    symbol_re = re.compile(r"\b(?:class|struct|enum|typedef|extern|[A-Za-z_][\w:<>,\s\*&]+)\s+([A-Za-z_]\w*)\s*\(")
    hits = []
    for h in headers:
        try: text = h.read_text(errors="ignore")
        except Exception: continue
        for i, line in enumerate(text.splitlines(), 1):
            low = line.lower()
            if any(k in low for k in KEYWORDS):
                if "//" in line and line.strip().startswith("//"): continue
                hits.append((h.relative_to(ROOT), i, line.strip()))
    for h, i, line in hits[:300]:
        lines.append(f"- `{h}:{i}` `{line[:180]}`")
    if len(hits) > 300: lines.append(f"- ... {len(hits)-300} more")
    lines += ["", "## Next mapping", "", "Use this report to replace the TODO blocks in `bridge_cpp/einscan_legacy_bridge.cpp`. The bridge CLI contract is already wired to the Python UI."]
    REPORT.write_text("\n".join(lines), encoding="utf-8")
    print(REPORT)

if __name__ == "__main__":
    main()
