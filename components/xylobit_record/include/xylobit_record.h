#ifndef _XYLOBIT_RECORD_H
#define _XYLOBIT_RECORD_H

//
// lib //
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "xylobit_storage.h"
#include "xylobit_speaker.h"
#include "xylobit_uti.h"


//
// def //
//

#define RECORD_DIR	STORAGE_MOUNT_POINT"/record"

#define RECORD_XLEN_RF_NAME	20
#define	RECORD_MAX_FILE		10

#define RECORD_MAX_NOTE		10000

#define RECORD_DIR_MODE		S_IRWXU

// reserved for future uses
#define RECORD_RF_SIZE_HEADER_RES	8
#define RECORD_RF_SIZE_HEADER_LEN	(sizeof(size_t))
// on esp32s3 wroom (32bit) the size of header is:
// 20 + 4 + 8 = 32 bytes
#define RECORD_RF_SIZE_HEADER		RECORD_XLEN_RF_NAME			+ \
									RECORD_RF_SIZE_HEADER_LEN	+ \
									RECORD_RF_SIZE_HEADER_RES

// size of one note:
// octave	+ tone	+	beat	= note
// 4bit		+ 4bit	+	8bit	= 2byte
//
// **note** the 4 bit tone also store whether it's in reset or no - see below
//
//	4bit for tone:
//	[0-11]	for 12 tones
//	12		for reset
//	[13-15]	not used at the moment
//
#define RECORD_RF_SIZE_NOTE		2

//
// type //
//

struct _record_file_t
{
	char	name_file[RECORD_XLEN_RF_NAME + 1];
	Note*	lst_note;
	size_t	len_lst_note;
};
typedef struct _record_file_t record_file_t;


//
// var //
//

extern uint8_t		RECORD_NUM_FILE;

extern char**		RECORD_LST_FILE;
extern char*		RECORD_FILE_CURRENT;

extern bool			RECORD_IS_OPEN;

extern bool			RECORD_KEEP_ORG_OCTV;


//
// func //
//

int
record_open();


int
record_close();


int
record_update_list();


/*
 * it will write the notes in the xylobit_rf format from the lst_note in rf_new
 * to the file in rf_new
 * if file exists - it will overwrite, other wise will create, the file
 */
int
record_add(
		record_file_t* rf_new
		);


int
record_alloc_load(
		record_file_t* rf_new
		);


int
record_get_unique_name(
		char	name_file[RECORD_XLEN_RF_NAME + 1]
		);


int
record_alloc_read(
		const char*	name_file_record,
		size_t*		len_buf_file_content,
		uint8_t**	buf_file_content
		);


int
record_remove_rf(
		const char*	name_file_record
		);


#endif
