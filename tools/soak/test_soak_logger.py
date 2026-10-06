"""Unit test cho tools/soak/soak_logger.py — chạy không cần board.

    python -m pytest tools/soak -q
"""
from __future__ import annotations

import sys
import threading
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import soak_logger as sl  # noqa: E402
from soak_logger import Row  # noqa: E402

NODE = ("SOAK node up={up} boot={boot} rr={rr} heap={heap} minheap={minheap} blk=60000 "
        "tx_ok={tx} tx_fail={fail} mqtt_rc={mrc} hwm_sensor=1800 hwm_net={hwm} hwm_buzz=900")
SCR = ("SOAK scr up={up} boot={boot} rr={rr} iheap={heap} iminheap={minheap} iblk=30000 "
       "psram=4000000 psrammin=3900000 rx={rx} linkdn={ld} maxgap_ms={gap} mqtt_rc={mrc} hwm_lvgl=5000")


def test_classify():
    assert sl.classify("SOAK node up=60 boot=1 rr=POWERON") == "SOAK"
    assert sl.classify("BOOT scr boot=3 rr=PANIC") == "BOOT"
    assert sl.classify("Guru Meditation Error: Core  0 panic'ed (LoadProhibited)") == "EVENT"
    assert sl.classify("\x1b[0;31mE (1234) coreiot_client: something\x1b[0m") == "EVENT"
    assert sl.classify("I (99) collision_dashboard: ESP-NOW link DOWN") == "EVENT"
    assert sl.classify("DIST: [12.0, 0.0, 0.0, 0.0, 0.0, 0.0]") is None
    assert sl.classify("[S0] REJECT: timeout | Pulse: 0 us") is None


def test_parse_heartbeat_types():
    kind, board, f = sl.parse_heartbeat("I (5) x: SOAK scr up=60 boot=2 rr=TASK_WDT rx=10 hwm_lvgl=-1")
    assert (kind, board) == ("SOAK", "scr")
    assert f == {"up": 60, "boot": 2, "rr": "TASK_WDT", "rx": 10, "hwm_lvgl": -1}


def test_sum_deltas_handles_reset():
    beats = [sl.Beat(0, {"boot": 1, "tx_ok": 100}), sl.Beat(60, {"boot": 1, "tx_ok": 700}),
             sl.Beat(120, {"boot": 2, "tx_ok": 50}), sl.Beat(180, {"boot": 2, "tx_ok": 650})]
    assert sl.sum_deltas(beats, "tx_ok") == 600 + 50 + 600


def test_linear_slope():
    assert sl.linear_slope([0, 1, 2], [10, 8, 6]) == -2
    assert sl.linear_slope([1], [1]) is None


def test_rate_limiter_flushes_dropped_count():
    rl = sl.RateLimiter(limit=2)
    assert rl.allow(0.0) == (True, 0)
    assert rl.allow(1.0) == (True, 0)
    assert rl.allow(2.0) == (False, 0)
    assert rl.allow(3.0) == (False, 0)
    assert rl.allow(61.0) == (True, 2)  # sang phút mới: báo đã bỏ 2 dòng


