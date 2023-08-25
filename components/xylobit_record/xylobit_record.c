#include "xylobit_record.h"


//
// var
//

uint8_t		RECORD_NUM_FILE = 0;

char**		RECORD_LST_FILE = NULL;
char*		RECORD_FILE_CURRENT = NULL;

bool		RECORD_IS_OPEN = false;

bool		RECORD_KEEP_ORG_OCTV = true;


//
// func
//

int
record_open()
{
	//
	// var
	//

	int _ret = 0;


	//
	// main
	//
	
	if (RECORD_IS_OPEN)
	{
		return 38;
	}

	// alloc lst_rf
	_ret = uti_alloc_2d(
			(void***)(&RECORD_LST_FILE),
			1,
			RECORD_XLEN_RF_NAME,
			RECORD_MAX_FILE
			);
	
	if (_ret != 0)
	{
		return 6;
	}

	// attempt to open the /record dir

	_ret = record_update_list();

	if (_ret == 1)
	{
		// there was an error opening the record dir
		// atte,pt to mkdir
		_ret = mkdir(
				RECORD_DIR,
				RECORD_DIR_MODE
				);

		if (_ret != 0)
		{
			uti_free_2d(
					(void***)(&RECORD_LST_FILE),
					RECORD_MAX_FILE
					);

			// error //
			// error creating the dir
			return 3;
		}

		_ret = record_update_list();

		if (_ret != 0)
		{
			uti_free_2d(
					(void***)(&RECORD_LST_FILE),
					RECORD_MAX_FILE
					);

			// error //
			return 5;
		}

	}
	else if (_ret != 0)
	{
		uti_free_2d(
				(void***)(&RECORD_LST_FILE),
				RECORD_MAX_FILE
				);

		return 4;
	}


	RECORD_IS_OPEN = true;

	return 0;
}


int
record_close()
{
	//
	// main
	//
	
	if (!RECORD_IS_OPEN)
	{
		return 39;
	}

	uti_free_2d(
			(void***)(&RECORD_LST_FILE),
			RECORD_MAX_FILE
			);


	RECORD_IS_OPEN = false;

	return 0;
}


int
record_update_list()
{
	//
	// var
	//
	
	//int _i = 0;
	int _ret = 0;

	uint16_t _record_num_file = RECORD_MAX_FILE;


	//
	// main
	//
	
	_ret = storage_list_dir(
			RECORD_DIR,
			&_record_num_file,
			RECORD_XLEN_RF_NAME,
			RECORD_LST_FILE
			);

	if (_ret == 21)
	{
		// error //
		// issue opening the record directory
		// *** DON't CHANGE THIS RETURN VALUE 
		return 1;
	}
	else if (_ret != 0)
	{
		// error
		// other error
		return 2;
	}
	else
	{
		RECORD_NUM_FILE = _record_num_file;
	}

	RECORD_FILE_CURRENT = NULL;


	//for (_i = 0; _i < RECORD_NUM_FILE; _i++)
	//{
	//	printf("rec_up file:<%s>\n", RECORD_LST_FILE[_i]);
	//}

	return 0;
}


