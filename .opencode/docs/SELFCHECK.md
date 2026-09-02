# SELFCHECK — Kiểm tra trước khi báo DONE

> Trả lời 10 câu trước khi tuyên bố một task hoàn thành. Nếu **bất kỳ** câu nào là "không"/"ngoại lệ",
> task chưa DONE.

1. **Secret**: không có literal token/password/API key nào trong file tôi sẽ commit?
   (`python3 tools/guard/scan_secrets.py` exit 0)
2. **Shared contract**: tôi không định nghĩa lại symbol/struct nằm ở `firmware/shared/`?
3. **Thresholds**: ngưỡng nào tôi đụng tới đều sửa tại `firmware/shared/thresholds.h`, không nơi khác?
4. **SENSOR_COUNT**: không gõ tay; suy từ `sizeof(SENSOR_PINS)` + `static_assert`?
5. **Demo/dead code**: không thêm demo vào production; module thêm vào được build trong ≥1 env?
6. **Kích thước file**: mọi file tôi sửa/tạo ≤ 400 dòng?
7. **Git hygiene**: `sdkconfig`/`keys.json`/binary không bị stage; `dependencies.lock` được track?
8. **Git workflow**: chỉ remote `origin`; stage từng file cụ thể (không `add -A`); message conventional?
9. **DoD có lệnh xác minh**: tôi có thể paste lệnh + kết quả chạy (không phải "nhìn là chạy")?
10. **Log**: tôi đã tạo/cập nhật `docs/logs/<COMPONENT>_<TASK>_LOG.md` (mục tiêu, file sửa, kết quả,
    hướng dẫn demo)?

## Nếu vi phạm R#
- Đọc lại mục tương ứng trong `CONSTITUTION.md`; sửa ngay; không "sửa sau ở commit khác".
- Ghi vi phạm đã từng xảy ra vào `docs/roadmaps/<slug>.state.md` (Deviations).