def _synthetic_rows(hours: float = 2.0) -> list[Row]:
    rows: list[Row] = [Row(0.5, "sensor", "BOOT", "BOOT node boot=7 rr=POWERON"),
                       Row(0.6, "screen", "BOOT", "BOOT scr boot=3 rr=POWERON")]
    n = int(hours * 60)
    for i in range(1, n + 1):
        t = i * 60.0
        reset = i > n // 2  # sensor-node PANIC ở giữa
        up = (i - n // 2) * 60 if reset else i * 60
        boot, rr = (8, "PANIC") if reset else (7, "POWERON")
        tx = (i - n // 2) * 600 if reset else i * 600
        rows.append(Row(t, "sensor", "SOAK", NODE.format(
            up=up, boot=boot, rr=rr, heap=200000, minheap=190000, tx=tx, fail=0,
            mrc=0, hwm=1500 if i == 5 else 2000)))
        rows.append(Row(t + 0.2, "screen", "SOAK", SCR.format(
            up=i * 60, boot=3, rr="POWERON", heap=80000 - i * 10, minheap=70000,
            rx=i * 597, ld=1 if i > 30 else 0, gap=1800 if i == 31 else 120, mrc=2 if i > 50 else 0)))
    rows.append(Row(n // 2 * 60 + 1, "sensor", "PORT", "PORT_LOST device disconnected"))
    rows.append(Row(n // 2 * 60 + 3, "sensor", "EVENT", "Guru Meditation Error"))
    return rows


def test_analyze_synthetic_run():
    res = sl.analyze(_synthetic_rows())
    node, scr, e = res["node"], res["scr"], res["espnow"]

    assert node["beats"] == 120 and scr["beats"] == 120
    assert node["resets"]["count"] == 1 and node["resets"]["unexpected"] == 1
    assert node["resets"]["events"][0]["rr"] == "PANIC"
    assert scr["resets"]["count"] == 0
    assert node["port_lost"] == 1 and node["events"] == 1
    assert node["hwm_min"]["hwm_net"] == 1500
    assert node["heap"]["slope_kb_per_h"] == 0
    # màn hình mất 10 byte/phút = 0.586 KB/h
    assert abs(scr["heap"]["slope_kb_per_h"] - (-600 / 1024)) < 1e-6
    assert scr["mqtt_reconnects"] == 2

    # tx: 119 khoảng * 600 trừ phần mất do reset (sau reset cộng giá trị tính từ 0)
    assert e["tx_ok"] == 59 * 600 + 600 + 59 * 600
    assert e["rx"] == 119 * 597
    assert e["link_down"] == 1 and e["maxgap_ms"] == 1800 and e["windows_gap_over_limit"] == 1

    report = sl.format_report(res)
    assert "PANIC@" in report and "KHÔNG ĐẠT" in report


def test_missed_boot_counts_as_unexpected():
    beats = [sl.Beat(60, {"boot": 1, "rr": "POWERON"}), sl.Beat(120, {"boot": 3, "rr": "TASK_WDT"})]
    r = sl._resets(beats, [])
    assert r["count"] == 2 and r["missed_reason"] == 1 and r["unexpected"] == 2


def test_csv_roundtrip(tmp_path):
    import csv
    path = tmp_path / "s.csv"
    with path.open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(sl.CSV_HEADER)
        w.writerow(["2026-10-06T21:00:00.000", "60.000", "sensor", "SOAK", "SOAK node up=60 boot=1"])
    (row,) = sl.load_csv(path)
    assert row.kind == "SOAK" and row.t_s == 60.0


class _FakeSerial:
    def __init__(self, chunks):
        self.chunks = list(chunks)
        self.in_waiting = 0

    def read(self, n):
        if not self.chunks:
            raise OSError("device disconnected")
        return self.chunks.pop(0)

    def close(self):
        pass


def test_read_port_reopens_after_disconnect():
    """Cổng mất giữa chừng (board reset) -> PORT_LOST, mở lại, tiếp tục đọc."""
    stop = threading.Event()
    opened = []
    sessions = [
        _FakeSerial([b"SOAK node up=60 boot=1\nDIST: [1]\n"]),
        "fail",  # USB-CDC chưa xuất hiện lại
        _FakeSerial([b"BOOT node boot=2 rr=PANIC\n", b"Guru Meditation Error\n"]),
    ]

    def open_fn():
        if not sessions:
            stop.set()
            raise OSError("no more")
        s = sessions.pop(0)
        opened.append(s)
        if s == "fail":
            raise OSError("could not open port")
        return s

    got = []
    sl.read_port(open_fn, "sensor", lambda src, kind, line: got.append((kind, line)), stop, retry_s=0)
    kinds = [k for k, _ in got if k != "RAW"]
    assert kinds == ["PORT", "SOAK", "PORT", "PORT", "BOOT", "EVENT", "PORT"]
    # Lần mở thất bại ngay sau PORT_LOST không ghi thêm dòng (tránh spam khi USB biến mất lâu).
    assert [l for k, l in got if k == "PORT"] == [
        "PORT_OPEN", "PORT_LOST device disconnected", "PORT_OPEN", "PORT_LOST device disconnected"]
    assert any(k == "RAW" and l.startswith("DIST") for k, l in got)


class _Port:
    def __init__(self, device, vid=None, pid=None):
        self.device, self.vid, self.pid = device, vid, pid


def test_resolve_port_autodetect_by_vid_pid():
    """COM4 trên máy dev là Bluetooth: phải chọn theo VID:PID, không theo số COM."""
    ports = lambda: [_Port("COM4"), _Port("COM7", 0x303A, 0x1001), _Port("COM9", 0x1A86, 0x55D3)]
    assert sl.resolve_port("auto", "sensor", ports) == "COM7"
    assert sl.resolve_port(None, "screen", ports) == "COM9"
    assert sl.resolve_port("COM12", "sensor", ports) == "COM12"  # chỉ định tay thì giữ nguyên


def test_resolve_port_missing_or_ambiguous_exits():
    import pytest
    with pytest.raises(SystemExit):
        sl.resolve_port("auto", "sensor", lambda: [_Port("COM4")])
    with pytest.raises(SystemExit):
        sl.resolve_port("auto", "sensor", lambda: [_Port("COM7", 0x303A, 0x1001), _Port("COM8", 0x303A, 0x1001)])
