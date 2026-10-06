"""serial_ports.py — tự dò cổng COM của 2 board theo VID:PID (dùng chung cho tools/latency, soak, accuracy).

Số COM đổi theo máy/cổng USB (máy dev hiện tại: sensor-node COM7, màn hình COM9; COM4 là Bluetooth),
nên các script nhận `auto` (mặc định) và tìm theo phần cứng:
  sensor-node: USB-Serial-JTAG của ESP32-S3, VID:PID 303A:1001
  màn hình:    cầu UART CH343 của Waveshare, VID:PID 1A86:55D3
"""
from __future__ import annotations

from typing import Callable, Iterable

KNOWN_IDS: dict[str, tuple[tuple[int, int], ...]] = {
    "sensor": ((0x303A, 0x1001),),
    "screen": ((0x1A86, 0x55D3),),
}


def _list_ports():
    from serial.tools import list_ports  # lazy: test truyền danh sách giả

    return list_ports.comports()


def resolve_port(port: str | None, role: str, ports: Callable[[], Iterable] = _list_ports) -> str:
    """Trả về `port` nếu đã chỉ định; với None/'auto' thì dò theo VID:PID của `role`."""
    if port and port.lower() != "auto":
        return port
    ids = KNOWN_IDS[role]
    found = [p for p in ports() if (getattr(p, "vid", None), getattr(p, "pid", None)) in ids]
    if len(found) == 1:
        return found[0].device
    want = ", ".join(f"{v:04X}:{p:04X}" for v, p in ids)
    if not found:
        raise SystemExit(f"Không tìm thấy cổng {role} (VID:PID {want}). Cắm board hoặc chỉ định cổng; "
                         "xem: python -m serial.tools.list_ports -v")
    raise SystemExit(f"Có {len(found)} cổng khớp {role} ({', '.join(p.device for p in found)}); "
                     "hãy chỉ định cổng.")