int
record_add(
		record_file_t* rf_new
		)
{
	//
	// var
	//
	
	int _i = 0;
	int _j = 0;
	int _ret = 0;

	uint8_t*	buf_content = NULL;
	char*		path_file = NULL;

	size_t _len_path	= sizeof(RECORD_DIR) + 1;
	size_t _len_name	= RECORD_XLEN_RF_NAME + 1;

	size_t _len_file	= (
			RECORD_RF_SIZE_HEADER						+
			(RECORD_RF_SIZE_NOTE * rf_new->len_lst_note)
			);


	//
	// main
	//
	
	// creating a local str of the path_file = record_dir + name_file
	//
	path_file = (char*)calloc(
			(_len_path + _len_name),
			1
			);

	if (path_file == NULL)
	{
		return 21;
	}

	_ret = uti_copy_str_len(
			RECORD_DIR"/",
			path_file,
			&_len_path,
			true
			);
			
	if (_ret != 0)
	{
		free(path_file);
		return 22;
	}

	//printf("from record_add file:<%s>\n", rf_new->name_file);

	_ret = uti_copy_str_len(
			rf_new->name_file,
			(path_file + _len_path),
			&_len_name,
			true
			);
			
	if (_ret != 0)
	{
		free(path_file);
		return 23;
	}

	// buffer allocation //
	//
	buf_content = calloc(_len_file, 1);

	if (buf_content == NULL)
	{
		free(path_file);
		return 24;
	}

	// adding the header //
	//
	_len_name = RECORD_XLEN_RF_NAME;

	// adding the file_name
	_ret = uti_copy_str_len(
			rf_new->name_file,
			(char*)buf_content,
			&_len_name,
			true
			);

	if (_ret != 0)
	{
		free(buf_content);
		free(path_file);
		return 24;
	}

	// adding the header_len
	*((size_t*)(buf_content + RECORD_XLEN_RF_NAME)) = rf_new->len_lst_note;

	// adding the body //
	//

	_j = 0;
	for (_i = RECORD_RF_SIZE_HEADER; _i < _len_file; _i += RECORD_RF_SIZE_NOTE)
	{
		// checking tone and reset bool
		if (rf_new->lst_note[_j].tone > 12)
		{
			free(buf_content);
			free(path_file);
			return 29;
		}
		// checking octave
		if (rf_new->lst_note[_j].octave > 9)
		{
			free(buf_content);
			free(path_file);
			return 30;
		}
		// checking beat
		if (rf_new->lst_note[_j].beat > 5)
		{
			free(buf_content);
			free(path_file);
			return 31;
		}

		// tone
		if (rf_new->lst_note[_j].is_reset)
		{
			// 0x0C = 12
			buf_content[_i] |= 0x0C;
		}
		else
		{
			//printf("new note adding<%d>%d>\n",
			//		rf_new->lst_note[_j].tone,
			//		rf_new->lst_note[_j].octave
			//	  );
			buf_content[_i] = (uint8_t)(rf_new->lst_note[_j].tone) & 0x0F;
		}

		// octave
		buf_content[_i] |=	(uint8_t)(rf_new->lst_note[_j].octave)	<< 4;

		// beat - quite unnecessary
		buf_content[_i + 1] = (uint8_t)(rf_new->lst_note[_j].beat);

		_j++;
	}

	// writing to the file //
	//
	//printf("from record_add file2:<%s>\n", path_file);
	_ret = storage_write_bin(
			path_file,
			buf_content,
			_len_file
			);

	if (_ret != 0)
	{
		free(buf_content);
		free(path_file);

		return 40;
	}

	// ending
	//
	free(buf_content);
	free(path_file);

	return 0;
}


int
record_alloc_load(
		record_file_t* rf_new
		)
{
	//
	// var
	//
	
	int _i		= 0;
	int _j		= 0;
	int _ret	= 0;

	uint8_t*	buf_content = NULL;
	char*		path_file = NULL;

	size_t _len_path	= sizeof(RECORD_DIR) + 1;
	size_t _len_name	= RECORD_XLEN_RF_NAME + 1;
	size_t _len_header	= RECORD_RF_SIZE_HEADER;
	size_t _len_file	= 0;

	//
	// main
	//
	
	// creating a local str of the path_file = record_dir + name_file
	//
	path_file = (char*)calloc(
			(_len_path + _len_name),
			1
			);

	if (path_file == NULL)
	{
		return 11;
	}

	_ret = uti_copy_str_len(
			RECORD_DIR"/",
			path_file,
			&_len_path,
			true
			);
			
	if (_ret != 0)
	{
		free(path_file);
		return 12;
	}

	_ret = uti_copy_str_len(
			rf_new->name_file,
			(path_file + _len_path),
			&_len_name,
			true
			);
			
	if (_ret != 0)
	{
		free(path_file);
		return 13;
	}

	// reading the header first //
	//
	_ret = storage_alloc_read_bin(
			path_file,
			&buf_content,
			&_len_header
			);

	if (_ret != 0)
	{
		//printf("Error in record_alloc reading file<%d>\n", _ret);
		free(path_file);
		return 14;
	}

	if (_len_header != RECORD_RF_SIZE_HEADER)
	{
		free(buf_content);
		free(path_file);
		return 15;
	}

	// reads header_len
	rf_new->len_lst_note = *((size_t*)(buf_content + RECORD_XLEN_RF_NAME));

	// reading the body
	//
	_len_file =
		(
		 RECORD_RF_SIZE_HEADER							+
		 ((rf_new->len_lst_note) * RECORD_RF_SIZE_NOTE)
		);

	free(buf_content);

	_ret = storage_alloc_read_bin(
			path_file,
			&buf_content,
			&_len_file
			);

	if (_ret != 0)
	{
		free(path_file);
		return 16;
	}

	if (
			_len_file != 
			(
			 RECORD_RF_SIZE_HEADER							+
			 ((rf_new->len_lst_note) * RECORD_RF_SIZE_NOTE)
			)
	   )
	{
		free(buf_content);
		free(path_file);
		return 17;
	}

	// conversion
	//
	rf_new->lst_note = calloc(
			rf_new->len_lst_note,
			sizeof(Note)
			);

	if (rf_new->lst_note == NULL)
	{
		free(buf_content);
		free(path_file);
		return 18;
	}

	_j = 0;
	for (_i = RECORD_RF_SIZE_HEADER; _i < _len_file; _i += RECORD_RF_SIZE_NOTE)
	{
		rf_new->lst_note[_j].octave	= (octv_t)(buf_content[_i] & 0xF0) >> 4;
		rf_new->lst_note[_j].tone	= (tone_t)(buf_content[_i] & 0x0F);
		rf_new->lst_note[_j].beat	= (beat_t)(buf_content[_i + 1]);

		// checking tone and seting the reset bool
		// checking octave
		if (rf_new->lst_note[_j].octave > 8)
		{
			free(rf_new->lst_note);
			free(buf_content);
			free(path_file);
			return 19;
		}

		// checking beat
		if (rf_new->lst_note[_j].beat > 5)
		{
			free(rf_new->lst_note);
			free(buf_content);
			free(path_file);
			return 19;
		}

		// checking the tone
		if (rf_new->lst_note[_j].tone > 12)
		{
			free(rf_new->lst_note);
			free(buf_content);
			free(path_file);
			return 19;
		}


		// adjusting the rest
		if (rf_new->lst_note[_j].tone == 12)
		{
			//rf_new->lst_note[_j].tone = -1;
			rf_new->lst_note[_j].is_reset = true;
		}
		else
		{
			rf_new->lst_note[_j].is_reset = false;
			//printf("read note adding<%d>%d>\n",
			//		rf_new->lst_note[_j].tone,
			//		rf_new->lst_note[_j].octave
			//	  );
		}


		_j++;
	}

	// ending
	//
	free(buf_content);
	free(path_file);


	return 0;
}


