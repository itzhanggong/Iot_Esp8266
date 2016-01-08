/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_devicefind.h"
#include "user_webserver.h"
#include "../include/driver/uart.h"
#include "gpio.h"
#if ESP_PLATFORM
#include "user_esp_platform.h"
#endif



void system_init_done(){

	//os_timer_disarm(&wifi_status_timer);
	//os_timer_setfn(&wifi_status_timer, (os_timer_func_t *)wifi_status_callback, NULL);
	//os_timer_arm(&wifi_status_timer, 1000, 1);

	return;
}
void user_rf_pre_init(void)
{
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_printf("SDK version:%s\n", system_get_sdk_version());

    //user_esp_platform_init();
    user_plug_init();

    user_devicefind_init();

    user_webserver_init(SERVER_PORT);

    system_init_done_cb(system_init_done);					//系统初始化后回调。
}

