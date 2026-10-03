# SENSOR_NODE_RELEASE_CONFIRM_LOG — "nhanh vào – chậm ra" cho bộ lọc khoảng cách

Ngày: 2026-10-03  
Nhánh: `khoa`  
Roadmap: `docs/roadmaps/sensor-release-confirm.roadmap.json` (4 bước, ledger `.state.md`)

## 1. Vấn đề
Người dùng thấy cảm biến "chưa chính xác như trước". Điều tra bằng log serial thật (COM7, chỉ đọc, giữ DTR/RTS thấp để không reset board),
đọc code/lịch sử git và mô phỏng bộ lọc trên PC.

## 2. Nguyên nhân (đã chứng minh một phần, xem mục 6)
- **Log thật (25 s, chỉ cảm biến S3 = LEFT_REAR cắm dây)**: 253 mẫu, **không có lần `REJECT` nào của S3** (mọi mẫu đều hợp lệ), nhưng đầu ra nhảy giữa
  22,6 / 190 / 247 / 498 cm: 48% thời gian báo "xa" (> 100 cm), 37 lần nhảy ≥ 30 cm (88 lần/phút). Các giá trị lặp lại đúng số → echo phản xạ ảo, không phải vật di chuyển.
- **Code**: `firmware/shared/thresholds.h` không đổi tham số lọc nào từ 16/9. Thay đổi duy nhất là 21/9 (`11c306d`, `dc50fef`, `6c9b306`): khối fast-track trong
  `distance_filter.cpp` cho phép **chỉ 1 mẫu** xa hơn ≥ 30 cm là đầu ra nhảy theo ngay và xoá lịch sử → đầu ra đi theo từng echo ảo xa trong khi vật vẫn ở gần.
- **Mô phỏng** (biên dịch chính `distance_filter.cpp` của 3 phiên bản trên PC, chuỗi mẫu giả lập có echo ảo 190/247/498 cm lấy từ log thật):

| Bộ lọc | Báo "xa" sai, echo ảo lẻ tẻ 20% | Báo "xa" sai, echo ảo thành đợt (48% xa) | Trễ báo động | Trễ nhả |
|---|---|---|---|---|
| Trước 21/9 (cluster) | 0% | 7–18% | 0,5 s | 0,6 s |
| Bản 21/9 (nhả 1 mẫu) | 35% | 57–72% | 0,1 s | 0,1 s |
| **Bản này (nhả sau 5 mẫu xa)** | **0,1%** | 9–27% | 0,1 s | 0,5 s |

## 3. Thay đổi
- `firmware/shared/thresholds.h`: `FILTER_RELEASE_CONFIRM_SAMPLES 5` (R3) + `TBS_STATIC_ASSERT` giá trị trong [2, `FILTER_HISTORY_SIZE`]
  (đã thử đặt 1 và 10: build bị chặn).
- `firmware/sensor-node/src/distance_filter.cpp`: chiều RA chỉ nhả khi `FILTER_RELEASE_CONFIRM_SAMPLES` mẫu mới nhất liên tiếp đều xa hơn kết quả hiện tại
  ≥ `FILTER_MIN_JUMP_THRESHOLD_CM`; giá trị nhả = **mẫu gần nhất** trong số đó. Chiều VÀO (2 mẫu sát nhau) không đổi.
- `firmware/sensor-node/include/distance_filter.h`: ghi thêm status `FAST_TRACK_CROSSING` vào chú thích.
- `firmware/sensor-node/test/test_distance_filter.cpp`, `test_runner.cpp`: sửa `test_filter_fast_track_crossing`; thêm 3 test
  (`..._release_ignores_isolated_far_echoes`, `..._release_after_confirmed_far_samples`, `..._near_target_with_ghost_echoes_stays_near`).

