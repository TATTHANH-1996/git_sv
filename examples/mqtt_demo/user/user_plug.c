#include "esp_common.h"
#include "user_config.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt/MQTTClient.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "c_types.h"
#include "pwm.h"
#include "ds18b20.h"
#include "lwip/apps/sntp_time.h"
#include "lwip/apps/time.h"
#include <math.h>

#ifdef PLUG_DEVICE
#include "user_plug.h"
#endif

#ifdef MOTOR_DEVICE
#include "user_motor.h"
#endif

//PWM period 1000us(1Khz)
int PWM_PERIOD = 0;

struct motor_saved_param motor_param;

LOCAL struct plug_saved_param plug_param1;
LOCAL struct plug_saved_param plug_param2;

LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[PLUG_KEY_NUM];
LOCAL os_timer_t link_led_timer;
LOCAL uint8 link_led_level = 0;

LOCAL os_timer_t pH_timer, ds18b20_timer;
char *pH[30];
float pHVol = 0, phValue = 0;
float pH_change;
uint16 avgValue = 0;
int count_ph = 0;

bool r = 0;
float t, t_value = 0;
char *tem[20];
int count_tem = 0;

extern short int total_time, play_time, delay_time;
extern int speed_min, speed_max, feeder_speed;
extern int speed_run;
extern bool flag_stop;
extern int cnt;
extern int flag_ready, tt;
extern uint8 auto_feed, total_play;
extern int ratio;
extern float duties;
extern uint32 time_unix;
int flag_run_1 = 0;
uint8 auto_change;
char *mess[200];
extern int start_status, stop_status;

int pub1 = 0, pub2 = 0, pub3 = 0;

extern void MQTTPub(const char *topicFilter, char *payload, int retain,
		int send_delay);

//pwm out_put io
uint32 pwmio_info[8][3] = { { PWM_0_OUT_IO_MUX, PWM_0_OUT_IO_FUNC,
PWM_0_OUT_IO_NUM }, {
PWM_1_OUT_IO_MUX, PWM_1_OUT_IO_FUNC, PWM_1_OUT_IO_NUM }, };

/****************************************************************************/
// SET TIME UNIX
void ICACHE_FLASH_ATTR time_sync_init(void) {
	if (configTime(0, 6, "0.asia.pool.ntp.org", "1.asia.pool.ntp.org",
			"2.asia.pool.ntp.org", 1)) {
		struct timeval tim;
		os_printf("time %d", gettimeofday(&tim,NULL));
	}
}

/*******************************************************************************/

uint8 user_plug_get_status_1(void) {
	return plug_param1.status;
}
uint8 user_plug_get_status_2(void) {
	return plug_param2.status;
}

//      BUTTON START

void user_plug_set_status_1(bool status1) {
	if (status1 != plug_param1.status) {
		if (status1 > 1) {
			printf("error status input!\n");
			return;
		}
		if (start_status == 0) {
			total_time = 5;
			play_time = 10;
			delay_time = 30;

			speed_min = 400;
			speed_max = 1000;
			feeder_speed = 200;

			cnt = 0;
			total_play = 0;
			speed_run = speed_max;
			duties = (float) (speed_max - speed_min)
					/ (float) (play_time + 2 * tt);
			ratio = (int) (duties);
			start_status = 1;
			flag_stop = 0;
			auto_feed = 1;
			sprintf((char*) mess,
					"{ \"auto_feed\": %d, \"next_time\": %d, \"total_play\": %d, \"total_time\": %d }",
					auto_feed, NULL, total_play, total_time);
			MQTTPub("tele/92/1/auto_feed", (char*) mess, 1, 1);
			pub1 = 1;
		}

		plug_param1.status = status1;
	}
}

//      BUTTON STOP

void user_plug_set_status_2(bool status2) {
	if (status2 != plug_param2.status) {
		if (status2 > 1) {
			printf("error status input!\n");
			return;
		}
		if (stop_status == 0) {
			flag_stop = 1;
			GPIO_OUTPUT_SET(LED_RUN_NUM, 0);
			GPIO_OUTPUT_SET(FAN_NUM, 1);
			auto_feed = 3;
			pwm_stop(0x0);
			os_printf("STOP\n");
			sprintf((char*) mess,
					"{ \"auto_feed\": %d, \"next_time\": %d, \"total_play\": %d, \"total_time\": %d }",
					auto_feed, NULL, NULL, NULL);
			MQTTPub("tele/92/1/auto_feed", (char*) mess, 1, 1);
			pub1 = 1;
			total_time = 0;
			play_time = 0;
			delay_time = 0;

			speed_min = 0;
			speed_max = 0;
			feeder_speed = 0;
			cnt = 0;
			start_status = 0;
		}
		plug_param2.status = status2;
	}
}

