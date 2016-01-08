/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_esp_platform.c
 *
 * Description: The client mode configration.
 *              Check your hardware connection with the host while use this mode.
 *
 * Modification history:
 *     2014/5/09, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "user_esp_platform.h"
#include "user_iot_version.h"
#include "upgrade.h"

#if ESP_PLATFORM

#define ESP_DEBUG

#ifdef ESP_DEBUG
#define ESP_DBG os_printf
#else
#define ESP_DBG
#endif

#define ACTIVE_FRAME    "{\"nonce\": %d,\"path\": \"/v1/device/activate/\", \"method\": \"POST\", \"body\": {\"encrypt_method\": \"PLAIN\", \"token\": \"%s\", \"bssid\": \""MACSTR"\",\"rom_version\":\"%s\"}, \"meta\": {\"Authorization\": \"token %s\"}}\n"

#if PLUG_DEVICE
#include "user_plug.h"

#define RESPONSE_FRAME  "{\"status\": 200, \"datapoint\": {\"x\": %d}, \"nonce\": %d, \"deliver_to_device\": true}\n"
#define FIRST_FRAME     "{\"nonce\": %d, \"path\": \"/v1/device/identify\", \"method\": \"GET\",\"meta\": {\"Authorization\": \"token %s\"}}\n"

LOCAL uint32 count = 0;
#endif

#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"

#if PLUG_DEVICE || LIGHT_DEVICE
#define BEACON_FRAME    "{\"path\": \"/v1/ping/\", \"method\": \"POST\",\"meta\": {\"Authorization\": \"token %s\"}}\n"
#define RPC_RESPONSE_FRAME  "{\"status\": 200, \"nonce\": %d, \"deliver_to_device\": true}\n"
#define TIMER_FRAME     "{\"body\": {}, \"get\":{\"is_humanize_format_simple\":\"true\"},\"meta\": {\"Authorization\": \"Token %s\"},\"path\": \"/v1/device/timers/\",\"post\":{},\"method\": \"GET\"}\n"
#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Authorization: token %s\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"

LOCAL uint8 ping_status;
LOCAL os_timer_t beacon_timer;
#endif

#ifdef USE_DNS
ip_addr_t esp_server_ip;
#endif

LOCAL struct espconn user_conn;
LOCAL struct _esp_tcp user_tcp;
LOCAL os_timer_t client_timer;
 struct esp_platform_saved_param esp_param;
LOCAL uint8 device_status;
LOCAL uint8 device_recon_count = 0;
LOCAL uint32 active_nonce = 0;
LOCAL uint8 iot_version[20] = {0};
struct rst_info rtc_info;
void user_esp_platform_check_ip(uint8 reset_flag);


void ICACHE_FLASH_ATTR
user_esp_platform_get_token(uint8_t *token)
{
    if (token == NULL) {
        return;
    }

    os_memcpy(token, esp_param.token, sizeof(esp_param.token));
}

void ICACHE_FLASH_ATTR
user_esp_platform_set_token(uint8_t *token)
{
    if (token == NULL) {
        return;
    }

    esp_param.activeflag = 0;
    os_memcpy(esp_param.token, token, os_strlen(token));

    system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
}


void ICACHE_FLASH_ATTR
user_esp_platform_set_active(uint8 activeflag)
{
    esp_param.activeflag = activeflag;

    system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
}

void ICACHE_FLASH_ATTR
user_esp_platform_set_connect_status(uint8 status)
{
    device_status = status;
}

uint8 ICACHE_FLASH_ATTR
user_esp_platform_get_connect_status(void)
{
    uint8 status = wifi_station_get_connect_status();

    if (status == STATION_GOT_IP) {
        status = (device_status == 0) ? DEVICE_CONNECTING : device_status;
    }

    ESP_DBG("status %d\n", status);
    return status;
}

