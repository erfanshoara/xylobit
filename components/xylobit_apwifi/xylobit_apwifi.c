#include "xylobit_apwifi.h"


//
// var
//

esp_netif_t*		APWIFI_NETIF_AP		= NULL;

bool				APWIFI_IS_OPEN		= false;
char* TAG_APWIFI = "Xylobit_apwifi";

//
// func
//

int
apwifi_open()
{
	if (APWIFI_IS_OPEN)
	{
		return 6;
	}


	//
	// var
	//
	
	esp_err_t _err = ESP_OK;

	wifi_init_config_t	apwifi_init_conf	= WIFI_INIT_CONFIG_DEFAULT();

	wifi_config_t apwifi_conf =
	{
		.ap =
		{
			.ssid			= APWIFI_SSID,
			.ssid_len		= APWIFI_LEN_SSID,
			.channel		= APWIFI_CHANNEL,
			.password		= APWIFI_PASSWD,
			.max_connection	= APWIFI_MAX_CONN,
			.authmode		= APWIFI_AUTH_M,
			.sae_pwe_h2e	= APWIFI_SAE_PWE,
			.pmf_cfg		=
			{
				.required	= true,
			},
		},
	};


	//
	// main
	//

	// nvs init
	// {
	_err = nvs_flash_init();
	if (
			_err == ESP_ERR_NVS_NO_FREE_PAGES		||
			_err == ESP_ERR_NVS_NEW_VERSION_FOUND
			)
	{
		_err = nvs_flash_erase();
		if (_err != ESP_OK)
		{
			return 1;
		}

		_err = nvs_flash_init();
	}

	if (_err != ESP_OK)
	{
		return 2;
	}
	// }

	// it is set true - even if something fails, calling close() will set it 
	// to false
	APWIFI_IS_OPEN = true;

	// netif init
	//
	_err = esp_netif_init();
	if (_err != ESP_OK)
	{
		apwifi_close();
		return 3;
	}

	_err = esp_event_loop_create_default();
	if (_err != ESP_OK)
	{
		apwifi_close();
		return 8;
	}
	///

	//ESP_ERROR_CHECK(esp_event_loop_create_default());
	
	// wifi-ap init
	//
	APWIFI_NETIF_AP = esp_netif_create_default_wifi_ap();

    // Set a custom IP address, netmask, and gateway
	// {
	esp_netif_ip_info_t ip_info;

	esp_netif_str_to_ip4("192.168.1.1",		&(ip_info.ip));
	esp_netif_str_to_ip4("192.168.1.1",		&(ip_info.gw));
	esp_netif_str_to_ip4("255.255.255.0",	&(ip_info.netmask));

    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(APWIFI_NETIF_AP));
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(APWIFI_NETIF_AP));

    ESP_ERROR_CHECK(esp_netif_set_ip_info(APWIFI_NETIF_AP, &ip_info));

    ESP_ERROR_CHECK(esp_netif_dhcps_start(APWIFI_NETIF_AP));
	// }

	_err = esp_wifi_init(&apwifi_init_conf);
	if (_err != ESP_OK)
	{
		apwifi_close();
		return 4;
	}
	
	//esp_event_handler_instance_register();

	_err = esp_wifi_set_mode(WIFI_MODE_AP);
	if (_err != ESP_OK)
	{
		apwifi_close();
		return 5;
	}

	_err = esp_wifi_set_config(WIFI_IF_AP, &apwifi_conf);
	if (_err != ESP_OK)
	{
		apwifi_close();
		return 6;
	}

	_err = esp_wifi_start();
	if (_err != ESP_OK)
	{
		apwifi_close();
		return 7;
	}
	
	ESP_LOGI(TAG_APWIFI, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
			APWIFI_SSID,
			APWIFI_PASSWD,
			APWIFI_CHANNEL
			);


	return 0;
}


int
apwifi_close()
{
	if (!APWIFI_NETIF_AP)
	{
		return 7;
	}


	//
	// var
	//
	
	esp_err_t _err = ESP_OK;


	//
	// main
	//
	
	// destroying wifi-ap
	esp_wifi_stop();
	esp_wifi_deinit();
	esp_netif_destroy_default_wifi(APWIFI_NETIF_AP);

	// destroy netif
	esp_netif_deinit();

	// destroy nvs
	_err = nvs_flash_erase();
	if (_err != ESP_OK)
	{
		nvs_flash_deinit();
	}
	
	APWIFI_IS_OPEN = false;

	return 0;
}