/******************************************************************************
 * FunctionName : user_plug_short_press
 * Description  : key's short press function, needed to be installed
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
LOCAL void user_plug_short_press_1(void) {
	user_plug_set_status_1((~plug_param1.status) & 0x01);
	spi_flash_erase_sector(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE);
	spi_flash_write(
			(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
			(uint32*) &plug_param1, sizeof(struct plug_saved_param));
}

LOCAL void user_plug_short_press_2(void) {
	user_plug_set_status_2((~plug_param2.status) & 0x01);
	spi_flash_erase_sector(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE);
	spi_flash_write(
			(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
			(uint32*) &plug_param2, sizeof(struct plug_saved_param));
}

/******************************************************************************
 * FunctionName : user_plug_long_press
 * Description  : key's long press function, needed to be installed
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
LOCAL void user_plug_long_press(void) {
	int boot_flag = 12345;
	system_restore();

	system_rtc_mem_write(70, &boot_flag, sizeof(boot_flag));
	printf("long_press boot_flag %d  \n", boot_flag);
	system_rtc_mem_read(70, &boot_flag, sizeof(boot_flag));
	printf("long_press boot_flag %d  \n", boot_flag);
	system_restart();
}

/******************************************************************************
 * FunctionName : user_link_led_init
 * Description  : int led gpio
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
LOCAL void user_link_led_init(void) {
	PIN_FUNC_SELECT(LED_IO_MUX, LED_IO_FUNC);
}

LOCAL void user_link_led_timer_cb(void) {
	link_led_level = (~link_led_level) & 0x01;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_IO_NUM), link_led_level);
}

void user_link_led_timer_init(int time) {
	os_timer_disarm(&link_led_timer);
	os_timer_setfn(&link_led_timer, (os_timer_func_t*) user_link_led_timer_cb,
	NULL);
	os_timer_arm(&link_led_timer, time, 1);
	link_led_level = 0;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_IO_NUM), link_led_level);
}

/******************************************************************************
 * FunctionName : user_link_led_output
 * Description  : led flash mode
 * Parameters   : mode, on/off/xhz
 * Returns      : none
 *******************************************************************************/
void user_link_led_output(uint8 mode) {

	switch (mode) {
	case LED_OFF:
		os_timer_disarm(&link_led_timer);
		GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_IO_NUM), 1);
		break;

	case LED_ON:
		os_timer_disarm(&link_led_timer);
		GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_IO_NUM), 0);
		break;

	case LED_1HZ:
		user_link_led_timer_init(1000);
		break;

	case LED_5HZ:
		user_link_led_timer_init(200);
		break;

	case LED_20HZ:
		user_link_led_timer_init(50);
		break;

	default:
		printf("ERROR:LED MODE WRONG!\n");
		break;
	}

}
/******************************************************************************
 * FunctionName : user_get_key_status
 * Description  : a
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
BOOL user_get_key_status_1(void) {
	return get_key_status(single_key[0]);
}
BOOL user_get_key_status_2(void) {
	return get_key_status(single_key[1]);
}

/******************************************************************************
 * FunctionName : user_motor_get_duty
 * Description  : get duty of each channel
 * Parameters   : uint8 channel : MOTOR_CAP/MOTOR_PHOI
 * Returns      : NONE
 *******************************************************************************/
uint32 user_motor_get_duty(uint8 channel) {
	return motor_param.pwm_duty[channel];
}

/******************************************************************************
 * FunctionName : user_motor_set_duty
 * Description  : set each channel's duty params
 * Parameters   : uint8 duty    : 0 ~ PWM_PERIOD
 *                uint8 channel : MOTOR_CAP/MOTOR_PHOI
 * Returns      : NONE
 *******************************************************************************/
void user_motor_set_duty(uint32 duty, uint8 channel) {
	if (duty != motor_param.pwm_duty[channel]) {
		pwm_set_duty(duty, channel);

		motor_param.pwm_duty[channel] = pwm_get_duty(channel);
	}
}

/******************************************************************************
 * FunctionName : user_motor_get_period
 * Description  : get pwm period
 * Parameters   : NONE
 * Returns      : uint32 : pwm period
 *******************************************************************************/
uint32 user_motor_get_period(void) {
	return motor_param.pwm_period;
}

/******************************************************************************
 * FunctionName : user_motor_set_period
 * Description  : set pwm frequency
 * Parameters   : uint16 freq : 1000hz typically
 * Returns      : NONE
 *******************************************************************************/
void user_motor_set_period(uint32 period) {
	if (period != motor_param.pwm_period) {
		pwm_set_period(period);

		motor_param.pwm_period = pwm_get_period();
	}
}

