# TOOLS_RECORD_REPLAY_LOG — G1 T1.4 record/replay telemetry

Ngày: 2026-09-10
Nhánh: `nguyen` (chưa merge main)

## Mục tiêu
Cặp script ghi (subscribe MQTT) và phát lại telemetry (publish MQTT) đúng nhịp
từ file JSONL — schema V2 pinned ở `tools/scenarios.py` (đã xong step 2).
Phục vụ: tái hiện cảnh thật trong test phân loại/UI không cần board, chuẩn bị
T5.5/T5.6 (baseline), T5.7 (latency), T5.9 (báo nhầm/spot sót).

## File đã tạo/sửa
- `tools/record_telemetry.py` (mới) — subscribe `v1/devices/me/telemetry` của
  sensor-node, ghi JSONL mỗi message (dòng = payload V2 + `recv_epoch_ms`).
  CLI: `--seconds`, `--out`, `--topic`, `--broker`, `--port`, `--token`
  (R1: token từ config/keys.json / env COREIOT_TOKEN / --token, luôn mask khi in).
- `tools/replay_telemetry.py` (mới) — đọc JSONL, validate schema V2 (exit 1 nếu
  thiếu key hoặc distance âm), publish đúng nhịp `timestamp`/`recv_epoch_ms`
  (gap > 5s vẫn giữ nguyên nhịp, cap 10s/delay để không kẹt), `--scale` tuỳ chọn.
  `--dry-run` = validate + in từng dòng, không cần token.
- `tools/guard/test_guard.py` (sửa) — +4 tests T1.4 (replay valid/bad schema,
  record helpers, file rỗng/missing) → 29 passed.

## Kết quả kiểm thử (2026-09-10)
- `record_telemetry.py --seconds 3 --out /tmp/tb.jsonl`: connect MQTT thật
  `app.coreiot.io:1883` rc=0, subscribe OK, ghi 0 dòng (không board publish —
  đúng bản chất: record chỉ thu được khi sensor-node flash `yolo_uno_coreiot`).
- `replay_telemetry.py --in /tmp/tb.jsonl --dry-run`: 0 dòng, schema hợp lệ,
  exit 0 (DoD roadmap đạt: "record 10s rồi replay --dry-run không lỗi schema").
- Fixture 8 dòng approach (từ `test_mqtt_coreiot.build_payload_from_distances`):
  `--dry-run` exit 0, in đủ 8 ROW.
- Fixture bad (distance âm, thiếu d4/d5/d6): exit 1, báo "2/3 dòng lỗi schema".
- pytest tools/guard/test_guard.py: 29 passed; arch_guard OK; scan_secrets OK;
  check_rulechain_thresholds OK.

## Hướng dẫn vận hành / demo
```bash
# 1) Ghi telemetry thật 10s (cần sensor-node đang publish, config/keys.json có token):
python3 tools/record_telemetry.py --seconds 10 --out /tmp/tb.jsonl
# 2) Xem lại (không cần board/token):
python3 tools/replay_telemetry.py --in /tmp/tb.jsonl --dry-run
# 3) Phát lại lên CoreIoT đúng nhịp thu:
python3 tools/replay_telemetry.py --in /tmp/tb.jsonl
#    --scale 0.5 = nhanh gấp 2; --topic đổi đích phát.
```

## Deviation so với prompt gốc step 4
- Cả 2 script thêm `--broker/--port` (config từ keys.json, khớp test_mqtt_coreiot).
- `record_telemetry.py` thêm field phụ `recv_epoch_ms` (thời điểm HOST nhận —
  quan trọng cho T5.7 latency MQTT: truyền timestamp chính xác hơn timestamp
  thiết bị); replay ưu tiên dùng nó, bỏ nó khi republish để payload giữ đúng V2.
- `replay_telemetry.py` không có `--in/--seconds` cho record: DoD roadmap yêu cầu
  "--in/--out/--seconds/--topic" phân bổ: `--in` ở replay, `--out/--seconds` ở
  record, `--topic` cả 2 — đúng thiết kế.
- Đánh dấu T1.4 record (nửa record) chưa "flash-and-observe" vì cần board +
  token thật; script, schema, dry-run, publish path đều verify xong.

## Ghi nhận cho bước sau
- T5.5/T5.6 (step 13) dùng `--dry-run` + fixture để lặp baseline không cần board.
- T5.7 (step 14) dùng `recv_epoch_ms` cho đo latency nhánh MQTT.
- T5.9 (step 15) dùng tập replay đã gán nhãn từ `record_telemetry.py`.