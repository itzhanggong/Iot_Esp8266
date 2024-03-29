#ifndef __USER_ESPSWITCH_H__
#define __USER_ESPSWITCH_H__

#include "driver/key.h"

extern bool plug_status;
extern os_timer_t Led_timer;

/* NOTICE---this is for 512KB spi flash.
 * you can change to other sector if you use other size spi flash. */
#define PRIV_PARAM_START_SEC			0x3C

#define PRIV_PARAM_SAVE   				0

#define PLUG_KEY_NUM          		    1

extern void user_plug_init();
extern uint8 user_plug_get_status();
extern void user_plug_set_status(bool status);

#define PLUG_WIFI_LED_IO_NUM 	5
#define PLUG_WIFI_LED_IO_MUX 	PERIPHS_IO_MUX_GPIO0_U
#define PLUG_WIFI_LED_IO_FUNC   FUNC_GPIO5

#define PLUG_KEY_0_IO_MUX     	PERIPHS_IO_MUX_MTCK_U
#define PLUG_KEY_0_IO_NUM    	4
#define PLUG_KEY_0_IO_FUNC   	FUNC_GPIO4

#define PLUG_KEY_NUM 			1

extern os_timer_t Led_timer;
/*
#define PLUG_LINK_LED_IO_MUX     PERIPHS_IO_MUX_MTDI_U
#define PLUG_LINK_LED_IO_NUM     12
#define PLUG_LINK_LED_IO_FUNC    FUNC_GPIO12

#define PLUG_RELAY_LED_IO_MUX     PERIPHS_IO_MUX_MTDO_U
#define PLUG_RELAY_LED_IO_NUM     15
#define PLUG_RELAY_LED_IO_FUNC    FUNC_GPIO15

#define PLUG_STATUS_OUTPUT(pin, on)     GPIO_OUTPUT_SET(pin, on)

struct plug_saved_param {
    uint8_t status;
    uint8_t pad[3];
};*/

#endif