/******************************************************************************/
void user_motor_restart(void) {
	spi_flash_erase_sector(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE);
	spi_flash_write(
			(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
			(uint32*) &motor_param, sizeof(struct motor_saved_param));

	pwm_start();
}
/******************************************************************************/

void ICACHE_FLASH_ATTR Motor_PWM() {
	os_printf("Flag_stop: %d\n", flag_stop);
	speed_run -= ratio;
	user_motor_set_duty(speed_run, MOTOR_PHOI);
	float duty1 = (float) (speed_run / 10);
	int speed = (int) duty1;
	os_printf("Duty_1: %d %%\n", speed);

	if (flag_ready > tt && flag_ready <= (play_time + tt)) {
		user_motor_set_duty(feeder_speed, MOTOR_CAP);
		os_printf("Duty_0: %d %%\n", (feeder_speed / 10));
	} else {
		user_motor_set_duty(PWM_PERIOD, MOTOR_CAP);
		flag_run_1 = 1;
		os_printf("Duty_0: %d %%\n", PWM_PERIOD);
	}

	if (speed_run <= speed_min) {
		pwm_stop(0x0);
		flag_run_1 = 0;
		flag_stop = 1;

		cnt++;
		os_printf("cnt: %d\n", cnt);
		if (cnt < total_time) {
			speed_run = speed_max;
			duties = (float) (speed_max - speed_min)
					/ (float) (play_time + 2 * tt);
			ratio = (int) (duties);
			flag_ready = 0;
			flag_stop = 0;
			total_play += 1;
			auto_feed = 0;
			os_printf("auto_feed: %d\n", auto_feed);
			time_unix = sntp_get_current_timestamp();
			time_unix = (time_unix + delay_time);
			os_printf("time_unix: %d\n", time_unix);

			sprintf((char*) mess,
					"{ \"auto_feed\": %d, \"next_time\": %d, \"total_play\": %d, \"total_time\": %d }",
					auto_feed, time_unix, total_play, total_time);
			MQTTPub("tele/92/1/auto_feed", (char*) mess, 1, 1);
			pub1 = 1;
			os_printf("next_time: %d (s)\n", delay_time);
			vTaskDelay((delay_time * 1000) / portTICK_RATE_MS);
		} else {
			pwm_stop(0x0);
			GPIO_OUTPUT_SET(LED_RUN_NUM, 0); // Set onboard LED off
			GPIO_OUTPUT_SET(FAN_NUM, 1);
			auto_feed = 3;
			os_printf("auto_feed: %d\n", auto_feed);
			total_play = total_time;

			sprintf((char*) mess,
					"{ \"auto_feed\": %d, \"next_time\": %d, \"total_play\": %d, \"total_time\": %d }",
					auto_feed, NULL, total_play, total_time);
			MQTTPub("tele/92/1/auto_feed", (char*) mess, 1, 1);
			pub1 = 1;

			total_time = 0;
			play_time = 0;
			delay_time = 0;

			speed_min = 0;
			speed_max = 0;
			feeder_speed = 0;
			flag_stop = 1;
			cnt = 0;
			start_status = 0;
		}
	} else {
		GPIO_OUTPUT_SET(LED_RUN_NUM, 1); // Set onboard LED on
		GPIO_OUTPUT_SET(FAN_NUM, 0);
		user_motor_restart();
		auto_feed = 1;
		os_printf("auto_feed: %d\n", auto_feed);

		if (auto_change != auto_feed) {
			sprintf((char*) mess,
					"{ \"auto_feed\": %d, \"next_time\": %d, \"total_play\": %d, \"total_time\": %d }",
					auto_feed, NULL, total_play, total_time);
			MQTTPub("tele/92/1/auto_feed", (char*) mess, 1, 1);
			pub1 = 1;
		}
	}

	os_printf("ratio: %d\n", ratio);
	vTaskDelay(1000 / portTICK_RATE_MS);
	flag_ready++;
	auto_change = auto_feed;
	os_printf("Flag_ready: %d\n", flag_ready);
}

void ICACHE_FLASH_ATTR Task_Handle(void *pvParameters) {
	pvParameters = 0;
	for (;;) {
		if (flag_stop) {
			pwm_stop(0x0);
		} else {
			Motor_PWM();
		}
	}
	vTaskDelete(NULL);
	return;
}

/******************************************************************************/

void ICACHE_FLASH_ATTR Read_pH(void *arg) {
	char buff[30];
	avgValue = system_adc_read();
	os_printf("Read pH\n");

	pHVol = (float) (avgValue * (3.3 / 1024));  // convert analog into millivolt
	phValue = -5.7 * (pHVol - 0.2) + 21.04;   // convert millivolt into pH value

	os_printf("pHVol = %s\n", Float2String(buff, pHVol));
	os_printf("avgValue = %s\n", Float2String(buff, avgValue));
	os_printf("Value pH = %s\n", Float2String(buff, phValue));

	if (fabs(pH_change - phValue) > 0.1) {
		time_unix = sntp_get_current_timestamp();
		os_printf("time_unix pH = %d\n", time_unix);
		sprintf((char*) pH, "{ \"ph\": %0.2f, \"Time\": %d }", phValue,
				time_unix);

		MQTTPub("tele/92/1/ph", (char*) pH, 1, 1);
		pub2 = 1;
	} else {
		count_ph++;
		if (count_ph == 15) {
			time_unix = sntp_get_current_timestamp();
			os_printf("time_unix = %d\n", time_unix);
			sprintf((char*) pH, "{ \"ph\": %0.2f, \"Time\": %d }", phValue,
					time_unix);

			MQTTPub("tele/92/1/ph", (char*) pH, 1, 1);
			pub2 = 1;
			count_ph = 0;
		}
	}
	pH_change = phValue;
}
void ICACHE_FLASH_ATTR pH_task(void *pvParameters) {
	os_timer_disarm(&pH_timer);
	os_timer_setfn(&pH_timer, (os_timer_func_t*) Read_pH, (void*) 0);
	os_timer_arm(&pH_timer, 3000, 1);

	for (;;)
		;
}
/******************************************************************************/

void READ_SENSOR(void *args) {
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	r = ds1820_read(&t);
	os_printf("r: %d\n", r);
	char buff[30];

	if (r) {
		if (fabs(t - t_value) > 0.5) {
			time_unix = sntp_get_current_timestamp();
			os_printf("time_unix temp = %d\n", time_unix);
			sprintf((char*) tem, "{ \"temperature\": %0.2f, \"Time\": %d }", t,
					time_unix);
			os_printf("Temperature: %s *C\r\n", Float2String(buff, t));

			MQTTPub("tele/92/1/temperature", (char*) tem, 1, 1);
			pub3 = 1;
		} else {
			count_tem++;
			if (count_tem == 10) {
				time_unix = sntp_get_current_timestamp();
				os_printf("time_unix = %d\n", time_unix);
				sprintf((char*) tem, "{ \"temperature\": %0.2f, \"Time\": %d }",
						t, time_unix);
				os_printf("Temperature: %s *C\r\n", Float2String(buff, t));

				MQTTPub("tele/92/1/temperature", (char*) tem, 1, 1);
				pub3 = 1;
				count_tem = 0;
			}
		}
		t_value = t;
	} else {
		os_printf("error reading ds18b20 values ...\n");
	}
}
void ds18b20_task_handle(void *pvParameters) {
	os_timer_disarm(&ds18b20_timer);
	os_timer_setfn(&ds18b20_timer, (os_timer_func_t*) READ_SENSOR, (void*) 0);
	os_timer_arm(&ds18b20_timer, 4000, 1);

	for (;;)
		;
}

/******************************************************************************
 * FunctionName : user_plug_init
 * Description  : init plug's key function and relay output
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR user_plug_init(void) {
	time_sync_init();
	printf("user_plug_init start!\n");

	user_link_led_init();

	wifi_status_led_install(LED_IO_NUM, LED_IO_MUX, LED_IO_FUNC);

	single_key[0] = key_init_single(START_NUM, START_MUC, START_FUNC,
	NULL, user_plug_short_press_1);

	single_key[1] = key_init_single(STOP_NUM, STOP_MUC, STOP_FUNC,
			user_plug_long_press, user_plug_short_press_2);
	keys.key_num = PLUG_KEY_NUM;
	keys.single_key = single_key;
	key_init(&keys);

	spi_flash_read(
			(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
			(uint32*) &plug_param1, sizeof(struct plug_saved_param));
	spi_flash_read(
			(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
			(uint32*) &plug_param2, sizeof(struct plug_saved_param));
	/*init to off*/
	uint32 pwm_duty_init[PWM_CHANNEL];
	motor_param.pwm_period = 1000;
	memset(pwm_duty_init, 0, PWM_CHANNEL * sizeof(uint32));
	pwm_init(motor_param.pwm_period, pwm_duty_init, PWM_CHANNEL, pwmio_info);

	//	/*set target valuve from memory*/
	spi_flash_read(
			(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
			(uint32*) &motor_param, sizeof(struct motor_saved_param));

	/* Select GPIO function for GPIO15 which is connected to onboard LED. */
	PIN_FUNC_SELECT(LED_RUN_MUX, LED_RUN_FUNC);
	PIN_FUNC_SELECT(FAN_MUX, FAN_FUNC);

#ifdef MOTOR_DEVICE
	xTaskCreate(Task_Handle, "Task_Handle", 1024, NULL, 7, NULL);
#endif

#ifdef	SENSOR_DEVICE
	xTaskCreate(pH_task, "pH_task", 256, NULL, 6, NULL);
	xTaskCreate(ds18b20_task_handle, "ds18b20_task_handle", 256, NULL, 6, NULL);
#endif

}
