/* ************************************************************************
 *       Filename:  zg_at_cmd.c
 *    Description:
 *        Version:  1.0
 *        Created:  2015年12月25日 9时34分29秒
 *       Revision:  none
 *       Compiler:  eclipse
 *         Author:  YOUR NAME (zhang gong)
 *        Company:  wld
 * ************************************************************************/
#include "espconn.h"
#include "mem.h"
#include "smartconfig.h"
#include "osapi.h"
#include "user_interface.h"
#include "at_custom.h"
#include "../include/user_plug.h"

#if 0
	typedef enum {
    SC_STATUS_WAIT = 0,
    SC_STATUS_FIND_CHANNEL,
    SC_STATUS_GETTING_SSID_PSWD,
    SC_STATUS_LINK,
    SC_STATUS_LINK_OVER,
	} sc_status;
#endif

void user_smartconfig_done(sc_status status, void *pdata){

	uint8_t temp[64] = {0};

	switch(status){
		case SC_STATUS_WAIT:
			at_port_print("SC_STATUS_WAIT\n");
			break;
		case SC_STATUS_FIND_CHANNEL:
			at_port_print("SC_STATUS_FIND_CHANNEL\n");
			break;
		case SC_STATUS_GETTING_SSID_PSWD:
			at_port_print("SC_STATUS_GET_SSID_PSWD\n");
			sc_type *type = pdata;
			if(*type == SC_TYPE_ESPTOUCH){
				at_port_print("SC_TYPE:SC_TYPE_ESPTOUCH\n");
			}else{
				at_port_print("SC_TYPE:SC_TYPE_AIRKISS\n");
			}
			break;
		case SC_STATUS_LINK:
			at_port_print("SC_STATUS_LINK\n");
			struct station_config *sta_conf = pdata;
			wifi_station_set_config(sta_conf);
			wifi_station_disconnect();
			wifi_station_connect();
			break;
		case SC_STATUS_LINK_OVER:
			at_port_print("SC_STATUS_OVER\n");
			if(pdata != NULL){
				uint8 phone_ip[4] = {0};
				os_memcpy(phone_ip, (uint8 *)pdata, 4);
				os_sprintf(temp, "phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
				at_port_print(temp);
			}
			smartconfig_stop();
			os_timer_disarm(&Led_timer);
			wifi_status_led_install(PLUG_WIFI_LED_IO_NUM, PLUG_WIFI_LED_IO_MUX, PLUG_WIFI_LED_IO_FUNC);
			break;
		default:
			break;
	}
}

//void smartconfig_init(){
//
//	smartconfig_start(zg_smartconfig_done);
//	return;
//}
