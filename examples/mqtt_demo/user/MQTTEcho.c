#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "mqtt/MQTTClient.h"
#include "gpio.h"
#include "user_config.h"
#include "c_types.h"
#include "esp_common.h"

#include "pwm.h"
#include "lwip/apps/sntp_time.h"
#include "lwip/apps/time.h"
#include "cJSON.h"

#ifdef PLUG_DEVICE
#include "user_plug.h"
#endif

#ifdef MOTOR_DEVICE
#include "user_motor.h"
#endif

#define MQTT_CLIENT_THREAD_NAME         "mqtt_client_thread"
#define MQTT_CLIENT_THREAD_STACK_WORDS  2048
#define MQTT_CLIENT_THREAD_PRIO         8

MQTTMessage message;
MQTTClient client;
Network network;

extern int pub1, pub2, pub3;

int send_bz;
int retry = 0;
int send_delay;
const char *topicFilter;
int retain;
char *payload[100];

short int total_time, play_time, delay_time;
int speed_min, speed_max, feeder_speed;
int speed_run;
bool flag_stop = 1;
int cnt = 0;
int flag_ready = 0, tt = 2;
uint8 auto_feed, total_play = 0;
uint32 time_unix;
int ratio;
float duties;
int start_status = 0, stop_status = 0;

extern char *mess[200];
int split(const char *txt, char delim, char ***tokens) {
	int *tklen, *t, count = 1;
	char **arr, *p = (char*) txt;

	while (*p != '\0')
		if (*p++ == delim)
			count += 1;
	t = tklen = calloc(count, sizeof(int));
	for (p = (char*) txt; *p != '\0'; p++)
		*p == delim ? *t++ : (*t)++;
	*tokens = arr = malloc(count * sizeof(char*));
	t = tklen;
	p = *arr++ = calloc(*(t++) + 1, sizeof(char*));
	while (*txt != '\0') {
		if (*txt == delim) {
			p = *arr++ = calloc(*(t++) + 1, sizeof(char*));
			txt++;
		} else
			*p++ = *txt++;
	}
	free(tklen);
	return count;
}
void MQTTPub(const char *topicFilter, char *payload, int retain, int send_delay) {
	message.qos = QOS0;
	message.retained = retain;
	message.payload = payload;
	message.payloadlen = strlen(payload);
	send_bz = 1;
}

LOCAL xTaskHandle mqttc_client_handle;
static void messageArrived(MessageData *data) {
	char topicBuf[80];
	char payload[100];

	char **tokens;
	char **msg;
	int count, i;

	cJSON *json;

	uint16 data_len = data->message->payloadlen;
	strlcpy(topicBuf, data->topicName->lenstring.data,
			data->topicName->lenstring.len + 1);
	strlcpy(payload, data->message->payload, data_len + 1);

	const char *str = topicBuf;
	count = split(str, '/', &tokens);

	/* freeing tokens */
	for (i = 0; i < count; i++)
		free(tokens[i]);
	free(tokens);

	os_printf("topic :  %s,   message : %s\n", topicBuf, payload);

	if (strcmp(tokens[3], "auto_feed") == 0) {
		json = cJSON_Parse(payload);
		total_time = cJSON_GetObjectItem(json, "total_time")->valueint;
		os_printf("total_time :  %d\n", total_time);

		play_time = cJSON_GetObjectItem(json, "play_time")->valueint;
		os_printf("play_time  :  %d\n", play_time);

		delay_time = cJSON_GetObjectItem(json, "delay_time")->valueint;
		os_printf("delay_time :  %d\n", delay_time);

		// Free the memory!
		cJSON_Delete(json);
	} else if (strcmp(tokens[3], "configs") == 0) {
		json = cJSON_Parse(payload);
		speed_min = 10 * cJSON_GetObjectItem(json, "min_speed")->valueint;
		os_printf("speed_min :  %d\n", speed_min);

		speed_max = 10 * cJSON_GetObjectItem(json, "max_speed")->valueint;
		os_printf("speed_max :  %d\n", speed_max);

		feeder_speed = 10 * cJSON_GetObjectItem(json, "feeder_speed")->valueint;
		os_printf("feeder_speed :  %d\n", feeder_speed);
		// Free the memory!
		cJSON_Delete(json);
	} else if (strcmp(tokens[3], "auto_feed1") == 0) {
		json = cJSON_Parse(payload);
		auto_feed = cJSON_GetObjectItem(json, "auto_feed")->valueint;
		os_printf("auto_feed :  %d\n", auto_feed);
		if (auto_feed == 1) {
			if (start_status == 0) {
				flag_stop = 0;
				total_play = 0;
				flag_ready = 0;
				cnt = 0;

				start_status = 1;
				speed_run = speed_max;

				duties = (float) (speed_max - speed_min)
						/ (float) (play_time + 2 * tt);
				ratio = (int) (duties);
			}
		}
		if (auto_feed == 3) {

			GPIO_OUTPUT_SET(LED_RUN_NUM, 0); // Set onboard LED off
			GPIO_OUTPUT_SET(FAN_NUM, 1);
			flag_stop = 1;
			start_status = 0;
			sprintf((char*) mess,
					"{ \"auto_feed\": %d, \"next_time\": %d, \"total_play\": %d, \"total_time\": %d }",
					3, NULL, NULL, NULL);
			MQTTPub("tele/92/1/auto_feed", (char*) mess, 1, 1);
			pub1 = 1;
			pwm_stop(0x0);

			total_time = 0;
			play_time = 0;
			delay_time = 0;

			speed_min = 0;
			speed_max = 0;
			feeder_speed = 0;
		}
		// Free the memory!
		cJSON_Delete(json);
	} else
		os_printf("Error!!!\n");

}
/****************************************************************************/

