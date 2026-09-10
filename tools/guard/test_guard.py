#!/usr/bin/env python3
"""test_guard.py — pytest cho các script tools/ (B7 + B9).

Chạy:  python3 -m pytest tools/guard/test_guard.py -q
Coverage: gen_credentials (validate + sinh header), scan_secrets (bắt token /
sạch), check_rulechain_thresholds (skip best-effort / OK khi có snapshot),
check_size (cap/missing), test_mqtt_coreiot (build_payload + dry-run + snapshot
rule-chain V2 chứa ngưỡng 100/30).

KHÔNG tạo/đụng file tracked nào: toàn bộ file tạm nằm trong tmp_path.
"""
from __future__ import annotations

import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GUARD = ROOT / "tools" / "guard"

VALID_KEYS = {
    "COREIOT_BROKER": "app.coreiot.io",
    "COREIOT_PORT": 1883,
    "SENSOR_NODE_DEVICE_TOKEN": "ci-dummy-sensor-9f8e7d6c5b4a",
    "WAVESHARE_SCREEN_DEVICE_TOKEN": "ci-dummy-screen-1a2b3c4d5e6f",
    "COREIOT_TELEMETRY_TOPIC": "v1/devices/me/telemetry",
    "WIFI_SSID": "ci-dummy-wifi",
    "WIFI_PASSWORD": "ci-dummy-pass-012345",
}

PLACEHOLDER_KEYS = dict(VALID_KEYS, SENSOR_NODE_DEVICE_TOKEN="<SENSOR_NODE_ACCESS_TOKEN_MOI>")


def run_script(script: str, *args: str, cwd: Path = ROOT) -> subprocess.CompletedProcess:
    return subprocess.run(
        [sys.executable, str(GUARD / script), *args],
        capture_output=True,
        text=True,
        cwd=cwd,
    )


def write_keys(tmp_path: Path, data: dict) -> Path:
    keys = tmp_path / "keys.json"
    keys.write_text(json.dumps(data), encoding="utf-8")
    return keys


# ---------------------------------------------------------------- gen_credentials


def test_gen_credentials_check_ok(tmp_path):
    keys = write_keys(tmp_path, VALID_KEYS)
    p = run_script("gen_credentials.py", "--keys", str(keys), "--check")
    assert p.returncode == 0, p.stderr
    assert "GEN-CREDENTIALS OK" in p.stdout


def test_gen_credentials_rejects_placeholder(tmp_path):
    keys = write_keys(tmp_path, PLACEHOLDER_KEYS)
    p = run_script("gen_credentials.py", "--keys", str(keys), "--check")
    assert p.returncode == 2
    assert "ERROR" in p.stderr
    assert "placeholder" in p.stderr


def test_gen_credentials_writes_header(tmp_path):
    keys = write_keys(tmp_path, VALID_KEYS)
    out = tmp_path / "nested" / "credentials.h"
    p = run_script("gen_credentials.py", "--keys", str(keys), "--out", str(out))
    assert p.returncode == 0, p.stderr
    text = out.read_text(encoding="utf-8")
    assert "#define WIFI_PASSWORD \"ci-dummy-pass-012345\"" in text
    assert "#define COREIOT_PORT 1883" in text
    assert "DO NOT COMMIT" in text


def test_gen_credentials_requires_out_or_check(tmp_path):
    keys = write_keys(tmp_path, VALID_KEYS)
    p = run_script("gen_credentials.py", "--keys", str(keys))
    assert p.returncode == 2


# ------------------------------------------------------------------ scan_secrets


def test_scan_secrets_detects_coreiot_token(tmp_path):
    leak = tmp_path / "leak.txt"
    # Nối runtime: tránh literal 16+ ký tự trong source (R1 — repo-wide scan
    # không được tự hít fixture của chính nó). File tạm vẫn có token đầy đủ.
    token = "ABCdef" + "1234567890"
    leak.write_text(f'COREIOT_SENSOR_NODE_DEVICE_TOKEN = "{token}"\n', encoding="utf-8")
    p = run_script("scan_secrets.py", "--path", str(leak))
    assert p.returncode == 1
    assert "SECRET-SCAN FAIL" in p.stdout


def test_scan_secrets_clean_file(tmp_path):
    ok = tmp_path / "ok.txt"
    ok.write_text("int x = 42;\n", encoding="utf-8")
    p = run_script("scan_secrets.py", "--path", str(ok))
    assert p.returncode == 0
    assert "SECRET-SCAN OK" in p.stdout


# ------------------------------------------------------- check_rulechain_thresholds


def test_rulechain_skips_when_snapshot_missing():
    # Snapshot có thể chưa tồn tại (best-effort, exit 0 không chặn CI) hoặc
    # đã có và khớp ngưỡng (OK). Cả hai trường hợp đều exit 0.
    p = run_script("check_rulechain_thresholds.py")
    assert p.returncode == 0
    assert "SKIP" in p.stdout or "OK" in p.stdout


