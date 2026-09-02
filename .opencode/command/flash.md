---
description: Flash + monitor firmware lên board qua cổng serial.
agent: build
---

Flash ESP32-S3 và mở serial monitor. Tham số: `<firmware> [port]` — mặc định `sensor-node` dùng
`/dev/ttyACM0`, `waveshare-screen` dùng `/dev/ttyACM1`.

```bash
cd firmware/<firmware>
pio run -e yolo_uno -t upload --upload-port <port>
pio device monitor -p <port> -b 115200
```

> Thay đổi firmware-specific (pin/task/MQTT shape) phải có **DoD flash-and-observe**: flash, quan sát
> serial log đạt trạng thái mong đợi trong thời gian xác định (ví dụ: `warn_state=1` trong 2 s khi
> distance < 50 cm). Trên Windows dùng `build_and_flash.bat flash <COM>`.