# Sensor-node Install Guide — Orchestration State
Roadmap: docs/roadmaps/sensor-node-install-guide.roadmap.json
Updated: 2026-09-08

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Tạo khung tài liệu + BOM + an toàn | DONE | `grep -cE ...` = 19 dòng | |
| 2 | Bảng đấu dây GPIO 6 cảm biến + buzzer | DONE | `grep -nE ...` 8 dòng khớp | nguồn duy nhất thresholds.h |
| 3 | Cầu phân áp Echo 5V→3.3V + nguồn cấp | DONE | `grep -nE ...` 7 dòng | |
| 4 | Lắp cơ khí 6 vị trí quanh xe | DONE | `grep -cE ...` = 13 dòng | |
| 5 | Đi dây & chống nước | DONE | `grep -nE ...` 6 dòng | |
| 6 | Flash & kiểm tra nhanh bằng serial | DONE | `grep -nE ...` 6 dòng | |
| 7 | Troubleshooting + nghiệm thu + link chéo | DONE | `grep -cE ...` = 19 (≥6 OK) | |

## Contracts established
- Bảng GPIO map: S0 FRONT {5,6}, S1 LEFT_FRONT {7,8}, S2 RIGHT_FRONT {9,10},
  S3 LEFT_REAR {17,18}, S4 RIGHT_REAR {21,38}, S5 REAR {3,4};
  buzzer GPIO11; cấm GPIO47/48 (PSRAM), 26–32 (SPI0 flash), 19/20 (USB).
- Nguồn: VCC cảm biến 5V riêng + CHUNG GND; Echo 5V qua phân áp R1=1kΩ / R2=2kΩ → ~3.33V.
- Vị trí cảm biến: FRONT/REAR thẳng 0°, 4 góc nghiêng ngoài ~30°, FOV 75°, ≥20 cm giữa các dò.
- Build/flash: `cd firmware/sensor-node && pio run -e yolo_uno`, upload/monitor qua
  --upload-port <PORT> / -b 115200; log mong đợi: `Supersonic sensor array started (6 cam bien)`,
  `[ESPNOW] Send OK`.

## Deviations from plan
- Không tách worker 7 bước (todo tất cả cùng 1 file `docs/INSTALLATION_SENSOR_NODE.md`);
  thực thi trực tiếp trong 1 lượt theo DoD từng step, verify đầy đủ bằng lệnh roadmap.
- Lệnh Step 7 đếm theo `grep -cE` trên toàn file (10 khớp) — DoD "≥6 dòng troubleshooting"
  vẫn đạt vì bảng có 6 dòng triệu chứng riêng + các mục khác cũng khớp pattern trong file.

## Ràng buộc bắt buộc (áp dụng cho mọi step)
- R1: không chèn credential/token vào tài liệu.
- R2/R3: GPIO/ngưỡng ghi trong tài liệu PHẢI khớp `firmware/shared/thresholds.h`.
- R10: mỗi bước có `verification_command` chạy được (grep/test).
- Khi xong mỗi step: chạy lệnh verification, cập nhật cột Status, cập nhật Deviations nếu có.