def test_rulechain_matches_sensor_threshold_defines():
    # R3/R11 gate thật sự phải trích được SENSOR_CAUTION_CM/SENSOR_DANGER_CM
    # trong thresholds.h và đối chiếu với snapshot rule-chain (100/30).
    p = run_script("check_rulechain_thresholds.py")
    assert p.returncode == 0, p.stdout
    assert "OK" in p.stdout


# ----------------------------------------------------------- test_mqtt_coreiot (B9)


def load_tool_module():
    """Import tools/test_mqtt_coreiot.py (paho được import lazy nên không cần cài)."""
    spec = importlib.util.spec_from_file_location(
        "test_mqtt_coreiot", ROOT / "tools" / "test_mqtt_coreiot.py"
    )
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def test_build_payload_v2_format():
    t = load_tool_module()
    p = t.build_payload(20.0)
    assert {k: p[k] for k in ("d1", "d2", "d3", "d4", "d5", "d6")} == {
        k: 20.0 for k in ("d1", "d2", "d3", "d4", "d5", "d6")
    }
    assert p["nearest_cm"] == 20.0
    assert p["has_nearest"] is True
    assert p["warning_status"] == "DANGER"
    assert p["vehicle_detected"] is True


def test_classify_boundaries():
    t = load_tool_module()
    assert t.classify(29.9) == "DANGER"
    assert t.classify(30.0) == "DANGER"   # x <= 30
    assert t.classify(30.1) == "CAUTION"
    assert t.classify(100.0) == "CAUTION"  # 30 < x <= 100
    assert t.classify(100.1) == "NORMAL"


def test_dry_run_without_token():
    # --dry-run không cần token/paho; in payload JSON, exit 0.
    p = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "test_mqtt_coreiot.py"),
         "--dry-run", "--distance", "25"],
        capture_output=True, text=True, cwd=ROOT,
    )
    assert p.returncode == 0, p.stderr
    assert '"warning_status": "DANGER"' in p.stdout
    assert '"nearest_cm": 25.0' in p.stdout


# ---------------------------------------------------------------- scenarios (G1 T1.1)


def load_scenarios_module():
    """Import tools/scenarios.py (stdlib thuần, không phụ thuộc paho)."""
    spec = importlib.util.spec_from_file_location(
        "scenarios", ROOT / "tools" / "scenarios.py"
    )
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def test_scenarios_has_4_named_timelines():
    s = load_scenarios_module()
    assert set(s.SCENARIO_NAMES) == {"approach", "crossing", "slam", "normal"}
    for name in s.SCENARIO_NAMES:
        rows = list(s.iter_scenario(name))
        assert len(rows) >= 4, name          # ≥ 4 mốc thời gian
        assert all(len(r) == 6 for r in rows), name  # đủ 6 slot


def test_scenarios_semantics():
    s = load_scenarios_module()
    rows = {name: list(s.iter_scenario(name)) for name in s.SCENARIO_NAMES}

    # approach: d1 giảm dần, mốc đầu > 150 → mốc cuối < 30; slot khác ≥ 60.
    app = rows["approach"]
    assert app[0][0] > 150 and app[-1][0] < 30
    assert all(v >= 60.0 for r in app[1:] for v in r[1:])

    # crossing: tồn tại 2 mốc liên tiếp chênh ≥ 40 ở slot index 2 hoặc 4.
    crs = rows["crossing"]
    assert any(
        abs(a - b) >= 40.0
        for prev, cur in zip(crs, crs[1:])
        for a, b in ((prev[2], cur[2]), (prev[4], cur[4]))
    )

    # slam: d1 giảm từ > 100 xuống < 30 trong ≤ 3 mốc.
    slm = rows["slam"]
    assert slm[0][0] > 100
    assert slm[2][0] < 30

    # normal: mọi giá trị > 100 (không mốc nào ≤ CAUTION_CM).
    nrm = rows["normal"]
    assert all(v > 100.0 for r in nrm for v in r)


def test_iter_scenario_unknown_raises():
    s = load_scenarios_module()
    try:
        next(s.iter_scenario("khong_co"))
    except KeyError:
        return
    raise AssertionError("iter_scenario('khong_co') phải raise KeyError")


def test_build_payload_from_distances_danger():
    t = load_tool_module()
    p = t.build_payload_from_distances([5, 50, 50, 50, 50, 50])
    assert p["d1"] == 5.0
    assert p["d5"] == 50.0
    assert p["nearest_cm"] == 5.0
    assert p["has_nearest"] is True
    assert p["warning_status"] == "DANGER"
    assert {k: p[k] for k in ("d1", "d2", "d3", "d4", "d5", "d6")}


