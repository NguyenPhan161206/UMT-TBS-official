"""Unit test cho tools/accuracy/measure_accuracy.py — chạy không cần board.

    python -m pytest tools/accuracy -q
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import measure_accuracy as ma  # noqa: E402


def test_parse_valid_line():
    s, f = ma.parse_acc_line("ACC S0 raw=101.5 out=100.2 has=1 status=OK n=4 pulse=5920")
    assert s == 0
    assert f == {"raw": "101.5", "out": "100.2", "has": "1", "status": "OK", "n": "4", "pulse": "5920"}


def test_parse_reject_line_keeps_reason_with_spaces():
    s, f = ma.parse_acc_line(
        "ACC S2 raw=nan pulse=31000 calc=534.1 rej=Khong co Echo hop le hoac vuot pham vi do")
    assert s == 2
    assert f["rej"] == "Khong co Echo hop le hoac vuot pham vi do"
    assert f["calc"] == "534.1" and f["raw"] == "nan"


def test_parse_ignores_other_lines():
    assert ma.parse_acc_line("DIST: [1.0, 0, 0, 0, 0, 0]") is None
    assert ma.parse_acc_line("[S0] REJECT: timeout") is None


def _sample(raw, out=math.nan, has=0, true_cm=100.0, angle=None, height=None):
    return ma.Sample(0.0, 0, true_cm, angle, height, "t", raw, out, has, "OK" if has else "WARMUP",
                     3, 0, math.nan, "" if not math.isnan(raw) else "timeout")


def test_summarize_point():
    samples = [_sample(98.0, 100.0, 1), _sample(102.0, 100.0, 1), _sample(104.0, 101.0, 1),
               _sample(math.nan)]
    p = ma.summarize_point(samples)
    assert p["attempts"] == 4 and p["reject_pct"] == 25.0
    assert p["raw"]["n"] == 3 and p["raw"]["mean"] == (98 + 102 + 104) / 3
    assert abs(p["raw"]["bias"] - 4 / 3) < 1e-9
    assert p["raw"]["mae"] == (2 + 2 + 4) / 3 and p["raw"]["max_abs_err"] == 4
    assert p["filtered"]["n"] == 3 and abs(p["filtered"]["bias"] - 1 / 3) < 1e-9
    assert p["has_output_pct"] == 100.0
    assert "| S0 | 100 | 4 | 25.0 |" in ma.format_points([p])


def test_summarize_point_no_valid_samples():
    p = ma.summarize_point([_sample(math.nan), _sample(math.nan)])
    assert p["reject_pct"] == 100.0 and p["raw"]["n"] == 0 and math.isnan(p["has_output_pct"])
    assert "–" in ma.format_points([p])


def test_beam_detection_uses_tolerance():
    cell = [_sample(150.0, true_cm=150, angle=20, height=50),
            _sample(40.0, true_cm=150, angle=20, height=50),   # vọng từ sàn: hợp lệ nhưng không phải vật
            _sample(math.nan, true_cm=150, angle=20, height=50),
            _sample(160.0, true_cm=150, angle=20, height=50)]
    c = ma.summarize_beam(cell, tol_cm=15)
    assert c["valid_pct"] == 75.0 and c["detect_pct"] == 50.0 and c["raw_mean_hit"] == 155.0


def test_group_and_csv_roundtrip(tmp_path):
    samples = [_sample(100.0, 100.0, 1), _sample(math.nan),
               _sample(150.0, true_cm=150, angle=10, height=50)]
    path = tmp_path / "a.csv"
    ma.write_csv(path, samples)
    back = ma.load_csv([path])
    assert len(back) == 3 and math.isnan(back[1].raw) and back[2].angle_deg == 10.0
    assert back[0].angle_deg is None
    g = ma.group(back, beam=False)
    assert sorted(k[1] for k in g) == [100.0, 150.0]


def test_analyze_cli(tmp_path, capsys):
    path = tmp_path / "acc.csv"
    ma.write_csv(path, [_sample(99.0, 100.0, 1), _sample(101.0, 100.0, 1)])
    assert ma.main(["analyze", str(path)]) == 0
    assert "| S0 | 100 | 2 | 0.0 |" in capsys.readouterr().out


def test_analyze_writes_markdown_table(tmp_path, capsys):
    path = tmp_path / "acc_S2_100cm.csv"
    ma.write_csv(path, [_sample(99.0, 100.0, 1), _sample(101.0, 100.0, 1)])
    md = tmp_path / "out" / "acc.md"
    assert ma.main(["analyze", str(path), "--md", str(md)]) == 0
    text = md.read_text(encoding="utf-8")
    assert "| S0 | 100 | 2 | 0.0 |" in text and "acc_S2_100cm.csv" in text
