# Hướng dẫn kiểm tra & sử dụng Giai đoạn 1 (G1 — Mở khả năng kiểm thử)

> Trạng thái: **G1 4/4 bước DONE, đã merge `main`** — hiện `nguyen` = `main` = `01262ce`.
> Bổ sung fix boundary `01262ce`: **`x <= 30cm` → DANGER** ở firmware C (khớp cloud
> rule-chain `dist <= 30.0` và python mirror) — xem **mục 3.2** để test bằng mắt.
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

## 3. Test thủ công UI trên PC (host_sim — KHÔNG cần board)

Executable: `/tmp/host_sim/umt_dash_sim` — chạy **ĐÚNG code UI firmware**
(`ui_dashboard*`/`ui_dashboard_layout.c`) qua LVGL v9.1 + SDL2, stub thay phần cứng.
Cửa sổ 800×480 hiển thị giống hệt màn hình 7" waveshare — đây là cách xem UI nhanh
nhất ngay trên máy tính, không cần board/flash.

> Nếu đang dùng Wayland mà cửa sổ không hiện, thêm `SDL_VIDEODRIVER=x11 ` đầu lệnh
> (XWayland). Máy có GUI (`DISPLAY=:0`) thì cửa sổ mở bình thường.

### 3.1 Xem UI + kịch bản va chạm (từng bước)
```bash
# a. "Dừng gấp" — xem chuyển SAFE → CAUTION → DANGER (1 giây/mốc):
/tmp/host_sim/umt_dash_sim --scenario slam --interval 1000 --exit-after 30

# b. Vật lao tới từ xa — FRONT 160→20cm:
/tmp/host_sim/umt_dash_sim --scenario approach --interval 800 --exit-after 25

# c. Xe cắt ngang — side slot nhảy >40cm:
/tmp/host_sim/umt_dash_sim --scenario crossing --interval 800 --exit-after 25

# d. Không có vật — toàn SAFE:
/tmp/host_sim/umt_dash_sim --scenario normal --interval 800 --exit-after 20
```
Quan sát: D1/OVERALL đổi màu theo zone (xanh = SAFE, cam = CAUTION, đỏ = DANGER
+ cảnh báo nhấp nháy). `--interval <ms>` = tốc độ feed mốc (lớn = xem chậm);
`--exit-after <giây>` = tự đóng; đóng sớm hơn thì `Ctrl+C`.

### 3.2 Test boundary `x <= 30cm → DANGER` (fix commit `01262ce`)

Tạo fixture replay đi qua đúng mốc 30cm (có đủ schema V2 — dùng được cả với
`replay_telemetry.py --dry-run` lẫn `umt_dash_sim --replay`):
```bash
printf '%s\n' \
'{"d1":40.0,"d2":40.0,"d3":40.0,"d4":40.0,"d5":40.0,"d6":40.0,"nearest_cm":40.0,"has_nearest":true}' \
'{"d1":31.0,"d2":31.0,"d3":31.0,"d4":31.0,"d5":31.0,"d6":31.0,"nearest_cm":31.0,"has_nearest":true}' \
'{"d1":30.0,"d2":30.0,"d3":30.0,"d4":30.0,"d5":30.0,"d6":30.0,"nearest_cm":30.0,"has_nearest":true}' \
'{"d1":29.0,"d2":29.0,"d3":29.0,"d4":29.0,"d5":29.0,"d6":29.0,"nearest_cm":29.0,"has_nearest":true}' \
'{"d1":150.0,"d2":150.0,"d3":150.0,"d4":150.0,"d5":150.0,"d6":150.0,"nearest_cm":150.0,"has_nearest":true}' \
> /tmp/boundary_demo.jsonl

# Bước A — validate schema (không cần board):
python3 tools/replay_telemetry.py --in /tmp/boundary_demo.jsonl --dry-run

# Bước B — xem UI trên màn hình:
/tmp/host_sim/umt_dash_sim --replay /tmp/boundary_demo.jsonl --interval 2000 --exit-after 20
```
| Mốc | Zone hiển thị | Ghi chú |
|-----|---------------|---------|
| 40cm | CAUTION (cam) | |
| 31cm | CAUTION (cam) | |
| **30cm** | **DANGER (đỏ)** | fix `01262ce`: trước fix hiện CAUTION — nay ĐỎ |
| 29cm | DANGER (đỏ) | |
| 150cm | SAFE (xanh) | |

### 3.3 Headless (không cần display / CI)
```bash
SDL_VIDEODRIVER=dummy /tmp/host_sim/umt_dash_sim --scenario slam --exit-after 3
# DoD CI: xvfb-run -a /tmp/host_sim/umt_dash_sim --exit-after 3 --scenario approach
```

### 3.4 Các kịch bản có sẵn (nguồn: tools/scenarios.py)
| Scenario  | Ý nghĩa                          |
|-----------|----------------------------------|
| `approach`| vật tiến gần phía FRONT (160→20cm) |
| `crossing`| xe cắt ngang phía trước           |
| `slam`    | dừng gấp (110→20cm trong ≤3 mốc)  |
| `normal`  | không có cảnh báo (>100cm)        |

### 3.5 Chạy lại dữ liệu đã ghi thật (JSONL)
```bash
/tmp/host_sim/umt_dash_sim --replay /tmp/tb.jsonl --exit-after 5
```

Cờ hữu ích: `--scenario <name>`, `--replay <file.jsonl>`, `--exit-after <giây>`,
`--interval <ms>` (tốc độ feed mốc), `--help`.

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

# (3b) Boundary mirror: 30.0 -> DANGER, 30.1 -> CAUTION (fix 01262ce)
python3 tools/test_mqtt_coreiot.py --dry-run --distance 30.0
python3 tools/test_mqtt_coreiot.py --dry-run --distance 30.1

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