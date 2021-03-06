#include <SmingCore/SmingCore.h>
#include <defaultlightconfig.h>
#include <pwmchannels.h>
#include <spiffsconfig.h>
#include "lightcontrol.h"

//these get set from outside and should not be read or written in timer functions
uint32_t effect_target_values_[PWM_CHANNELS]={0,0,0,0,0};
uint32_t effect_intermid_values_[PWM_CHANNELS]={0,0,0,0,0};
uint32_t button_on_values_[PWM_CHANNELS]={0,0,0,0,0};

//these get set from outside and are only used at the end (TODO)
String mqtt_forward_to_;
String mqtt_payload_;


//internal types
enum effect_id_type {EFF_NO, EFF_FLASH, EFF_FADE};

//internal values
Timer flashTimer;
effect_id_type effect_ = EFF_NO;
uint32_t apply_last_values_[PWM_CHANNELS];
int32_t active_values_[PWM_CHANNELS];
int32_t steps_left_=0;
int32_t fade_diff_values_[PWM_CHANNELS];

//externally imported stuff
extern MqttClient *mqtt;

////////////////////
//// PWM Stuff ////
///////////////////

//init PWM and restore stored pwm values
void setupPWM()
{
	// PWM setup
	uint32 io_info[PWM_CHANNELS][3] = {
		// MUX, FUNC, PIN
		{PERIPHS_IO_MUX_MTDO_U,  FUNC_GPIO15, 15}, //R-
		{PERIPHS_IO_MUX_MTCK_U,  FUNC_GPIO13, 13}, //G-
		{PERIPHS_IO_MUX_MTDI_U,  FUNC_GPIO12, 12}, //B-
		{PERIPHS_IO_MUX_MTMS_U,  FUNC_GPIO14, 14}, //W1-
		{PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4 ,  4}, //W2-
	};

	DefaultLightConfig.load(effect_target_values_); //load initial default values
	pwm_init(pwm_period, effect_target_values_, PWM_CHANNELS, io_info);
	pwm_start();
	// GPIO / FAN Setup
	pinMode(FAN_GPIO, OUTPUT);
	// Check Fan
	checkFanNeeded();
}

///////////////////////////////////////
///// Related Hardware Stuff
///////////////////////////////////////

void enableFan(bool en)
{
	digitalWrite(FAN_GPIO,(en)? HIGH:LOW);
}

void checkFanNeeded()
{
	uint32_t pwm_sum = 0;
	for (uint8_t i=0;i<PWM_CHANNELS;i++)
	{
		pwm_sum += pwm_get_duty(i);
	}
	enableFan(pwm_sum >= NetConfig.fan_threshold);
}

/////////////////////
//// Light Stuff ////
/////////////////////

const int8_t FADE_CALC_BASE2_EXPONENT_ = 15;
const uint32_t FADE_PERIOD_ = 40; //ms == 25fps

void saveCurrentValues()
{
	for (uint8_t i=0;i<PWM_CHANNELS;i++)
		apply_last_values_[i] = pwm_get_duty(i);
}

void applyValues(int32_t values[PWM_CHANNELS])
{
	for (uint8_t i=0;i<PWM_CHANNELS;i++)
	{
		pwm_set_duty(static_cast<uint32_t>(values[i]),i);
	}
	pwm_start();
	checkFanNeeded();
}

void applyValues(uint32_t values[PWM_CHANNELS])
{
	for (uint8_t i=0;i<PWM_CHANNELS;i++)
	{
		pwm_set_duty(values[i],i);
	}
	pwm_start();
	checkFanNeeded();
}

void stopAndRestoreValues(bool abort)
{
	flashTimer.stop();
	steps_left_ = 0;
	applyValues(apply_last_values_);
	effect_ = EFF_NO;
	if (false == abort && 0 != mqtt && mqtt_forward_to_.length() > 0)
		mqtt->publish(mqtt_forward_to_, mqtt_payload_, false);
	mqtt_forward_to_="";
	mqtt_payload_="";
}

void timerFuncShowFlashEffect()
{
	if (steps_left_ > 0)
	{
		if (steps_left_ % 2 == 0)
		{
			applyValues(active_values_);
		} else {
			applyValues(effect_intermid_values_);
		}
		steps_left_--;
	} else {
		stopAndRestoreValues();
	}
}

