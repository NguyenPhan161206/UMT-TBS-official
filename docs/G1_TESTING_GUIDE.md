# Hướng dẫn kiểm tra & sử dụng Giai đoạn 1 (G1 — Mở khả năng kiểm thử)

> Trạng thái: **4/4 bước DONE**, commit trên nhánh `nguyen` (chưa merge `main`).
> Mọi lệnh chạy tại thư mục gốc repo, trừ khi ghi rõ khác.

## 0. Chuẩn bị môi trường

```bash
# cmake, pytest, pio nằm trong venv — cần export PATH
export PATH="/home/binhnguyen/.venv-pio/bin:$PATH"

# (một lần đầu) host_sim build:
cmake -S firmware/waveshare-screen/host_sim -B /tmp/host_sim
cmake --build /tmp/host_sim -j$(nproc)
```

---

## 1. Kiểm tra tự động (guard)

Mọi lệnh exit `0` là đạt; `1` = có lỗi.

```bash
python3 tools/guard/scan_secrets.py                  # R1: không lộ secret (keys.json gitignored)
python3 tools/guard/arch_guard.py                    # R2/R3/R5: kiến trúc G1 (B1–B7 + mirror A2)
python3 tools/guard/check_rulechain_thresholds.py    # R3/R11: ngưỡng 100/30 khớp rule-chain
python3 tools/guard/gen_credentials.py --check       # R1: keys.json đủ field
python3 -m pytest tools/guard/test_guard.py -q       # R10: 29 unit tests
```

Chạy 1 lệnh duy nhất để kiểm tra nhanh cả cụm:

```bash
python3 tools/guard/scan_secrets.py && python3 tools/guard/arch_guard.py \
  && python3 tools/guard/check_rulechain_thresholds.py \
  && python3 -m pytest tools/guard/test_guard.py -q
```

CI (`.github/workflows/ci.yml`) tự chạy các lệnh trên mỗi push/PR đến `main`
+ gitleaks + build firmware 2 env — không đụng nhiều, chỉ cần push là có kết quả.

---

## 2. Unit test hazard_core (logic cảnh báo thuần)

```bash
/tmp/host_sim/hazard_core_tests     # kỳ vọng: "25 checks PASSED"
# hoặc qua ctest:
cd /tmp/host_sim && ctest --output-on-failure
```

Phủ các biên: ngưỡng 19/30/31, 99/100/101, skip-stale, crossing delta=40 / FRONT<150.

---

## 3. Mô phỏng màn hình cảm biến (host_sim — không cần board)

Executable: `/tmp/host_sim/umt_dash_sim` (bản build thật từ firmware `ui_dashboard*`,
chạy qua LVGL v9.1 + SDL2, stub thay thế phần cứng).

### Cửa sổ thật (máy có display)
```bash
/tmp/host_sim/umt_dash_sim --scenario approach --exit-after 10
```

### Headless (không cần X / môi trường CI)
```bash
SDL_VIDEODRIVER=dummy /tmp/host_sim/umt_dash_sim --scenario slam --exit-after 3
# DoD CI: xvfb-run -a /tmp/host_sim/umt_dash_sim --exit-after 3 --scenario approach
```

### 4 kịch bản có sẵn (nguồn: tools/scenarios.py)
| Scenario  | Ý nghĩa                          |
|-----------|----------------------------------|
| `approach`| vật tiến gần phía FRONT (160→20cm) |
| `crossing`| xe cắt ngang phía trước           |
| `slam`    | dừng gấp (110→20cm trong ≤3 mốc)  |
| `normal`  | không có cảnh báo (>100cm)        |

### Chạy lại dữ liệu đã ghi thật
```bash
/tmp/host_sim/umt_dash_sim --replay /tmp/tb.jsonl --exit-after 5
```

Cờ hữu ích: `--exit-after <giây>`, `--interval <ms>` (tốc độ feed mốc).

---

## 4. Gửi telemetry thử lên CoreIoT (MQTT)

Token đọc từ `config/keys.json` / env `COREIOT_TOKEN` / `--token` — **không gõ token trong lệnh**.

```bash
# Một lần với khoảng cách cố định:
python3 tools/test_mqtt_coreiot.py --distance 15.5

# Lặp theo kịch bản (approach/crossing/slam/normal):
python3 tools/test_mqtt_coreiot.py --scenario approach --loop --interval 2

# Chỉ xem payload, không gửi (không cần token):
python3 tools/test_mqtt_coreiot.py --scenario approach --dry-run
python3 tools/test_mqtt_coreiot.py --distance 25 --dry-run
```

---

## 5. Thu & phát lại telemetry THẬT (cần board)

**Yêu cầu:** sensor-node flash `yolo_uno_coreiot`, đang publish MQTT, và có token trong `config/keys.json`.

```bash
# Ghi telemetry thật ra JSONL (dòng = payload V2 + recv_epoch_ms):
python3 tools/record_telemetry.py --seconds 10 --out /tmp/tb.jsonl

# Kiểm tra schema (không cần board/token):
python3 tools/replay_telemetry.py --in /tmp/tb.jsonl --dry-run

# Phát lại lên CoreIoT đúng nhịp đã thu:
python3 tools/replay_telemetry.py --in /tmp/tb.jsonl
# --scale 0.5 = nhanh gấp 2; --topic <khác> = đổi đích
```

Lưu ý: không có board đang publish thì file chỉ có 0 dòng (hợp lệ, exit 0);
file có dòng sai schema (thiếu key / distance âm) khiến `--dry-run` exit 1.

---

## 6. Demo nhanh tổng hợp (chỉ máy dev — không cần board)

```bash
export PATH="/home/binhnguyen/.venv-pio/bin:$PATH"

# (1) Đơn vị: logic cảnh báo
/tmp/host_sim/hazard_core_tests

# (2) GUI: mô phỏng kịch bản dừng gấp
SDL_VIDEODRIVER=dummy /tmp/host_sim/umt_dash_sim --scenario slam --exit-after 3

# (3) Payload MQTT V2 sample (không gửi)
python3 tools/test_mqtt_coreiot.py --scenario approach --dry-run

# (4) Guard toàn cục
python3 tools/guard/scan_secrets.py && python3 tools/guard/arch_guard.py \
  && python3 -m pytest tools/guard/test_guard.py -q
```

---

## 7. Hồ sơ chi tiết (tham khảo thêm)

| Tài liệu | Nội dung |
|----------|----------|
| `docs/ARCHITECTURE_G1_TESTING.md` | kiến trúc G1 (3 lớp: hazard_core / scenarios / host_sim / record-replay) |
| `docs/logs/HOST_SIM_LOG.md` | log triển khai host_sim (step 3) |
| `docs/logs/TOOLS_RECORD_REPLAY_LOG.md` | log triển khai record/replay (step 4) |
| `docs/logs/TOOLS_SCENARIOS_LOG.md` | log triển khai scenarios + --scenario (step 2) |
| `docs/roadmaps/next-branch.state.md` | ledger trạng thái 16 steps |

> Ghi chú G1 chưa thao tác trên board thật: phần "record dữ liệu thật" (mục 5)
> cần board + token; mọi tool/schema/publish path còn lại đã verify không cần hardware.