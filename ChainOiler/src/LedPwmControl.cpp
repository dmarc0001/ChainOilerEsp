#include "LedPwmControl.hpp"
#include <esp_log.h>
#include <esp_err.h>

namespace esp32s2
{

  LedPwmControl::LedPwmControl()
  {
  }

  void LedPwmControl::init()
  {
    ESP_LOGD(tag, "pwm led init...");

    ESP_LOGD(tag, "init timer...");
    // timer config
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,   // timer mode
        .duty_resolution = LEDC_TIMER_9_BIT, // resolution of PWM duty
        .timer_num = LEDC_LS_TIMER,          // timer index
        .freq_hz = 5000,                     // frequency of PWM signal
        .clk_cfg = LEDC_AUTO_CLK,            // Auto select the source clock
    };
    //
    // timer für low speed (5 khz) setzten
    //
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ESP_LOGD(tag, "init timer...done");

    //
    // LED Kanäle initialisieren
    //
    ESP_LOGD(tag, "init pwm channels...");
    ledc_channel[LED_CONTROL_CHANNEL] =
        {
            .gpio_num = Prefs::LED_CONTROL,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = PWM_TIMER_CHANNEL_CONTROL,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_LS_TIMER,
            .duty = 0,
            .hpoint = 0,
        };

    ledc_channel[LED_RAIN_CHANNEL] =
        {
            .gpio_num = Prefs::LED_RAIN,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = PWM_TIMER_CHANNEL_RAIN,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_LS_TIMER,
            .duty = 0,
            .hpoint = 0,
        };

    ledc_channel[LED_PUMP_CHANNEL] =
        {
            .gpio_num = Prefs::LED_PUMP,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = PWM_TIMER_CHANNEL_PUMP,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_LS_TIMER,
            .duty = 0,
            .hpoint = 0,
        };

    ledc_channel[LED_REED_CHANNEL] =
        {
            .gpio_num = Prefs::LED_REED_CONTROL,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = PWM_TIMER_CHANNEL_REED,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_LS_TIMER,
            .duty = 0,
            .hpoint = 0,
        };

