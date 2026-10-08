
## sensor-node
- Thời lượng: 18.47 h, 1108 dòng SOAK, thiếu 1 nhịp (>90 s), mất cổng 0 lần, 0 dòng lỗi/sự kiện
- Reset: 0 (ngoài ý muốn 0); lý do: —
- Heap trống (KB): đầu 256.9, cuối 256.9, thấp nhất 253.8; min-ever thấp nhất 248.3 (sau 10 phút đầu 249.1 → cuối 248.3); block lớn nhất thấp nhất 244.0
- Độ dốc heap trống: +0.001 KB/h (1108 điểm, bỏ 10 phút đầu sau boot)
- Stack HWM thấp nhất (byte): hwm_buzz=1172, hwm_net=1732, hwm_sensor=1932
- MQTT reconnect: 0
## màn hình
- Thời lượng: 18.47 h, 1109 dòng SOAK, thiếu 0 nhịp (>90 s), mất cổng 0 lần, 11 dòng lỗi/sự kiện
- Reset: 0 (ngoài ý muốn 0); lý do: —
- Heap trống (KB): đầu 62.2, cuối 62.2, thấp nhất 59.2; min-ever thấp nhất 50.9 (sau 10 phút đầu 54.8 → cuối 50.9); block lớn nhất thấp nhất 25.0
- Độ dốc heap trống: -0.002 KB/h (1109 điểm, bỏ 10 phút đầu sau boot)
- PSRAM trống (KB): đầu 7281.4, cuối 7281.4, min-ever 7281.2
- Stack HWM thấp nhất (byte): hwm_lvgl=6156
- MQTT reconnect: 0
## ESP-NOW
- Gửi 664388 (lỗi 0), màn hình nhận 663056 → 99.80 % (xấp xỉ: 2 board lệch cửa sổ ≤ 60 s)
- Mất link: 0 lần; khoảng hở lớn nhất 317 ms; 0 cửa sổ có khoảng hở > 1500 ms
## Tiêu chí đề xuất (docs/SOAK_TEST.md)
- 0 reset ngoài ý muốn: sensor-node ĐẠT / màn hình ĐẠT
- Không lần mất link nào > 1500 ms: ĐẠT
- Min heap đi ngang: xem 'sau 10 phút đầu → cuối' và độ dốc ở trên (người đọc tự kết luận)
