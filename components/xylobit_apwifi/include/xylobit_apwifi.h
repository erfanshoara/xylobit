#ifndef _XYLOBIT_APWIFI_H
#define _XYLOBIT_APWIFI_H


//
// lib //
//

#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"


//
// def //
//

#define APWIFI_SSID		"Xylobit"
#define APWIFI_PASSWD		"Xylopasswd10!"

#define APWIFI_LEN_SSID	7
#define APWIFI_CHANNEL	1
#define	APWIFI_AUTH_M		WIFI_AUTH_WPA3_PSK
#define	APWIFI_MAX_CONN	3
#define	APWIFI_SAE_PWE	WPA3_SAE_PWE_BOTH


//
// type //
//


//
// var //
//

extern esp_netif_t*		APWIFI_NETIF_AP;

extern bool				APWIFI_IS_OPEN;

extern char* TAG;

//
// func //
//

int
apwifi_open();


int
apwifi_close();


#endif
