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
    bool beeping = false;
    bool continuous = false;

    for (;;)
    {
        float nearestCm = 0.0f;
        bool hasNearest = sharedStateGetNearest(nearestCm);

        bool danger = hasNearest && nearestCm > 0.0f && nearestCm <= SENSOR_DANGER_CM;
        bool caution = hasNearest && nearestCm > 0.0f && nearestCm <= SENSOR_CAUTION_CM;

        uint32_t now = millis();

        if (danger)
        {
            if (!continuous)
            {
                buzzerToneOn();
                continuous = true;
                beeping = true;
                lastBeepStartMs = now;
            }
        }
        else
        {
            if (continuous)
            {
                buzzerToneOff();
                continuous = false;
                beeping = false;
            }
            if (caution)
            {
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
            else if (beeping)
            {
                buzzerToneOff();
                beeping = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_POLL_INTERVAL_MS));
    }
}