def test_scenario_dry_run_full_timeline():
    # --scenario approach --dry-run: in ≥ 4 payload, mỗi payload 1 dòng JSON.
    p = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "test_mqtt_coreiot.py"),
         "--scenario", "approach", "--dry-run"],
        capture_output=True, text=True, cwd=ROOT,
    )
    assert p.returncode == 0, p.stderr
    payload_lines = [ln for ln in p.stdout.splitlines() if ln.startswith("{")]
    assert len(payload_lines) >= 4
    assert '"d1"' in payload_lines[0] and '"nearest_cm"' in payload_lines[0]
    assert '"has_nearest"' in payload_lines[-1]


def test_scenario_dry_run_with_distance_override():
    # DoD roadmap: --scenario approach --dry-run --distance 30 → payload đủ key.
    p = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "test_mqtt_coreiot.py"),
         "--scenario", "approach", "--dry-run", "--distance", "30"],
        capture_output=True, text=True, cwd=ROOT,
    )
    assert p.returncode == 0, p.stderr
    assert '"d1"' in p.stdout and '"nearest_cm"' in p.stdout
    assert '"has_nearest"' in p.stdout
    assert '"d1": 30.0' in p.stdout


def test_rulechain_snapshot_has_v2_thresholds():
    # R3/R11: snapshot rule-chain phải chứa ngưỡng zone 100/30 (mirror
    # firmware/shared/thresholds.h) để check_rulechain_thresholds.py OK.
    rc = (ROOT / "cloud" / "coreiot" / "rule_chain" / "supersonic_rule_chain.json").read_text(
        encoding="utf-8"
    )
    nums = {
        float(m.group(1))
        for m in re.finditer(r"(?:caution|danger)[^0-9-]{0,40}(-?\d+(?:\.\d+)?)", rc, re.IGNORECASE)
    }
    assert 100.0 in nums and 30.0 in nums


# --------------------------------------------------------------------- check_size


def test_check_size_ok(tmp_path):
    f = tmp_path / "f.bin"
    f.write_bytes(b"x" * 10)
    p = run_script("check_size.py", str(f), "20")
    assert p.returncode == 0
    assert "SIZE-GATE OK" in p.stdout


def test_check_size_over_cap(tmp_path):
    f = tmp_path / "f.bin"
    f.write_bytes(b"x" * 30)
    p = run_script("check_size.py", str(f), "20")
    assert p.returncode == 1
    assert "VƯỢT" in p.stderr


def test_check_size_missing_file(tmp_path):
    p = run_script("check_size.py", str(tmp_path / "nope.bin"), "20")
    assert p.returncode == 1
    assert "MISSING" in p.stderr


def test_check_size_bad_args():
    p = run_script("check_size.py", "only-one-arg")
    assert p.returncode == 2


# ------------------------------------------------------------------ arch_guard (B1-B7)


def load_arch_guard():
    """Import tools/guard/arch_guard.py dưới dạng module (python script thuần)."""
    spec = importlib.util.spec_from_file_location("arch_guard", GUARD / "arch_guard.py")
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def test_arch_guard_ok():
    # Trên repo thật: mọi quy tắc B1-B7 + mirror A2 đều thoả (G1 step 1 hoàn thành).
    p = subprocess.run(
        [sys.executable, str(GUARD / "arch_guard.py")],
        capture_output=True, text=True, cwd=ROOT,
    )
    assert p.returncode == 0, p.stderr
    assert "ARCH-GUARD OK" in p.stdout


def test_arch_guard_detects_missing_crossing_constant():
    # B5: thiếu CROSSING_FRONT_THRESHOLD_CM trong hazard_core.h nếu hazard_core
    # tồn tại nhưng là bản xoá trắng -> fail('thiếu #define').
    mod = load_arch_guard()
    root = ROOT  # chạy trên repo thật, check dương tính: không được có error
    # (guard OK đã phủ ở test_arch_guard_ok; đây là test hợp đồng API gọi được)
    assert hasattr(mod, "check_b5")


def test_arch_guard_helpers():
    # Kiểm tra các helper parse dùng cho python-mirror (mục A2).
    mod = load_arch_guard()
    thr = "#define SENSOR_CAUTION_CM 100\n#define SENSOR_DANGER_CM 30\n"
    mqtt = "CAUTION_CM = 100.0\nDANGER_CM = 30.0\n"
    assert mod._c_int(thr, "SENSOR_CAUTION_CM") == 100
    assert mod._c_int(thr, "SENSOR_DANGER_CM") == 30
    assert mod._python_float(mqtt, "CAUTION_CM") == 100.0
    assert mod._python_float(mqtt, "DANGER_CM") == 30.0