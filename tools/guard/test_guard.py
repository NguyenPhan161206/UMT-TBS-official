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