# T1_4_DATA_RECORDER_REPLAYER_LOG — Telemetry Data Recorder & Replayer (V2)

> Ngày: 2026-09-10 · Component: `tools/recorder/` · Nhiệm vụ: **T1.4** ✅

## 1. Mục tiêu
Xây dựng bộ công cụ Python độc lập phục vụ việc **thu thập, lưu trữ, và phát lại dữ liệu cảm biến thật**:
1. **`data_recorder.py`**:
   - Thu thập luồng dữ liệu 6 cảm biến từ cổng Serial (USB CDC / UART từ board thật), MQTT CoreIoT, hoặc Mock generator.
   - Lưu trữ theo chuẩn JSON Lines (`.jsonl`) kèm timestamp (ms), cờ valid, khoảng cách gần nhất và nhãn kịch bản.
2. **`data_replayer.py`**:
   - Đọc các tệp `.jsonl` và phát lại theo đúng nhịp thời gian thực (`elapsed_ms`) với hệ số tốc độ tuỳ chọn (`--speed`, `--loop`).
   - Hỗ trợ các đích phát:
     - `--target udp`: Gửi JSON tới `127.0.0.1:9090` (sẵn sàng làm nguồn cấp dữ liệu cho **T1.2 LVGL Simulator**).
     - `--target console`: Vẽ radar và bảng khoảng cách màu ANSI trực quan (SAFE/CAUTION/DANGER).
     - `--target mqtt`: Publish telemetry lên CoreIoT MQTT broker.
3. Không phụ thuộc vào phần cứng: có thể chạy sinh dữ liệu mẫu và phát lại 100% trên PC ngay cả khi chưa cắm board.

## 2. File tạo mới
- **`tools/recorder/data_recorder.py`**: Công cụ ghi dữ liệu (Serial / MQTT / Mock $\rightarrow$ JSONL).
- **`tools/recorder/data_replayer.py`**: Công cụ phát lại dữ liệu (JSONL $\rightarrow$ Console / UDP / MQTT).
- **`tools/recorder/test_recorder_replayer.py`**: Bộ unit test tự động (8 test cases).
- **`data/recordings/sample_approaching_obstacle.jsonl`**: Dataset mẫu vật cản tiếp cận vùng DANGER bên hông phải xe tải.
- **`data/recordings/sample_multi_sensor_active.jsonl`**: Dataset mẫu nhiều cảm biến hoạt động đồng thời.

## 3. Kết quả kiểm thử (DoD)
```powershell
# 1. Unit tests tự động cho T1.4
$env:PYTHONUTF8="1"; $env:PYTHONIOENCODING="utf-8"; python -m pytest tools/recorder/test_recorder_replayer.py -v
# → 8 passed in 0.62s ✅

# 2. R1 — Quét secret trong toàn bộ file tracked
python tools/guard/scan_secrets.py
# → SECRET-SCAN OK: no secret patterns found. ✅

# 3. Kiểm tra regression toàn bộ guard
$env:PYTHONUTF8="1"; $env:PYTHONIOENCODING="utf-8"; python -m pytest tools/guard/test_guard.py -q
# → 16 passed in 0.97s ✅

# 4. Host tests sensor-node
cd firmware/sensor-node && pio test -e native
# → 22 succeeded in 00:00:01.405 ✅

# 5. R7 — File length gate (<= 400 dòng/file)
# data_recorder.py: 290 dòng | data_replayer.py: 245 dòng | test_recorder_replayer.py: 135 dòng ✅
```

## 4. Hướng dẫn vận hành / Demo

### A. Ghi dữ liệu (Recorder)
```powershell
# 1. Ghi dữ liệu giả lập (không cần board) trong 10 giây:
python tools/recorder/data_recorder.py --source mock --scenario approach --duration 10 -o data/recordings/my_test.jsonl

# 2. Ghi dữ liệu thật từ board cắm cổng Serial (khi có board):
python tools/recorder/data_recorder.py --source serial --port COM3 --tag "vat_can_ben_phai"

# 3. Ghi dữ liệu nhận từ cloud CoreIoT:
python tools/recorder/data_recorder.py --source mqtt --duration 30
```

### B. Phát lại dữ liệu (Replayer)
```powershell
# 1. Phát lại trực quan trên Terminal với tốc độ bình thường:
python tools/recorder/data_replayer.py -i data/recordings/sample_approaching_obstacle.jsonl --target console --speed 1.0

# 2. Phát lại lặp vô tận bắn vào UDP 127.0.0.1:9090 (để nuôi T1.2 LVGL Simulator):
python tools/recorder/data_replayer.py -i data/recordings/sample_approaching_obstacle.jsonl --target udp --loop

# 3. Phát lại lên MQTT CoreIoT:
python tools/recorder/data_replayer.py -i data/recordings/sample_approaching_obstacle.jsonl --target mqtt --speed 1.0
```
