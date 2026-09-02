---
description: Build firmware (sensor-node hoặc waveshare-screen) với PlatformIO.
agent: build
---

Build firmware. Tham số: tên firmware (`sensor-node` | `waveshare-screen`) và optional env
(`yolo_uno` mặc định, `yolo_uno_coreiot` cho bản CoreIoT).

```bash
cd firmware/<firmware>
pio run -e <env>
```

> Nếu PlatformIO chưa cài trên máy: hướng dẫn `pip install platformio`; build xác minh qua CI (B7)
> hoặc máy Windows `build_and_flash.bat`.

Báo kết quả; nếu lỗi tóm tắt nguyên nhân gọn. DoD bắt buộc tôn trọng R# (không thêm secret, không
đổi shared contract khi chưa có step roadmap).