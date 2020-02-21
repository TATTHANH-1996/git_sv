#ifndef __USER_MOTOR_H__
#define __USER_MOTOR_H__

#include "esp_common.h"
#include "user_config.h"

#include "key.h"
#include "pwm.h"

#define PWM_CHANNEL 2
#define MOTOR_CAP   0
#define MOTOR_PHOI  1

#define PWM_0_OUT_IO_MUX   PERIPHS_IO_MUX_MTDI_U
#define PWM_0_OUT_IO_NUM   12
#define PWM_0_OUT_IO_FUNC  FUNC_GPIO12

#define PWM_1_OUT_IO_MUX   PERIPHS_IO_MUX_MTMS_U
#define PWM_1_OUT_IO_NUM   14
#define PWM_1_OUT_IO_FUNC  FUNC_GPIO14

struct motor_saved_param {
	uint32 pwm_period;
	uint32 pwm_duty[PWM_CHANNEL];
};

void ICACHE_FLASH_ATTR user_motor_init(void);
uint32 user_motor_get_duty(uint8 channel);
void user_motor_set_duty(uint32 duty, uint8 channel);
uint32 user_motor_get_period(void);
void user_motor_set_period(uint32 period);

#endif