int ICACHE_FLASH_ATTR
user_esp_platform_parse_nonce(char *pbuffer)
{
    char *pstr = NULL;
    char *pparse = NULL;
    char noncestr[11] = {0};
    int nonce = 0;
    pstr = (char *)os_strstr(pbuffer, "\"nonce\": ");

    if (pstr != NULL) {
        pstr += 9;
        pparse = (char *)os_strstr(pstr, ",");

        if (pparse != NULL) {
            os_memcpy(noncestr, pstr, pparse - pstr);
        } else {
            pparse = (char *)os_strstr(pstr, "}");

            if (pparse != NULL) {
                os_memcpy(noncestr, pstr, pparse - pstr);
            } else {
                pparse = (char *)os_strstr(pstr, "]");

                if (pparse != NULL) {
                    os_memcpy(noncestr, pstr, pparse - pstr);
                } else {
                    return 0;
                }
            }
        }

        nonce = atoi(noncestr);
    }

    return nonce;
}


void ICACHE_FLASH_ATTR
user_esp_platform_get_info(struct espconn *pconn, uint8 *pbuffer)
{
    char *pbuf = NULL;
    int nonce = 0;

    pbuf = (char *)os_zalloc(packet_size);

    nonce = user_esp_platform_parse_nonce(pbuffer);

    if (pbuf != NULL) {

        os_sprintf(pbuf, RESPONSE_FRAME, user_plug_get_status(), nonce);


        ESP_DBG("%s\n", pbuf);
        espconn_sent(pconn, pbuf, os_strlen(pbuf));
#endif
        os_free(pbuf);
        pbuf = NULL;
    }
}


void ICACHE_FLASH_ATTR
user_esp_platform_set_info(struct espconn *pconn, uint8 *pbuffer)
{

    char *pstr = NULL;
    pstr = (char *)os_strstr(pbuffer, "plug-status");

    if (pstr != NULL) {
        pstr = (char *)os_strstr(pbuffer, "body");

        if (pstr != NULL) {

            if (os_strncmp(pstr + 27, "1", 1) == 0) {
                //user_plug_set_status(0x01);
            } else if (os_strncmp(pstr + 27, "0", 1) == 0) {
                //user_plug_set_status(0x00);
            }
        }
    }

    user_esp_platform_get_info(pconn, pbuffer);
}


LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_reconnect(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_reconnect\n");

    user_esp_platform_check_ip(0);
}


LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_discon_cb(void *arg)
{
    struct espconn *pespconn = arg;
    struct ip_info ipconfig;
	struct dhcp_client_info dhcp_info;
    ESP_DBG("user_esp_platform_discon_cb\n");

    os_timer_disarm(&beacon_timer);


    if (pespconn == NULL) {
        return;
    }

    pespconn->proto.tcp->local_port = espconn_port();


    //user_link_led_output(1);


    user_esp_platform_reconnect(pespconn);

}


LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_discon(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_discon\n");


    //user_link_led_output(1);



    espconn_disconnect(pespconn);

}

LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_sent_cb(void *arg)
{
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_sent_cb\n");
}

LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_sent(struct espconn *pespconn)
{
    uint8 devkey[token_size] = {0};
    uint32 nonce;
    char *pbuf = (char *)os_zalloc(packet_size);

    os_memcpy(devkey, esp_param.devkey, 40);

    if (esp_param.activeflag == 0xFF) {
        esp_param.activeflag = 0;
    }

    if (pbuf != NULL) {
        if (esp_param.activeflag == 0) {
            uint8 token[token_size] = {0};
            uint8 bssid[6];
            active_nonce = os_random() & 0x7FFFFFFF;

            os_memcpy(token, esp_param.token, 40);

            wifi_get_macaddr(STATION_IF, bssid);

            os_sprintf(pbuf, ACTIVE_FRAME, active_nonce, token, MAC2STR(bssid),iot_version, devkey);
        }


        else {
            nonce = os_random() & 0x7FFFFFFF;
            os_sprintf(pbuf, FIRST_FRAME, nonce , devkey);
        }


        ESP_DBG("%s\n", pbuf);

        espconn_sent(pespconn, pbuf, os_strlen(pbuf));


        os_free(pbuf);
    }
}

#if PLUG_DEVICE || LIGHT_DEVICE

LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_sent_beacon(struct espconn *pespconn)
{
    if (pespconn == NULL) {
        return;
    }

    if (pespconn->state == ESPCONN_CONNECT) {
        if (esp_param.activeflag == 0) {
            ESP_DBG("please check device is activated.\n");
            user_esp_platform_sent(pespconn);
        } else {
            uint8 devkey[token_size] = {0};
            os_memcpy(devkey, esp_param.devkey, 40);

            ESP_DBG("user_esp_platform_sent_beacon %u\n", system_get_time());

            if (ping_status == 0) {
                ESP_DBG("user_esp_platform_sent_beacon sent fail!\n");
                user_esp_platform_discon(pespconn);
            } else {
                char *pbuf = (char *)os_zalloc(packet_size);

                if (pbuf != NULL) {
                    os_sprintf(pbuf, BEACON_FRAME, devkey);

                    espconn_sent(pespconn, pbuf, os_strlen(pbuf));


                    ping_status = 0;
                    os_timer_arm(&beacon_timer, BEACON_TIME, 0);
                    os_free(pbuf);
                }
            }
        }
    } else {
        ESP_DBG("user_esp_platform_sent_beacon sent fail!\n");
        user_esp_platform_discon(pespconn);
    }
}

LOCAL void ICACHE_FLASH_ATTR
user_platform_rpc_set_rsp(struct espconn *pespconn, int nonce)
{
    char *pbuf = (char *)os_zalloc(packet_size);

    if (pespconn == NULL) {
        return;
    }

    os_sprintf(pbuf, RPC_RESPONSE_FRAME, nonce);
    ESP_DBG("%s\n", pbuf);

    espconn_sent(pespconn, pbuf, os_strlen(pbuf));

    os_free(pbuf);
}

LOCAL void ICACHE_FLASH_ATTR
user_platform_timer_get(struct espconn *pespconn)
{
    uint8 devkey[token_size] = {0};
    char *pbuf = (char *)os_zalloc(packet_size);
    os_memcpy(devkey, esp_param.devkey, 40);

    if (pespconn == NULL) {
        return;
    }

    os_sprintf(pbuf, TIMER_FRAME, devkey);
    ESP_DBG("%s\n", pbuf);
    espconn_sent(pespconn, pbuf, os_strlen(pbuf));

    os_free(pbuf);
}

LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_upgrade_rsp(void *arg)
{
    struct upgrade_server_info *server = arg;
    struct espconn *pespconn = server->pespconn;
    uint8 devkey[41] = {0};
    uint8 *pbuf = NULL;
    char *action = NULL;

    os_memcpy(devkey, esp_param.devkey, 40);
    pbuf = (char *)os_zalloc(packet_size);

    if (server->upgrade_flag == true) {
        ESP_DBG("user_esp_platform_upgarde_successfully\n");
        action = "device_upgrade_success";
        os_sprintf(pbuf, UPGRADE_FRAME, devkey, action, server->pre_version, server->upgrade_version);
        ESP_DBG("%s\n",pbuf);

        espconn_sent(pespconn, pbuf, os_strlen(pbuf));


        if (pbuf != NULL) {
            os_free(pbuf);
            pbuf = NULL;
        }
    } else {
        ESP_DBG("user_esp_platform_upgrade_failed\n");
        action = "device_upgrade_failed";
        os_sprintf(pbuf, UPGRADE_FRAME, devkey, action,server->pre_version, server->upgrade_version);
        ESP_DBG("%s\n",pbuf);


        espconn_sent(pespconn, pbuf, os_strlen(pbuf));


        if (pbuf != NULL) {
            os_free(pbuf);
            pbuf = NULL;
        }
    }

    os_free(server->url);
    server->url = NULL;
    os_free(server);
    server = NULL;
}

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_begin
 * Description  : Processing the received data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 *                server -- upgrade param
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_upgrade_begin(struct espconn *pespconn, struct upgrade_server_info *server)
{
    uint8 user_bin[9] = {0};
    uint8 devkey[41] = {0};

    server->pespconn = pespconn;

    os_memcpy(devkey, esp_param.devkey, 40);
    os_memcpy(server->ip, pespconn->proto.tcp->remote_ip, 4);


    server->port = 80;


    server->check_cb = user_esp_platform_upgrade_rsp;
    server->check_times = 120000;

    if (server->url == NULL) {
        server->url = (uint8 *)os_zalloc(512);
    }

    if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1) {
        os_memcpy(user_bin, "user2.bin", 10);
    } else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2) {
        os_memcpy(user_bin, "user1.bin", 10);
    }

    os_sprintf(server->url, "GET /v1/device/rom/?action=download_rom&version=%s&filename=%s HTTP/1.0\r\nHost: "IPSTR":%d\r\n"pheadbuffer"",
               server->upgrade_version, user_bin, IP2STR(server->ip),
               server->port, devkey);
    ESP_DBG("%s\n",server->url);


    if (system_upgrade_start(server) == false) {

        ESP_DBG("upgrade is already started\n");
    }
}
#endif

