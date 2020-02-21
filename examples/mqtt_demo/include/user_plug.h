#ifndef __USER_ESPSWITCH_H__
#define __USER_ESPSWITCH_H__

#include "key.h"
#include "c_types.h"


#define PLUG_KEY_NUM        2
//GPIO INPUT SETTINGS

#define  START_MUC     PERIPHS_IO_MUX_GPIO0_U
#define  START_NUM     0
#define  START_FUNC    FUNC_GPIO0

#define  STOP_MUC      PERIPHS_IO_MUX_GPIO2_U
#define  STOP_NUM      2
#define  STOP_FUNC     FUNC_GPIO2

#define LED_IO_MUX     PERIPHS_IO_MUX_GPIO5_U
#define LED_IO_NUM     5
#define LED_IO_FUNC    FUNC_GPIO5

#define LED_RUN_MUX    PERIPHS_IO_MUX_MTDO_U
#define LED_RUN_NUM    15
#define LED_RUN_FUNC   FUNC_GPIO15

#define FAN_MUX        PERIPHS_IO_MUX_MTCK_U
#define FAN_NUM        13
#define FAN_FUNC       FUNC_GPIO13

#define PLUG_STATUS_OUTPUT(pin, on)     GPIO_OUTPUT_SET(pin, on)

struct plug_saved_param {
	uint8 status;
	uint8 pad[3];
};

enum {
    LED_OFF = 0,
    LED_ON  = 1,
    LED_1HZ,
    LED_5HZ,
    LED_20HZ,
};

void ICACHE_FLASH_ATTR user_plug_init(void);
uint8 user_plug_get_status_1(void);
uint8 user_plug_get_status_2(void);

void user_plug_set_status_1(bool status1);
void user_plug_set_status_2(bool status2);

BOOL user_get_key_status_1(void);
BOOL user_get_key_status_2(void);

#endif

