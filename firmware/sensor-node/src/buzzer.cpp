#include "buzzer.h"

#include <Arduino.h>

#include "shared_state.h"
#include "task_cfg.h"

void buzzerInit()
{
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
}

static void buzzerToneOn()
{
    tone(BUZZER_PIN, BUZZER_TONE_HZ);
}

static void buzzerToneOff()
{
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
}

void buzzerTask(void *pvParameters)
{
    (void)pvParameters;
    buzzerInit();

    uint32_t lastBeepStartMs = millis();
    bool beeping   = false;
    bool continuous = false;

    for (;;)
    {
        float nearestCm = 0.0f;
        bool hasNearest = sharedStateGetNearest(nearestCm);

        uint32_t now = millis();

        bool isMuted = sharedStateGetMute();

        /* Tắt còi ngay lập tức nếu KHÔNG có cảm biến hợp lệ nào, hoặc bị MUTE từ màn hình. */
        if (!hasNearest || nearestCm <= 0.0f || isMuted)
        {
            if (beeping || continuous)
            {
                buzzerToneOff();
                continuous = false;
                beeping    = false;
            }
            vTaskDelay(pdMS_TO_TICKS(TASK_POLL_INTERVAL_MS));
            continue;
        }

        /* Phân loại zone dùng ngưỡng dùng chung (thresholds.h R3). */
        bool danger  = nearestCm <= (float)SENSOR_DANGER_CM;
        bool caution = !danger && nearestCm <= (float)SENSOR_CAUTION_CM;

        if (danger)
        {
            /* DANGER: kêu liên tục. */
            if (!continuous)
            {
                buzzerToneOn();
                continuous = true;
                beeping    = true;
                lastBeepStartMs = now;
            }
        }
        else if (caution)
        {
            /* CAUTION: bip ngắt quãng mỗi BUZZER_WARNING_PERIOD_MS. */
            if (continuous)
            {
                /* Vừa thoát khỏi DANGER, dừng kêu liên tục. */
                buzzerToneOff();
                continuous = false;
                beeping    = false;
            }
            if (!beeping && (now - lastBeepStartMs >= BUZZER_WARNING_PERIOD_MS))
            {
                buzzerToneOn();
                beeping = true;
                lastBeepStartMs = now;
            }
            else if (beeping && (now - lastBeepStartMs >= BUZZER_BEEP_ON_MS))
            {
                buzzerToneOff();
                beeping = false;
            }
        }
        else
        {
            /* SAFE: tắt còi. */
            if (continuous || beeping)
            {
                buzzerToneOff();
                continuous = false;
                beeping    = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_POLL_INTERVAL_MS));
    }
}