/******************************************************************************
 * FunctionName : user_esp_platform_recv_cb
 * Description  : Processing the received data from the server
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    char *pstr = NULL;
    LOCAL char pbuffer[1024 * 2] = {0};
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_recv_cb %s\n", pusrdata);

    os_timer_disarm(&beacon_timer);

    if (length == 1460) {
        os_memcpy(pbuffer, pusrdata, length);
    }else{
        struct espconn *pespconn = (struct espconn *)arg;
        os_memcpy(pbuffer + os_strlen(pbuffer), pusrdata, length);
        if ((pstr = (char *)os_strstr(pbuffer, "\"activate_status\": ")) != NULL &&
                user_esp_platform_parse_nonce(pbuffer) == active_nonce) {
            if (os_strncmp(pstr + 19, "1", 1) == 0) {
                ESP_DBG("device activates successful.\n");

                device_status = DEVICE_ACTIVE_DONE;
                esp_param.activeflag = 1;
                system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
                user_esp_platform_sent(pespconn);

                ESP_DBG("device activates failed.\n");
                device_status = DEVICE_ACTIVE_FAIL;
            }
       }else if ((pstr = (char *)os_strstr(pbuffer, "\"action\": \"sys_upgrade\"")) != NULL) {
            if ((pstr = (char *)os_strstr(pbuffer, "\"version\":")) != NULL) {
                struct upgrade_server_info *server = NULL;
                int nonce = user_esp_platform_parse_nonce(pbuffer);
                user_platform_rpc_set_rsp(pespconn, nonce);

                server = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));
                os_memcpy(server->upgrade_version, pstr + 12, 16);
                server->upgrade_version[15] = '\0';
                os_sprintf(server->pre_version,"%s%d.%d.%dt%d(%s)",VERSION_TYPE,IOT_VERSION_MAJOR,\
                    	IOT_VERSION_MINOR,IOT_VERSION_REVISION,device_type,UPGRADE_FALG);
                user_esp_platform_upgrade_begin(pespconn, server);
            }
        } else if ((pstr = (char *)os_strstr(pbuffer, "\"action\": \"sys_reboot\"")) != NULL) {
            os_timer_disarm(&client_timer);
            os_timer_setfn(&client_timer, (os_timer_func_t *)system_upgrade_reboot, NULL);
            os_timer_arm(&client_timer, 1000, 0);
        } else if ((pstr = (char *)os_strstr(pbuffer, "/v1/device/timers/")) != NULL) {
            int nonce = user_esp_platform_parse_nonce(pbuffer);
            user_platform_rpc_set_rsp(pespconn, nonce);
            os_timer_disarm(&client_timer);
            os_timer_setfn(&client_timer, (os_timer_func_t *)user_platform_timer_get, pespconn);
            os_timer_arm(&client_timer, 2000, 0);
        } else if ((pstr = (char *)os_strstr(pbuffer, "\"method\": ")) != NULL) {
            if (os_strncmp(pstr + 11, "GET", 3) == 0) {
                user_esp_platform_get_info(pespconn, pbuffer);
            } else if (os_strncmp(pstr + 11, "POST", 4) == 0) {
                user_esp_platform_set_info(pespconn, pbuffer);
            }
        } else if ((pstr = (char *)os_strstr(pbuffer, "ping success")) != NULL) {
            ESP_DBG("ping success\n");
            ping_status = 1;
        } else if ((pstr = (char *)os_strstr(pbuffer, "send message success")) != NULL) {
        } else if ((pstr = (char *)os_strstr(pbuffer, "timers")) != NULL) {
            user_platform_timer_start(pusrdata , pespconn);
        }
        else if ((pstr = (char *)os_strstr(pbuffer, "device")) != NULL) {
            user_platform_timer_get(pespconn);
        }
        os_memset(pbuffer, 0, sizeof(pbuffer));
    }

    os_timer_arm(&beacon_timer, BEACON_TIME, 0);

}


LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_ap_change(void){

    uint8 current_id;
    uint8 i = 0;

    ESP_DBG("user_esp_platform_ap_is_changing\n");

    current_id = wifi_station_get_current_ap_id();
    ESP_DBG("current ap id =%d\n", current_id);

    if(current_id == AP_CACHE_NUMBER-1){
       i = 0;
    }else{
       i = current_id + 1;
    }

    while(wifi_station_ap_change(i) != true){
       i++;
       if(i == AP_CACHE_NUMBER-1){
    	   i = 0;
       }
    }

    device_recon_count = 0;
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
    os_timer_arm(&client_timer, 100, 0);
}


LOCAL bool ICACHE_FLASH_ATTR
user_esp_platform_reset_mode(void)
{
    if (wifi_get_opmode() == STATION_MODE) {
        wifi_set_opmode(STATIONAP_MODE);
    }
    /* delay 5s to change AP */
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_ap_change, NULL);
    os_timer_arm(&client_timer, 5000, 0);

    return true;

}

LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_recon_cb(void *arg, sint8 err)
{
    struct espconn *pespconn = (struct espconn *)arg;

    ESP_DBG("user_esp_platform_recon_cb\n");

    os_timer_disarm(&beacon_timer);
    //user_link_led_output(1);

    if (++device_recon_count == 5) {
        device_status = DEVICE_CONNECT_SERVER_FAIL;

        if (user_esp_platform_reset_mode()) {
            return;
        }
    }
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
    os_timer_arm(&client_timer, 1000, 0);
}

/******************************************************************************
 * FunctionName : user_esp_platform_connect_cb
 * Description  : A new incoming connection has been connected.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_connect_cb\n");
    if (wifi_get_opmode() ==  STATIONAP_MODE ) {
        wifi_set_opmode(STATION_MODE);
    }


    //user_link_led_timer_done();

    device_recon_count = 0;
    espconn_regist_recvcb(pespconn, user_esp_platform_recv_cb);
    espconn_regist_sentcb(pespconn, user_esp_platform_sent_cb);
    user_esp_platform_sent(pespconn);
}

/******************************************************************************
 * FunctionName : user_esp_platform_connect
 * Description  : The function given as the connect with the host
 * Parameters   : espconn -- the espconn used to connect the connection
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_connect(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_connect\n");

#ifdef CLIENT_SSL_ENABLE
    espconn_secure_connect(pespconn);
#else
    espconn_connect(pespconn);
#endif
}

#ifdef USE_DNS
/******************************************************************************
 * FunctionName : user_esp_platform_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;

    if (ipaddr == NULL){

    	ESP_DBG("user_esp_platform_dns_found NULL\n");
        if (++device_recon_count == 5) {
            device_status = DEVICE_CONNECT_SERVER_FAIL;
            user_esp_platform_reset_mode();
        }
        return;
    }

    ESP_DBG("user_esp_platform_dns_found %d.%d.%d.%d\n",
            *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
            *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));

    if (esp_server_ip.addr == 0 && ipaddr->addr != 0) {
        os_timer_disarm(&client_timer);
        esp_server_ip.addr = ipaddr->addr;
        os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
        pespconn->proto.tcp->local_port = espconn_port();
        pespconn->proto.tcp->remote_port = 8000;
        ping_status = 1;
        espconn_regist_connectcb(pespconn, user_esp_platform_connect_cb);
        espconn_regist_disconcb(pespconn, user_esp_platform_discon_cb);
        espconn_regist_reconcb(pespconn, user_esp_platform_recon_cb);
        user_esp_platform_connect(pespconn);
    }
}

/******************************************************************************
 * FunctionName : user_esp_platform_dns_check_cb
 * Description  : 1s time callback to check dns found
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_dns_check_cb(void *arg)
{
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_dns_check_cb\n");

    espconn_gethostbyname(pespconn, ESP_DOMAIN, &esp_server_ip, user_esp_platform_dns_found);

    os_timer_arm(&client_timer, 1000, 0);
}

LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_start_dns(struct espconn *pespconn)
{
    esp_server_ip.addr = 0;
    espconn_gethostbyname(pespconn, ESP_DOMAIN, &esp_server_ip, user_esp_platform_dns_found);

    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_dns_check_cb, pespconn);
    os_timer_arm(&client_timer, 1000, 0);
}
#endif


/******************************************************************************
 * FunctionName : user_esp_platform_check_ip
 * Description  : espconn struct parame init when get ip addr
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_check_ip(uint8 reset_flag)
{
    struct ip_info ipconfig;

    os_timer_disarm(&client_timer);

    wifi_get_ip_info(STATION_IF, &ipconfig);

    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {

        //user_link_led_timer_init();

        user_conn.proto.tcp = &user_tcp;
        user_conn.type = ESPCONN_TCP;
        user_conn.state = ESPCONN_NONE;

        device_status = DEVICE_CONNECTING;

        if (reset_flag) {
            device_recon_count = 0;
        }

#if (PLUG_DEVICE || LIGHT_DEVICE)
        os_timer_disarm(&beacon_timer);
        os_timer_setfn(&beacon_timer, (os_timer_func_t *)user_esp_platform_sent_beacon, &user_conn);
#endif

#ifdef USE_DNS
        user_esp_platform_start_dns(&user_conn);
#else
        const char esp_server_ip[4] = {114, 215, 177, 97};

        os_memcpy(user_conn.proto.tcp->remote_ip, esp_server_ip, 4);
        user_conn.proto.tcp->local_port = espconn_port();

#ifdef CLIENT_SSL_ENABLE
        user_conn.proto.tcp->remote_port = 8443;
#else
        user_conn.proto.tcp->remote_port = 8000;
#endif

        espconn_regist_connectcb(&user_conn, user_esp_platform_connect_cb);
        espconn_regist_reconcb(&user_conn, user_esp_platform_recon_cb);
        user_esp_platform_connect(&user_conn);
#endif
    } else {
        /* if there are wrong while connecting to some AP, then reset mode */
        if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
                wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
                wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) {
            user_esp_platform_reset_mode();
        } else {
            os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
            os_timer_arm(&client_timer, 100, 0);
        }
    }
}

