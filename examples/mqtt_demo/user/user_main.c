#include "esp_common.h"
#include "user_config.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt/MQTTClient.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "c_types.h"
#include "pwm.h"
#include "cJSON.h"
#include "lwip/apps/sntp_time.h"
#include "lwip/apps/time.h"

#ifdef PLUG_DEVICE
#include "user_plug.h"
#endif

#ifdef MOTOR_DEVICE
#include "user_motor.h"
#endif

/******************************************************************************/

void wifi_event_handler_cb(System_Event_t *event) {
	if (event == NULL) {
		return;
	}

	switch (event->event_id) {
	case EVENT_STAMODE_GOT_IP:
		os_printf("sta got ip ,create task and free heap size is %d\n",
				system_get_free_heap_size());
		//user_conn_init();
		break;

	case EVENT_STAMODE_CONNECTED:
		os_printf("sta connected\n");
		break;

	case EVENT_STAMODE_DISCONNECTED:
		wifi_station_connect();
		break;

	default:
		break;
	}
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 user_rf_cal_sector_set(void) {
	flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch (size_map) {
	case FLASH_SIZE_4M_MAP_256_256:
		rf_cal_sec = 128 - 5;
		break;

	case FLASH_SIZE_8M_MAP_512_512:
		rf_cal_sec = 256 - 5;
		break;

	case FLASH_SIZE_16M_MAP_512_512:
	case FLASH_SIZE_16M_MAP_1024_1024:
		rf_cal_sec = 512 - 5;
		break;

	case FLASH_SIZE_32M_MAP_512_512:
	case FLASH_SIZE_32M_MAP_1024_1024:
		rf_cal_sec = 1024 - 5;
		break;

	default:
		rf_cal_sec = 0;
		break;
	}

	return rf_cal_sec;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/

void ICACHE_FLASH_ATTR user_init(void) {
	printf("SDK version:%s\n", system_get_sdk_version());
	wifi_set_opmode(STATION_MODE);
	struct station_config config;
	bzero(&config, sizeof(struct station_config));
	sprintf(config.ssid, "gratiot");
	sprintf(config.password, "zaq12345**");
	wifi_station_set_config(&config);
	wifi_set_event_handler_cb(wifi_event_handler_cb);
	wifi_station_connect();

#ifdef MQTT_ECHO
	user_conn_init();
#endif

#ifdef PLUG_DEVICE
	user_plug_init();
#endif
}
