/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_plug.c
 *
 * Description: plug demo's function realization
 *
 * Modification history:
 *     2014/5/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "user_plug.h"
#include "user_smartconfig.h"

LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[PLUG_KEY_NUM];

os_timer_t Led_timer;

bool len_status = 0;
bool is_after_long_press = 0;
bool plug_status;

uint8 ICACHE_FLASH_ATTR
user_plug_get_status(void)
{
    //return plug_param.status;
}

/******************************************************************************
 * FunctionName : user_plug_set_status
 * Description  : set plug's status, 0x00 or 0x01
 * Parameters   : uint8 - status
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_plug_set_status(bool status)
{
    /*if (status != plug_param.status) {
        if (status > 1) {
            os_printf("error status input!\n");
            return;
        }

        plug_param.status = status;
        PLUG_STATUS_OUTPUT(PLUG_RELAY_LED_IO_NUM, status);
    }*/
}


void Led_timer_callback(){

	len_status = !len_status;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(5), len_status);
}

static void user_plug_long_press(void){

	is_after_long_press = true;
	wifi_status_led_uninstall();

	os_timer_disarm(&Led_timer);
	os_timer_setfn(&Led_timer, &Led_timer_callback);
	os_timer_arm(&Led_timer, 2000, 1);

	smartconfig_stop();
	smartconfig_start(user_smartconfig_done);
}

static void user_plug_short_press(void){
	if(is_after_long_press){
		is_after_long_press = false;
		return;
	}
	plug_status = !plug_status;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(12), plug_status);
}


void ICACHE_FLASH_ATTR
user_plug_init(void)
{
	plug_status = ON;

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(12), plug_status);						//每次上电，插座都为开状态。

	wifi_status_led_install(PLUG_WIFI_LED_IO_NUM, PLUG_WIFI_LED_IO_MUX, PLUG_WIFI_LED_IO_FUNC);

	single_key[0] = key_init_single(PLUG_KEY_0_IO_NUM, PLUG_KEY_0_IO_MUX, PLUG_KEY_0_IO_FUNC,
		                                    user_plug_long_press, user_plug_short_press);

    keys.key_num = PLUG_KEY_NUM;
    keys.single_key = single_key;

    key_init(&keys);
}

