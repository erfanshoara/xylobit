idf_component_register(SRCS "xylobit_apwifi.c"
                    INCLUDE_DIRS "./include"
					REQUIRES 
					"esp_wifi"
					"nvs_flash"
					)
