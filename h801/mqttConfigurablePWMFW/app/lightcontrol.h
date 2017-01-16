#ifndef LIGHTCONTROL_H
#define LIGHTCONTROL_H

#define MAX_ALLOWED_EFFECT_DURATION 60000
#define MIN_ALLOWED_EFFECT_DURATION 100
#define MAX_ALLOWED_EFFECT_REPETITIONS 10

#define DEFAULT_EFFECT_REPETITIONS 1
#define DEFAULT_EFFECT_DURATION 3200

enum FLASHFLAGS {FLASH_INTERMED_USERSET, FLASH_INTERMED_DARK, FLASH_INTERMED_ORIG};

const uint32_t pwm_period = 5000; // * 200ns ^= 1 kHz

extern uint32_t effect_target_values_[PWM_CHANNELS];
extern uint32_t effect_intermid_values_[PWM_CHANNELS];
extern String mqtt_forward_to_;
extern String mqtt_payload_;

void applyValues(uint32_t values[PWM_CHANNELS]);
void startFlash(uint8_t repetitions, FLASHFLAGS intermed);
void flashSingleChannel(uint8_t repetitions, uint8_t channel);
void startFade(uint32_t duration_ms);

#endif