## 4. Kiểm thử
- Viết test trước: trước khi sửa lọc **đúng 4 test đỏ** (đầu ra nhảy sang 300 / 190 / 320 cm sau 1 mẫu xa). Sau khi sửa: `pio test -e native` **19/19 pass**.
- Test dùng chuỗi giả-ngẫu-nhiên cố định (LCG) tình cờ có 1 đợt **5 echo ảo liên tiếp** (nhả hợp lệ theo thiết kế, ~4 mẫu "xa" trước khi quay lại), nên ngưỡng là ≤ 10 mẫu xa
  trên 285 (bản lỗi cho ~100).
- Bộ lọc trong repo cho đúng số liệu mô phỏng của phương án "nhả sau 5 mẫu" (đối chiếu từng ô).
- Build: sensor-node `yolo_uno` SUCCESS (flash 696.809 B), `yolo_uno_coreiot` SUCCESS (718.761 B), waveshare-screen `yolo_uno` SUCCESS (1.311.361 B, 31,8%; `thresholds.h` dùng chung nên build lại).
- Guard: `arch_guard` OK, `scan_secrets` OK, `pytest tools/guard/test_guard.py` 29 passed (Windows cần `PYTHONUTF8=1`).

## 5. Nạp thiết bị và đo lại
- Nạp sensor-node **env `yolo_uno_coreiot`** (giữ nguyên chế độ đang chạy, không đổi Wi-Fi) qua COM7: `Hash of data verified`.
  Màn hình **không nạp lại** (chỉ thêm macro vào `thresholds.h`, không đổi hành vi).
- Log 25 s sau khi nạp: S3 đọc ổn định ~170–190 cm, 252 mẫu, 0 lần nhảy. **Không so sánh được với log trước** vì cảnh đã đổi: lúc trước có vật ở 22,6 cm,
  lần này không còn vật gần (chỉ còn tường ~190 cm), nên con số "0 nhảy" không chứng minh bản sửa.
- Gợi ý (chưa kết luận): khi không có vật gần, cùng cảm biến không hề trả 247/498 cm → echo ảo có vẻ gắn với vật ở sát vùng mù (~22,6 cm).
- ESP-NOW vẫn **chưa nối** (159 lần `send fail` trong 25 s): sensor-node đang quét Wi-Fi mọi kênh vì không vào được AP — vấn đề riêng, chưa xử lý.

## 6. Hạn chế và việc cần làm
- [ ] **Đo lại đúng cảnh cũ**: đặt vật ở ~23 cm như trước và thêm 40 / 60 / 100 cm, bắt log ~25 s mỗi vị trí
      (`pio device monitor -p COM7 -b 115200`, đọc cột slot 3 của dòng `DIST:`). Kỳ vọng: tỷ lệ báo "xa" và số lần nhảy giảm mạnh so với 48% / 88 lần/phút.
- Mô phỏng dùng mô hình echo ảo tổng hợp; firmware chỉ in giá trị đã lọc nên **chưa có mẫu thô thật**. Cần thêm cờ in mẫu thô (chưa làm) để cân chỉnh N bằng dữ liệu thật (T1.4/T5.9).
- Đợt echo ảo ≥ 5 mẫu liên tiếp vẫn bị coi là nhả; quay lại gần cần 2 mẫu gần liên tiếp. Với echo ảo thành đợt dài, 9–27% thời gian vẫn báo "xa" sai — cần xử lý ở phần cứng/điều kiện đo.
- Trễ nhả cảnh báo tăng từ 0,1 s lên 0,5 s (chiều VÀO không đổi). Đổi bằng `FILTER_RELEASE_CONFIRM_SAMPLES`.
- Phát hiện phụ chưa sửa: vật gần hơn `MIN_DISTANCE_CM` (15 cm) bị coi là mẫu lỗi, sau 2 lần liên tiếp thành `DISCONNECTED` (màn hình hiện "không dữ liệu" thay vì DANGER).
- Giả thuyết chưa kiểm chứng: vật sát vùng mù JSN-SR04T; Wi-Fi quét kênh/sụt nguồn gây thêm echo ảo.
