#ifndef _XYLOBIT_STORAGE_H
#define _XYLOBIT_STORAGE_H

//
// lib //
//

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#include "xylobit_uti.h"

//
// def //
//

#define STORAGE_MOUNT_POINT "/xylobit"

#define STORAGE_PIN_D0	40
#define STORAGE_PIN_CLK	39
#define STORAGE_PIN_CMD	38

#define STORAGE_SLOT_WIDTH	1
#define STORAGE_SDMMC_FLAG	SDMMC_HOST_FLAG_1BIT
#define STORAGE_DO_MOUNT	false

#define STORAGE_VFS_MAX_FILES	5
// 16384 = 16 * 1024
#define STORAGE_VFS_UNIT_SIZE	16384

//
// type //
//


//
// var //
//

extern sdmmc_card_t* STORAGE_SDCARD;

//
// func //
//

int
storage_open();


int
storage_close();


//int
//storage_get_info();


/*
 * this is an alloc function and the caller must free the memory
 * on the successful return.
 * on failure, the calle will free the memory if already allocated.
 *
 * the allocation is only for str_content,
 *
 * in:
 * 	[in]  path_file:		str holding the name of the file
 * 	[out] str_content:		the pointer to the content
 * 							caller must free this on successful return
 * 	[io]  len_file_content:	as [in], this variable tells callee to how
 * 							fur in the file it should read.
 * 							as [out], this will tell the caller the length of
 * 							str_content on successful return
 */
int
storage_alloc_read(
		const char*	path_file,
		char**		str_content,
		size_t*		len_str_content
		);


/*
 */
int
storage_write(
		const char*		path_file,
		const char*		str_content,
		const size_t	len_str_content
		);


/*
 * this will only get enteries up to the number indicated by len_lst_ent, and
 * will return when reached to the limit even if there are more enteries.
 *
 * this will update the value at len_lst_ent to the new len of the values
 * - if less enteries exist.
 *
 * xlen_str_ent is to limit the len of the name of the entries, if they are
 * longer, the name will get cut up to the char indicated by xlen_str_ent
 */
int
storage_list_dir(
		const char*	path_dir,
		uint16_t*	len_lst_ent,
		size_t		xlen_str_ent,
		char**		lst_ent
		);


int
storage_alloc_read_bin(
		const char*		path_file,
		uint8_t**		buf_content,
		size_t*			len_buf_content
		);


int
storage_write_bin(
		const char*			path_file,
		const uint8_t*		buf_content,
		const size_t		len_buf_content
		);


#endif
