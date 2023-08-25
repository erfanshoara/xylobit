idf_component_register(SRCS "xylobit_storage.c"
                    INCLUDE_DIRS "./include"
					REQUIRES 
					"sdmmc"
					"driver"
					"fatfs"

					"xylobit_uti"
					)
