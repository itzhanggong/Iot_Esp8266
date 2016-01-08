#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define ESP_PLATFORM        1
#define USE_OPTIMIZE_PRINTF
#define PLUG_DEVICE             1

#define USE_DNS

#ifdef USE_DNS
#define ESP_DOMAIN      "iot.espressif.cn"
#endif

//#define SOFTAP_ENCRYPT

#ifdef SOFTAP_ENCRYPT
#define PASSWORD	"v*%W>L<@i&Nxe!"
#endif


#if PLUG_DEVICE || LIGHT_DEVICE
#define BEACON_TIMEOUT  150000000
#define BEACON_TIME     50000
#endif


#define AP_CACHE_NUMBER    5

#define 	ON	 	0
#define		OFF 	1



#endif