int
record_get_unique_name(
		char	name_file[RECORD_XLEN_RF_NAME + 1]
		)
{
	//
	// var
	//
	
	int _i = 0;
	int _j = 0;
	int _ret = 0;

	bool is_unique = true;


	//
	// main
	//
	
	_ret = record_update_list();

	if (_ret != 0)
	{
		return 52;
	}

	if (RECORD_NUM_FILE == RECORD_MAX_FILE)
	{
		// error - keep it to 51
		// indicating the max number of file has reached 
		return 51;
	}

	for (_i = 1; _i <= RECORD_MAX_FILE; _i++)
	{
		snprintf(
				name_file,
				RECORD_XLEN_RF_NAME,
				"%d.RF",
				_i
				);

		is_unique = true;

		for (_j = 0; _j < RECORD_NUM_FILE; _j++)
		{
			_ret = strncmp(
					name_file,
					RECORD_LST_FILE[_j],
					RECORD_XLEN_RF_NAME
					);

			if (_ret == 0)
			{
				is_unique = false;
				break;
			}
		}
		
		if (is_unique)
		{
			return 0;
		}
	}

	name_file[0] = '\0';

	return 53;
}

int
record_alloc_read(
		const char*	name_file_record,
		size_t*		len_buf_file_content,
		uint8_t**	buf_file_content
		)
{
	//
	// var
	//
	
	int _ret	= 0;

	char*		path_file = NULL;

	size_t _len_path	= sizeof(RECORD_DIR) + 1;
	size_t _len_name	= RECORD_XLEN_RF_NAME + 1;

	//
	// main
	//
	
	// creating a local str of the path_file = record_dir + name_file
	//
	path_file = (char*)calloc(
			(_len_path + _len_name),
			1
			);

	if (path_file == NULL)
	{
		return 11;
	}

	_ret = uti_copy_str_len(
			RECORD_DIR"/",
			path_file,
			&_len_path,
			true
			);
			
	if (_ret != 0)
	{
		free(path_file);
		return 12;
	}

	_ret = uti_copy_str_len(
			name_file_record,
			(path_file + _len_path),
			&_len_name,
			true
			);
			
	if (_ret != 0)
	{
		free(path_file);
		return 13;
	}

	// reading the header first //
	//
	_ret = storage_alloc_read_bin(
			path_file,
			buf_file_content,
			len_buf_file_content
			);

	if (_ret != 0)
	{
		free(path_file);
		return 14;
	}

	//
	// ending
	//
	free(path_file);


	return 0;
}


int
record_remove_rf(
		const char*	name_file_record
		)
{
	//
	// var
	//
	
	int _ret	= 0;

	char*		path_file = NULL;

	size_t _len_path	= sizeof(RECORD_DIR) + 1;
	size_t _len_name	= RECORD_XLEN_RF_NAME + 1;

	//
	// main
	//
	
	// creating a local str of the path_file = record_dir + name_file
	//
	path_file = (char*)calloc(
			(_len_path + _len_name),
			1
			);

	if (path_file == NULL)
	{
		return 11;
	}

	_ret = uti_copy_str_len(
			RECORD_DIR"/",
			path_file,
			&_len_path,
			true
			);
			
	if (_ret != 0)
	{
		free(path_file);
		return 12;
	}

	_ret = uti_copy_str_len(
			name_file_record,
			(path_file + _len_path),
			&_len_name,
			true
			);
			
	if (_ret != 0)
	{
		free(path_file);
		return 13;
	}

	// removing the file
	//
	_ret = remove(path_file);

	if (_ret != 0)
	{
		free(path_file);
		return 14;
	}

	//
	// ending
	//

	free(path_file);

	return 0;
}