    // Set LED Controller with previously prepared configuration
    for (auto ch = 0; ch < LED_COUNT; ++ch)
    {
      ESP_LOGD(tag, "init channel %02d...", ch);
      ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[ch]));
      changed[ch] = true;
    }
    ESP_LOGD(tag, "init pwm channels...done");

    // install fading
    ESP_LOGD(tag, "enable fading...");
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
    ESP_LOGD(tag, "enable fading...done");
    ESP_LOGD(tag, "pwm led init...OK");
  }

  void LedPwmControl::allOff()
  {
    for (auto ch = 0; ch < LED_COUNT; ++ch)
    {
      ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0);
      changed[ch] = false;
    }
  }

  void LedPwmControl::setRainLED(bool _set)
  {
    if (_set)
      ledc_set_duty(ledc_channel[LED_RAIN_CHANNEL].speed_mode, ledc_channel[LED_RAIN_CHANNEL].channel, LED_MAX);
    else
      ledc_set_duty(ledc_channel[LED_RAIN_CHANNEL].speed_mode, ledc_channel[LED_RAIN_CHANNEL].channel, LED_MIN);
    changed[LED_RAIN_CHANNEL] = true;
  }

  void LedPwmControl::setControlLED(bool _set)
  {
    if (_set)
      ledc_set_duty(ledc_channel[LED_CONTROL_CHANNEL].speed_mode, ledc_channel[LED_CONTROL_CHANNEL].channel, LED_MAX);
    else
      ledc_set_duty(ledc_channel[LED_CONTROL_CHANNEL].speed_mode, ledc_channel[LED_CONTROL_CHANNEL].channel, LED_MIN);
    changed[LED_CONTROL_CHANNEL] = true;
  }

  void LedPwmControl::setControlCrossLED(bool _set)
  {
    if (_set)
    {
      ledc_set_duty(ledc_channel[LED_CONTROL_CHANNEL].speed_mode, ledc_channel[LED_CONTROL_CHANNEL].channel, LED_MAX);
      ledc_set_duty(ledc_channel[LED_RAIN_CHANNEL].speed_mode, ledc_channel[LED_RAIN_CHANNEL].channel, LED_MAX);
    }
    else
    {
      ledc_set_duty(ledc_channel[LED_CONTROL_CHANNEL].speed_mode, ledc_channel[LED_CONTROL_CHANNEL].channel, LED_DARK);
      ledc_set_duty(ledc_channel[LED_RAIN_CHANNEL].speed_mode, ledc_channel[LED_RAIN_CHANNEL].channel, LED_DARK);
    }
    changed[LED_CONTROL_CHANNEL] = true;
    changed[LED_RAIN_CHANNEL] = true;
  }

  void LedPwmControl::setPumpLED(bool _set)
  {
    if (_set)
      ledc_set_duty(ledc_channel[LED_PUMP_CHANNEL].speed_mode, ledc_channel[LED_PUMP_CHANNEL].channel, LED_MAX);
    else
      ledc_set_duty(ledc_channel[LED_PUMP_CHANNEL].speed_mode, ledc_channel[LED_PUMP_CHANNEL].channel, LED_MIN);
    changed[LED_PUMP_CHANNEL] = true;
  }

  bool LedPwmControl::fadeOutPumpLED()
  {
    changed[LED_PUMP_CHANNEL] = false;
    ledc_set_fade_with_time(ledc_channel[LED_PUMP_CHANNEL].speed_mode, ledc_channel[LED_PUMP_CHANNEL].channel, LED_MIN, FADE_TIME);
    ledc_fade_start(ledc_channel[LED_PUMP_CHANNEL].speed_mode, ledc_channel[LED_PUMP_CHANNEL].channel, LEDC_FADE_NO_WAIT);
    return true;
  }

  void LedPwmControl::setAttentionLED(bool _set)
  {
    if (_set)
    {
      ledc_set_duty(ledc_channel[LED_CONTROL_CHANNEL].speed_mode, ledc_channel[LED_CONTROL_CHANNEL].channel, LED_MIN);
      ledc_set_duty(ledc_channel[LED_RAIN_CHANNEL].speed_mode, ledc_channel[LED_RAIN_CHANNEL].channel, LED_MAX);
      ledc_set_duty(ledc_channel[LED_PUMP_CHANNEL].speed_mode, ledc_channel[LED_PUMP_CHANNEL].channel, LED_MIN);
    }
    else
    {
      ledc_set_duty(ledc_channel[LED_CONTROL_CHANNEL].speed_mode, ledc_channel[LED_CONTROL_CHANNEL].channel, LED_MAX);
      ledc_set_duty(ledc_channel[LED_RAIN_CHANNEL].speed_mode, ledc_channel[LED_RAIN_CHANNEL].channel, LED_MIN);
      ledc_set_duty(ledc_channel[LED_PUMP_CHANNEL].speed_mode, ledc_channel[LED_PUMP_CHANNEL].channel, LED_MAX);
    }
    changed[LED_CONTROL_CHANNEL] = true;
    changed[LED_RAIN_CHANNEL] = true;
    changed[LED_PUMP_CHANNEL] = true;
  }

  void LedPwmControl::setAPModeLED(bool _set)
  {
    if (_set)
    {
      ledc_set_duty(ledc_channel[LED_RAIN_CHANNEL].speed_mode, ledc_channel[LED_RAIN_CHANNEL].channel, LED_MAX);
      ledc_set_duty(ledc_channel[LED_CONTROL_CHANNEL].speed_mode, ledc_channel[LED_CONTROL_CHANNEL].channel, LED_MAX);
      ledc_set_duty(ledc_channel[LED_PUMP_CHANNEL].speed_mode, ledc_channel[LED_PUMP_CHANNEL].channel, LED_MAX);
    }
    else
    {
      ledc_set_duty(ledc_channel[LED_RAIN_CHANNEL].speed_mode, ledc_channel[LED_RAIN_CHANNEL].channel, LED_DARK);
      ledc_set_duty(ledc_channel[LED_CONTROL_CHANNEL].speed_mode, ledc_channel[LED_CONTROL_CHANNEL].channel, LED_DARK);
      ledc_set_duty(ledc_channel[LED_PUMP_CHANNEL].speed_mode, ledc_channel[LED_PUMP_CHANNEL].channel, LED_DARK);
    }
    changed[LED_CONTROL_CHANNEL] = true;
    changed[LED_RAIN_CHANNEL] = true;
    changed[LED_PUMP_CHANNEL] = true;
  }

  void LedPwmControl::makeChange()
  {
    for (auto ch = 0; ch < LED_COUNT; ++ch)
    {
      if (changed[ch])
      {
        ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
      }
      changed[ch] = false;
    }
  }
}
