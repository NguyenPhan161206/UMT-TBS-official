#include "buzzer.h"

#include <Arduino.h>

#include "shared_state.h"

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

    for (;;)
    {
        float nearestCm = 0.0f;
        bool hasNearest = sharedStateGetNearest(nearestCm);

        uint32_t period = 0;
        if (hasNearest && nearestCm > 0.0f)
        {
            if (nearestCm < BUZZER_DANGER_DISTANCE_CM)
            {
                period = BUZZER_DANGER_PERIOD_MS;
            }
            else if (nearestCm <= BUZZER_WARNING_DISTANCE_CM)
            {
                period = BUZZER_WARNING_PERIOD_MS;
            }
        }

        uint32_t now = millis();

        if (period == 0)
        {
            if (beeping)
            {
                buzzerToneOff();
                beeping = false;
            }
        }
        else if (!beeping && (now - lastBeepStartMs >= period))
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

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
