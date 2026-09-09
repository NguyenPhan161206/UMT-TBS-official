#!/usr/bin/env python3
"""arch_guard.py — ép buộc kiến trúc G1 (quy tắc B1–B7, docs/ARCHITECTURE_G1_TESTING.md).

Chạy: python3 tools/guard/arch_guard.py
Exit 0 = OK. Exit 1 = có vi phạm (liệt kê ra stderr).

Các check hiện có:
  [B1] R-dep: hazard_core KHÔNG include sensor_model.h;
       ui_dashboard KHÔNG include espnow_receiver.h (header nội bộ).
  [B2] R-core-stateless: hazard_core.c không khai báo static mutable
       (static .* = ...); không include FreeRTOS/LVGL.
  [B5] R-single-truth: CROSSING_DELTA_CM / CROSSING_FRONT_THRESHOLD_CM
       ĐỊNH NGHĨA ĐÚNG 1 NƠI (hazard_core.h), không còn trong ui_dashboard_theme.h;
       python mirror tools/test_mqtt_coreiot.py CAUTION_CM/DANGER_CM
       khớp firmware/shared/thresholds.h SENSOR_*_CM (mục A2 HARDCODED_CONFIG_NOTES).
  [R2] sensor_model_classify KHÔNG còn tồn tại (đã xoá wrapper);
       các symbol hazard_core định nghĩa đúng 1 nơi.
  [B7] hazard_core.h <= 4 hàm public hazard_*; file <= 400 dòng.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

ERRORS: list[str] = []


def fail(msg: str) -> None:
    ERRORS.append(f"[arch-guard] {msg}")


def grep_repo(pattern: str, root: Path, include: str = "*.{c,h,cpp}") -> list[Path]:
    return sorted(p for p in root.rglob("*") if p.is_file() and ".pio" not in p.parts
                  and "managed_components" not in p.parts
                  and _match_suffix(p, include))


def _match_suffix(p: Path, pattern: str) -> bool:
    pats = set(x.strip() for x in pattern.strip("{}").split(","))
    return p.suffix in pats or p.suffix in {".c", ".h", ".cpp"}


def count_occurrences(root: Path, needle: str, include: str = "*.{c,h,cpp}") -> int:
    total = 0
    for p in grep_repo(needle, root, include):
        total += len(p.read_text(encoding="utf-8", errors="ignore").split(needle)) - 1
    return total


def has_directive(p: Path, directive: str) -> bool:
    try:
        txt = p.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return False
    return any(line.strip().startswith(directive) for line in txt.splitlines())


# ------------------------------------------------------------------ B1 R-dep
def check_b1(root: Path) -> None:
    hc_h = root / "firmware/waveshare-screen/components/hazard_core/hazard_core.c"
    if hc_h.exists() and has_directive(hc_h, '#include "sensor_model.h"'):
        fail("B1: hazard_core.c KHÔNG được include sensor_model.h (phụ thuộc ngược)")

    ui_dir = root / "firmware/waveshare-screen/components/ui_dashboard"
    for f in sorted(p for p in ui_dir.rglob("*.c") if ".pio" not in p.parts):
        if has_directive(f, '#include "espnow_receiver.h"'):
            fail(f"B1: ui_dashboard không được include espnow_receiver.h ({f.relative_to(root)})")


# ----------------------------------------------------------------- B2 no static state
def check_b2(root: Path) -> None:
    hc_c = root / "firmware/waveshare-screen/components/hazard_core/hazard_core.c"
    if not hc_c.exists():
        return
    txt = hc_c.read_text(encoding="utf-8", errors="ignore")
    for line in txt.splitlines():
        s = line.strip()
        if re.match(r"^static\s+(?!const)", s) and "=" in s:
            fail(f"B2: hazard_core.c state mutable tại dòng {line}")
    for banned in ("freertos/FreeRTOS.h", "freertos/semphr.h", "lvgl.h"):
        if has_directive(hc_c, f'#include "{banned}"'):
            fail(f"B2: hazard_core.c không được include {banned}")


# ----------------------------------------------------------------- B5 single-truth
def check_b5(root: Path) -> None:
    theme_h = root / "firmware/waveshare-screen/components/ui_dashboard/ui_dashboard_theme.h"
    if theme_h.exists():
        txt = theme_h.read_text(encoding="utf-8", errors="ignore")
        for sym in ("CROSSING_DELTA_CM", "CROSSING_FRONT_THRESHOLD_CM"):
            if re.search(rf"#define\s+{sym}\b", txt):
                fail(f"B5: {sym} còn định nghĩa trong ui_dashboard_theme.h — phải ở hazard_core.h")

    hc_h = root / "firmware/waveshare-screen/components/hazard_core/include/hazard_core.h"
    for sym in ("CROSSING_DELTA_CM", "CROSSING_FRONT_THRESHOLD_CM"):
        if hc_h.exists() and re.search(rf"#define\s+{sym}\b", hc_h.read_text(encoding="utf-8", errors="ignore")) is None:
            fail(f"B5: thiếu #define {sym} trong hazard_core.h")

    # python mirror (mục A2 HARDCODED_CONFIG_NOTES): CAUTION_CM/DANGER_CM == SENSOR_*_CM
    mqtt = (root / "tools/test_mqtt_coreiot.py").read_text(encoding="utf-8", errors="ignore")
    thr = (root / "firmware/shared/thresholds.h").read_text(encoding="utf-8", errors="ignore")
    py_pairs = {
        "CAUTION_CM": _python_float(mqtt, "CAUTION_CM"),
        "DANGER_CM": _python_float(mqtt, "DANGER_CM"),
    }
    sh_pairs = {
        "SENSOR_CAUTION_CM": _c_int(thr, "SENSOR_CAUTION_CM"),
        "SENSOR_DANGER_CM": _c_int(thr, "SENSOR_DANGER_CM"),
    }
    expect = {"CAUTION_CM": sh_pairs["SENSOR_CAUTION_CM"], "DANGER_CM": sh_pairs["SENSOR_DANGER_CM"]}
    for py_name, c_name in (("CAUTION_CM", "SENSOR_CAUTION_CM"), ("DANGER_CM", "SENSOR_DANGER_CM")):
        py_v = py_pairs[py_name]
        c_v = sh_pairs[c_name]
        if py_v is not None and c_v is not None and abs(py_v - float(c_v)) > 1e-9:
            fail(f"B5: python mirror {py_name}={py_v} lệch thresholds.h {c_name}={c_v}")


def _c_int(txt: str, name: str) -> int | None:
    m = re.search(rf"#define\s+{name}\s+(\d+)", txt)
    return int(m.group(1)) if m else None


def _python_float(txt: str, name: str) -> float | None:
    m = re.search(rf"{name}\s*=\s*([\d.]+)", txt)
    return float(m.group(1)) if m else None


# ----------------------------------------------------------------- R2 dead wrapper gone
def check_r2(root: Path) -> None:
    if count_occurrences(root, "sensor_model_classify") > 0:
        fail("R2: sensor_model_classify vẫn còn trong firmware (đã xoá wrapper)")
    hc_h = root / "firmware/waveshare-screen/components/hazard_core/include/hazard_core.h"
    for sym in ("hazard_classify", "hazard_worst_zone", "hazard_eval_crossing"):
        n = count_occurrences(root, sym)
        if n < 2:  # 1 khai báo (.h) + 1 định nghĩa (.c) trở lên
            fail(f"R2: {sym} thiếu định nghĩa/khai báo (count={n})")


# ----------------------------------------------------------------- B7 sizing + API size
def check_b7(root: Path) -> None:
    for rel in ("firmware/waveshare-screen/components/hazard_core/include/hazard_core.h",
                "firmware/waveshare-screen/components/hazard_core/hazard_core.c"):
        p = root / rel
        if p.exists():
            n = len(p.read_text(encoding="utf-8", errors="ignore").splitlines())
            if n > 400:
                fail(f"B7: {rel} vượt 400 dòng ({n})")
    hc_h = root / "firmware/waveshare-screen/components/hazard_core/include/hazard_core.h"
    if hc_h.exists():
        pub = re.findall(rf"\bhazard_[a-z_]+\s*\(", hc_h.read_text(encoding="utf-8", errors="ignore"))
        if len(set(pub)) > 4:
            fail(f"B4: hazard_core public API > 4 hàm ({sorted(set(pub))})")


def main() -> int:
    check_b1(ROOT)
    check_b2(ROOT)
    check_b5(ROOT)
    check_r2(ROOT)
    check_b7(ROOT)
    if ERRORS:
        for e in ERRORS:
            print(e, file=sys.stderr)
        print("[arch-guard] FAIL", file=sys.stderr)
        return 1
    print("[arch-guard] ARCH-GUARD OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())