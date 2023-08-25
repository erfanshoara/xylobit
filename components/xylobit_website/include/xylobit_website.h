#ifndef _XYLOBIT_WEBSITE_H
#define _XYLOBIT_WEBSITE_H


//
// lib //
//

#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"

#include "xylobit_apwifi.h"
#include "xylobit_storage.h"
#include "xylobit_uti.h"

//
// def //
//

#define WEBSITE_SERVER_PORT		80
#define WEBSITE_MAX_SOCKET		5
#define WEBSITE_LRU_PURGE		true
#define WEBSITE_TASK_PRIORITY	3
#define WEBSITE_STACK_SIZE		20000

#define WEBSITE_DIR	STORAGE_MOUNT_POINT"/website"

//
// type //
//

struct _website_syringe_t
{
	const char*	str_value;
	size_t		len_str_value;
};
typedef struct _website_syringe_t website_syringe_t;

struct _website_post_val_t
{
	size_t	len_str_name;
	char*	str_name;

	size_t	len_str_value;
	char*	str_value;
};
typedef struct _website_post_val_t website_post_val_t;

//
// var //
//

extern bool				WEBSITE_IS_OPEN;

extern httpd_handle_t	WEBSITE_SERVER;

extern char* TAG_WEBSITE;

//
// func //
//

int
website_open();


int
website_close();


int
website_alloc_inject_syringe(
		const size_t		num_syringe,
		website_syringe_t	lst_syringe[num_syringe],
		const char*			str_org,
		char**				str_new,
		const size_t		len_str_org,
		size_t*				len_str_new
		);


int
website_alloc_fetch_file(
		const size_t		num_syringe,
		website_syringe_t	lst_syringe[num_syringe],
		const char*			path_file,
		char**				str_new,
		size_t*				len_str_new
		);

int
website_extract_post_val(
		size_t				len_str_recv,
		const char*			str_recv,
		size_t*				len_lst_val,
		website_post_val_t	lst_val[*len_lst_val]
		);


int
website_send_chunck(
		httpd_req_t*		req_get,
		size_t				len_buf,
		size_t				xlen_chunck,
		const char*			buf_send
		);


#endif
