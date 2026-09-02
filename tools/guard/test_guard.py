#!/usr/bin/env python3
"""test_guard.py — pytest cho các script trong tools/guard/ (B7).

Chạy:  python3 -m pytest tools/guard/test_guard.py -q
Coverage: gen_credentials (validate + sinh header), scan_secrets (bắt token /
sạch), check_rulechain_thresholds (skip best-effort), check_size (cap/missing).

KHÔNG tạo/đụng file tracked nào: toàn bộ file tạm nằm trong tmp_path.
"""
from __future__ import annotations

import json
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
    leak.write_text('COREIOT_SENSOR_NODE_DEVICE_TOKEN = "ABCdef1234567890"\n', encoding="utf-8")
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
    # B0b chưa import rule-chain → best-effort, exit 0 (không chặn CI).
    p = run_script("check_rulechain_thresholds.py")
    assert p.returncode == 0
    assert "SKIP" in p.stdout or "OK" in p.stdout


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