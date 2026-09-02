# COREIOT_ACCEPTANCE_TOOL_LOG.md — B9 (phần A: tool + snapshot rule-chain V2)

- **Ngày:** 2026-09-02
- **Người thực hiện:** dev-orchestrator (B9 checklist)
- **Mục tiêu:** hoàn tất phần local/CI của B9 — port `tools/test_mqtt_coreiot.py` sang V2,
  port snapshot rule-chain CoreIoT sang ngữ nghĩa V2, làm gate R11 hoạt động thật; phần
  token/flash/nghiệm thu dashboard chờ user.

## File đã sửa/tạo
| File | Loại | Nội dung |
|------|------|----------|
| `cloud/coreiot/rule_chain/supersonic_rule_chain.json` | tạo | Snapshot rule-chain V2 (import lên CoreIoT ở bước nghiệm thu): giữ nguyên cấu trúc sơ đồ 7 nút (save telemetry/attributes → type switch → transform → change originator waveshare-screen → shared attributes notifyDevice=true → save timeseries), **script transform đổi sang field V2** `d1..d6` + `nearest_cm` + `has_nearest`; zone `DANGER <= 30`, `CAUTION <= 100` (mirror `firmware/shared/thresholds.h` R3/R11); nhúng text `caution_cm = 100, danger_cm = 30` để `check_rulechain_thresholds.py` đối chiếu được. |
| `tools/test_mqtt_coreiot.py` | tạo | Publisher Paho-MQTT V2: đọc secret từ `config/keys.json` (R1) / env `COREIOT_TOKEN` / `--token`; `build_payload()` sinh đúng format sensor-node (`d1..d6`, `nearest_cm`, `has_nearest`) + `warning_status`; `--dry-run` không cần token (CI/local); `--loop`/`--interval`/`--distance`; paho import lazy (test không phụ thuộc paho). |
| `tools/requirements.txt` | tạo | `paho-mqtt>=1.6,<3` (chỉ cần khi chạy thật). |
| `tools/guard/check_rulechain_thresholds.py` | sửa | **Fix bug R11 gate**: KEYS thêm `SENSOR_CAUTION_CM`/`SENSOR_DANGER_CM`; regex hỗ trợ `#define SENSOR_CAUTION_CM 100` (trước đây chỉ khớp `"caution_cm": 100` → th_vals luôn rỗng → gate luôn SKIP). |
| `tools/guard/test_guard.py` | sửa | +5 test: build_payload format V2, classify boundary (29.9/30/30.1/100/100.1), dry-run không token, snapshot chứa 100/30, gate R11 OK. |
| `.opencode/docs/ROADMAP_CHECKLIST.md` | sửa | Hàng B9 cập nhật trạng thái port xong. |

## Kết quả kiểm thử (lệnh + kết quả)
```bash
~/.venv-platformio/bin/python -m pytest tools/guard/test_guard.py -q
# 16 passed in 0.78s

python3 tools/guard/check_rulechain_thresholds.py
# CHECK-RULECHAIN OK: thresholds.h {100.0, 30.0} đều có mặt trong rule-chain.

python3 tools/guard/scan_secrets.py
# SECRET-SCAN OK: no secret patterns found.

/tmp/opencode/gitleaks/gitleaks detect --source . --no-banner
# no leaks found

python3 tools/test_mqtt_coreiot.py --dry-run --distance 15.5
# [DRY-RUN] warning_status = DANGER
```

## Hướng dẫn vận hành/demo (chờ token MỚI — R11)
1. User tạo account/2 device trên `app.coreiot.io`, sinh **token MỚI** (KHÔNG tái dùng token repo cũ).
2. Điền `config/keys.json` (`SENSOR_NODE_DEVICE_TOKEN`, `WAVESHARE_SCREEN_DEVICE_TOKEN`, `WIFI_SSID`, `WIFI_PASSWORD`) + import `cloud/coreiot/rule_chain/supersonic_rule_chain.json` vào CoreIoT rule-chains (gán làm root chain của sensor-node).
3. `pip install -r tools/requirements.txt`; chạy:
   - Thử không cần board: `python3 tools/test_mqtt_coreiot.py --dry-run --distance 80`
   - Nghiệm thu: `python3 tools/test_mqtt_coreiot.py --distance 15.5` → dashboard waveshare-screen hiện `warning_status = DANGER`; lặp với 60 (CAUTION), 150 (NORMAL); `--loop --interval 2` để xem diễn biến.
4. Flash 2 board, bật CoreIoT → nghiệm thu dashboard thật; ghi tiếp vào log nghiệm thu (phần B).

## Ghi chú / hạn chế
- Phần flash + kiểm tra dashboard thật là **B9-B**, cần token/user thao tác trên console CoreIoT.
- `check_rulechain_thresholds.py` (best-effort) giờ gate thật: CI sẽ đỏ nếu snapshot lệch ngưỡng 100/30.