static void mqtt_client_thread(void *pvParameters) {
	os_printf("mqtt client thread starts\n");

	unsigned char sendbuf[1000] = { 0 }, readbuf[1000] = { 0 };
	int rc = 0, count = 0;

	char szMac[30];
	char szStatusTopic[256];
	snprintf(szStatusTopic, sizeof(szStatusTopic), "tele/92/1/LWT", szMac);

	MQTTString willTopic = MQTTString_initializer;
	willTopic.cstring = szStatusTopic;

	MQTTString willMessage = MQTTString_initializer;
	willMessage.cstring = "Offline";

	MQTTPacket_willOptions willOptions = MQTTPacket_willOptions_initializer;
	willOptions.topicName = willTopic;
	willOptions.message = willMessage;
	willOptions.retained = 1;
	willOptions.qos = QOS0;

	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

	pvParameters = 0;
	NetworkInit(&network);
	MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf,
			sizeof(readbuf));

	char *address = MQTT_BROKER;
	startconnect: if ((rc = NetworkConnect(&network, address, MQTT_PORT)) != 0) {
		os_printf("Return code from network connect is %d\n", rc);
	}

#ifdef MQTT_TASK
    if ((rc = MQTTStartTask(&client)) != pdPASS) {
    	os_printf("Return code from start tasks is %d\n", rc);
    } else {
    	os_printf("Use MQTTStartTask\n");
    }
#endif

//	startconnect: (rc = NetworkConnect(&network, address, MQTT_PORT));
	connectData.MQTTVersion = 3;
	connectData.clientID.cstring = "ESP8266_sample_code";
	connectData.willFlag = 1;
	connectData.will = willOptions;
	connectData.keepAliveInterval = 10;

	mqttconnect: if ((rc = MQTTConnect(&client, &connectData)) != 0) {
		os_printf("MQTT connect fail, rc = %d\n", rc);
		vTaskDelay(2000 / portTICK_RATE_MS);
		goto mqttconnect;
	} else {
		os_printf("MQTT Connected\n");
	}

	MQTTMessage statusMessage;
	statusMessage.qos = QOS0;
	statusMessage.retained = 1;
	statusMessage.payload = "Online";
	statusMessage.payloadlen = strlen("Online");

	rc = MQTTPublish(&client, szStatusTopic, &statusMessage);
	if (rc != SUCCESS) {
		os_printf("MQTTPublish LWT: %d\n", rc);
	}

	if ((rc = MQTTSubscribe(&client, "cmnd/92/1/#", 0, messageArrived)) != 0) {
		os_printf("Return code from MQTT subscribe is %d\n", rc);
	} else {
		os_printf("MQTT subscribe to topic \"cmnd/92/1/#\"\n");
	}

	while (1) {
		rc = MQTTIsConnected(&client);

		if (!rc) {
			retry++;
#ifdef DEBUG_MODE
			os_printf("MQTT is disconnect [%d] [%d]\n", rc, retry);
#endif
		} else {
			retry = 0;
		}

		if (retry > 100) {
#ifdef DEBUG_MODE
			os_printf("disconnect\n");
#endif
			MQTTDisconnect(&client);
			retry = 0;
			vTaskDelay(100 / portTICK_RATE_MS);
			goto startconnect;
		}
		vTaskDelay(100 / portTICK_RATE_MS);

		if (send_bz > 1) {
			send_bz--;
		}
		if (send_bz) {
			if (pub1) {
				if ((rc = MQTTPublish(&client, "tele/92/1/auto_feed", &message))
						== 0) {
					send_bz = 0;
					os_printf("\n--->Publish auto success\n\n");
				} else {
					send_bz = 0;
					os_printf("\n--->Publish auto fail\n\n");
				}
				pub1 = 0;
			} else if (pub2) {
				if ((rc = MQTTPublish(&client, "tele/92/1/ph", &message))
						== 0) {
					send_bz = 0;
					os_printf("\n--->Publish ph success\n\n");
				} else {
					send_bz = 0;
					os_printf("\n--->Publish ph fail\n\n");
				}
				pub2 = 0;
			} else if (pub3) {
				if ((rc = MQTTPublish(&client, "tele/92/1/temperature", &message))
						== 0) {
					send_bz = 0;
					os_printf("\n--->Publish tem success\n\n");
				} else {
					send_bz = 0;
					os_printf("\n--->Publish tem fail\n\n");
				}
				pub3 = 0;
			}
		}
	}
	os_printf("mqtt_client_thread going to be deleted\n");
	vTaskDelete(NULL);
	return;
}

/****************************************************************************/
void user_conn_init(void) {
	int ret;
	ret = xTaskCreate(mqtt_client_thread, MQTT_CLIENT_THREAD_NAME,
			MQTT_CLIENT_THREAD_STACK_WORDS, NULL, MQTT_CLIENT_THREAD_PRIO,
			&mqttc_client_handle);

	if (ret != pdPASS) {
		printf("mqtt create client thread %s failed\n",
		MQTT_CLIENT_THREAD_NAME);
	}
}
