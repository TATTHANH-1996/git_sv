/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__


#define server_ip "192.168.101.142"
//#define server_ip "192.168.99.1"
//#define server_port 11883
#define server_port 9669


#define DEVICE_TYPE 		"gh_9e2cff3dfa51" //wechat public number
#define DEVICE_ID 			"12247567" //model ID

#define DEFAULT_LAN_PORT 	12476

#define MQTT_BROKER  "broker.hivemq.com"  /* MQTT Broker Address*/
//#define MQTT_BROKER  "192.168.99.123"  /* MQTT Broker Address*/
#define MQTT_PORT    1883             /* MQTT Port*/

//#define ETS_GPIO_INTR_ENABLE()  _xt_isr_unmask(1 << ETS_GPIO_INUM)  //ENABLE INTERRUPTS
//#define ETS_GPIO_INTR_DISABLE() _xt_isr_mask(1 << ETS_GPIO_INUM)    //DISABLE INTERRUPTS

#define DEBUG_MODE              1
#define PLUG_DEVICE             0
#define MOTOR_DEVICE            1
#define SENSOR_DEVICE			0
#define MQTT_ECHO			    1
#define C_JSON_SPLIT		    0

 typedef struct
{
    char retained[10];
    char empty[10];
    unsigned char Topic[30];
    unsigned char payload[30];

} xMessage;

/* NOTICE---this is for 512KB spi flash.
 * you can change to other sector if you use other size spi flash. */
#define PRIV_PARAM_START_SEC        0x7C
#define PRIV_PARAM_SAVE     0

void ICACHE_FLASH_ATTR user_conn_init(void);
void ICACHE_FLASH_ATTR json_start(void);

#endif

