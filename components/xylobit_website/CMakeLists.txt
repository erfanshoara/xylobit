idf_component_register(SRCS "xylobit_website.c"
                    INCLUDE_DIRS "./include"
					REQUIRES 
					"esp_http_server"
					"nvs_flash"

					"xylobit_apwifi"
					"xylobit_storage"
					"xylobit_uti"
					)