/******************************************************************************
 * FunctionName : user_esp_platform_init
 * Description  : device parame init based on espressif platform
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_init(void)
{

	os_sprintf(iot_version,"%s%d.%d.%dt%d(%s)",VERSION_TYPE,IOT_VERSION_MAJOR,\
	IOT_VERSION_MINOR,IOT_VERSION_REVISION,device_type,UPGRADE_FALG);
	os_printf("IOT VERSION = %s\n",iot_version);

	system_param_load(ESP_PARAM_START_SEC, 0, &esp_param, sizeof(esp_param));

	struct rst_info *rtc_info = system_get_rst_info();

	os_printf("reset reason: %x\n", rtc_info->reason);

	if (rtc_info->reason == REASON_WDT_RST ||
		rtc_info->reason == REASON_EXCEPTION_RST ||
		rtc_info->reason == REASON_SOFT_WDT_RST) {
		if (rtc_info->reason == REASON_EXCEPTION_RST) {
			os_printf("Fatal exception (%d):\n", rtc_info->exccause);
		}
		os_printf("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
				rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);
	}

	/***add by tzx for saving ip_info to avoid dhcp_client start****/
    struct dhcp_client_info dhcp_info;
    struct ip_info sta_info;
    system_rtc_mem_read(64,&dhcp_info,sizeof(struct dhcp_client_info));
	if(dhcp_info.flag == 0x01 ) {
		if (true == wifi_station_dhcpc_status())
		{
			wifi_station_dhcpc_stop();
		}
		sta_info.ip = dhcp_info.ip_addr;
		sta_info.gw = dhcp_info.gw;
		sta_info.netmask = dhcp_info.netmask;
		if ( true != wifi_set_ip_info(STATION_IF,&sta_info)) {
			os_printf("set default ip wrong\n");
		}
	}
    os_memset(&dhcp_info,0,sizeof(struct dhcp_client_info));
    system_rtc_mem_write(64,&dhcp_info,sizeof(struct rst_info));



    wifi_station_ap_number_set(AP_CACHE_NUMBER);




    if (esp_param.activeflag != 1) {


        wifi_set_opmode(STATIONAP_MODE);
    }

    user_plug_init();

    if (wifi_get_opmode() != SOFTAP_MODE) {
        os_timer_disarm(&client_timer);
        os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, 1);
        os_timer_arm(&client_timer, 100, 0);
    }
}