void timerFuncShowFadeEffect()
{
	if (steps_left_ > 0)
	{
		for (uint8_t i=0; i<PWM_CHANNELS; i++)
		{
			active_values_[i] = active_values_[i] + fade_diff_values_[i]; //calc in FADE_CALC_BASE2_EXPONENT_
			pwm_set_duty(static_cast<uint32_t>(active_values_[i] >> FADE_CALC_BASE2_EXPONENT_),i); //set in normal
		}
		steps_left_--;
		pwm_start();
	} else {
		stopAndRestoreValues();
	}
}

void timerFuncShowEffect()
{
	switch (effect_)
	{
		default:
		case EFF_NO:
			flashTimer.stop();
			break;
		case EFF_FLASH:
			timerFuncShowFlashEffect();
			break;
		case EFF_FADE:
			timerFuncShowFadeEffect();
			break;
	}
}


void startFlash(uint8_t repetitions=DEFAULT_EFFECT_REPETITIONS, FLASHFLAGS intermed=FLASH_INTERMED_USERSET, uint32_t flash_period=DEFAULT_FLASH_PERIOD_)
{
	if (repetitions > MAX_ALLOWED_EFFECT_REPETITIONS)
		return;
	if (flash_period < MIN_ALLOWED_EFFECT_PERIOD || flash_period > MAX_ALLOWED_EFFECT_PERIOD)
		return;
	if (effect_ != EFF_NO)
		stopAndRestoreValues(true);
	saveCurrentValues();
	memcpy(active_values_, effect_target_values_, PWM_CHANNELS*sizeof(uint32_t));
	switch (intermed)
	{
		case FLASH_INTERMED_DARK:
		memset(effect_intermid_values_, 0, PWM_CHANNELS*sizeof(uint32_t));
		break;
		case FLASH_INTERMED_ORIG:
		memcpy(effect_intermid_values_, apply_last_values_, PWM_CHANNELS*sizeof(uint32_t));
		break;
	}
	applyValues(effect_intermid_values_);
	steps_left_ = repetitions * 2;
	effect_ = EFF_FLASH;
	flashTimer.initializeMs(flash_period, timerFuncShowFlashEffect).start();
}

void flashSingleChannel(uint8_t repetitions, uint8_t channel)
{
	if (channel >= PWM_CHANNELS || repetitions > MAX_ALLOWED_EFFECT_REPETITIONS || repetitions == 0)
		return;
	if (effect_ != EFF_NO)
		stopAndRestoreValues(true);
	saveCurrentValues();
	memcpy(effect_target_values_, apply_last_values_, PWM_CHANNELS*sizeof(uint32_t));
	memcpy(effect_intermid_values_, apply_last_values_, PWM_CHANNELS*sizeof(uint32_t));
	effect_target_values_[channel] = pwm_period/5;
	effect_intermid_values_[channel] = 0;
	startFlash(repetitions);
}

void startFade(uint32_t duration_ms=DEFAULT_EFFECT_DURATION)
{
	if (duration_ms > MAX_ALLOWED_EFFECT_DURATION || duration_ms < MIN_ALLOWED_EFFECT_DURATION)
		return;
	if (effect_ != EFF_NO)
		stopAndRestoreValues(true);
	saveCurrentValues();

	steps_left_ = duration_ms / FADE_PERIOD_ + ((duration_ms % FADE_PERIOD_)? 1 : 0);
	//at 25fps: maximum possible value of steps_left: MAX_ALLOWED_EFFECT_DURATION/FADE_PERIOD = 60000/40 = 1500
	//          --> ld(1500)=11bit --> set FADE_CALC_BASE2_EXPONENT_:= 15bit since 11+15 < 31bit

	for (uint8_t i=0; i<PWM_CHANNELS; i++)
	{
		fade_diff_values_[i] = ((static_cast<int32_t>(effect_target_values_[i]) - static_cast<int32_t>(apply_last_values_[i])) << FADE_CALC_BASE2_EXPONENT_) / static_cast<int32_t>(steps_left_);
		active_values_[i] = static_cast<int32_t>(apply_last_values_[i]) << FADE_CALC_BASE2_EXPONENT_;
	}

	//copy effect_target_values_ to apply_last_values_ so effect_target_values_ get applied on stop or interrupt of effect
	memcpy(apply_last_values_, effect_target_values_, PWM_CHANNELS*sizeof(uint32_t));
	effect_ = EFF_FADE;
	flashTimer.initializeMs(FADE_PERIOD_, timerFuncShowFadeEffect).start();
}
