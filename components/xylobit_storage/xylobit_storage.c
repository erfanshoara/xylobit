#include "xylobit_storage.h"


//
// var
//

sdmmc_card_t* STORAGE_SDCARD;


//
// func
//

int
storage_open()
{
	//
	// var
	//
	
	sdmmc_host_t		sdcard_host = SDMMC_HOST_DEFAULT();
	sdmmc_slot_config_t	slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
	
	sdcard_host.flags = STORAGE_SDMMC_FLAG;
	
	slot_config.width = STORAGE_SLOT_WIDTH;
	
	// Assign GPIO pins for SDMMC interface
	slot_config.d0	= STORAGE_PIN_D0;
	slot_config.clk	= STORAGE_PIN_CLK;
	slot_config.cmd	= STORAGE_PIN_CMD;
	
	esp_vfs_fat_sdmmc_mount_config_t mount_config =
	{
	    .format_if_mount_failed	= STORAGE_DO_MOUNT,
	    .max_files				= STORAGE_VFS_MAX_FILES,
	    .allocation_unit_size	= STORAGE_VFS_UNIT_SIZE
	};


	//
	// main
	//
	
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(
			STORAGE_MOUNT_POINT,
			&sdcard_host,
			&slot_config,
			&mount_config,
			&STORAGE_SDCARD
			);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
			// error //
			/*
			 * Failed to mount filesystem. If you want to format
			 * the card, set the 'format_if_mount_failed' option
			 * to true.
			*/
			return 7;
        }
        else
        {
			// error //
			/*
			 * Failed to initialize the card (%s). Make sure SD card
			 * lines are connected correctly.
			 * esp_err_to_name(ret)
			 */
			return 8;

        }
    }


	return 0;
}


int
storage_close()
{
	//
	// main
	//
	

    esp_vfs_fat_sdcard_unmount(STORAGE_MOUNT_POINT, STORAGE_SDCARD);


	return 0;
}


int
storage_alloc_read(
		const char*	path_file,
		char**		str_content,
		size_t*		len_str_content
		)
{
	//
	// var
	//
	
	FILE*	f_main				= NULL;
	size_t	_len_str_content	= 0;
	int		_ret				= 0;


	//
	// main
	//
	

	f_main = fopen(path_file, "r");
	if (f_main == NULL)
	{
		printf("storage:<%s> error<%d><%s>\n", path_file, errno, strerror(errno));

		// error //
		return 1;
	}


	// finding the actual len of the file //
	
	_ret = fseek(f_main, 0L, SEEK_END);
	if (_ret != 0)
	{
		// error
		return 2;
	}


	_len_str_content = ftell(f_main);
	if (-1 == _len_str_content)
	{
		// error
		return 3;
	}

	_ret = fseek(f_main, 0L, SEEK_SET);
	if (_ret != 0)
	{
		// error
		return 4;
	}


	// checking if the given len is less than the actual size
	if (*len_str_content != 0)
	{
		if (*len_str_content < _len_str_content)
		{
			// if only part of the file is requested by caller
			_len_str_content = *len_str_content;
		}
		else
		{
			// if more tha what's acutally there is requested
			*len_str_content = _len_str_content;
		}
	}
	else
	{
		// if nothing was requested it assumes the entire file is requested
		*len_str_content = _len_str_content;
	}


	// allocate required bytes to store content of the file
	*str_content = calloc(1, *len_str_content);
	if (*str_content == 0)
	{
		fclose(f_main);
		return 5;
	}

	// now read the file
	_ret = fread(
			*str_content,
			1,
			*len_str_content,
			f_main
			);

	if (_ret == 0)
	{
		free(*str_content);
		fclose(f_main);
		return 6;
	}

	// and of course close the file
	fclose(f_main);
	
	// ret //
	return 0;
}


int
storage_write(
		const char*		path_file,
		const char*		str_content,
		const size_t	len_str_content
		)
{
	//
	// var
	//

	FILE*	f_main	= NULL;
	int		_ret	= 0;


	//
	// main
	//
	
	f_main = fopen(path_file, "w");
	if (f_main == NULL)
	{
		// error //
		return 9;
	}
	
	_ret = fwrite(
			str_content,
			1,
			len_str_content,
			f_main
			);
	
	if (_ret != 1)
	{
		fclose(f_main);
		return 11;
	}

	// and of course close the file
	fclose(f_main);


	return 0;
}


int
storage_list_dir(
		const char*	path_dir,
		uint16_t*	len_lst_ent,
		size_t		xlen_str_ent,
		char**		lst_ent
		)
{
	//
	// var
	//
	
	DIR*			dir_main = NULL;
	struct dirent*	dirent_main;

	int		_ret	= 0;
	int		_num	= 0;

	size_t	_xlen_str_ent = xlen_str_ent;


	//
	// main
	//
	
	dir_main = opendir(path_dir);
	if (dir_main == NULL)
	{
		// *** DON'T CHANGE THIS RETURN VALUE
		return 21;
	}

	for (
			_num = 0;
			((dirent_main = readdir(dir_main)) != NULL) &&
				(_num < *len_lst_ent);
			_num++
		)
	{
		_xlen_str_ent = xlen_str_ent;

		printf("list dir cp<%s>\n", dirent_main->d_name);

		_ret = uti_copy_str_len(
				dirent_main->d_name,
				lst_ent[_num],
				&_xlen_str_ent,
				true
				);
		printf("list dir cp<%s>to<%s>\n", dirent_main->d_name, lst_ent[_num]);


		if (_ret != 0)
		{
			closedir(dir_main);
			// error //
			return 15;
		}

		//printf("file:<%s>\n", dirent_main->d_name);
	}

	*len_lst_ent = _num;

	closedir(dir_main);

	return 0;
}


int
storage_alloc_read_bin(
		const char*		path_file,
		uint8_t**		buf_content,
		size_t*			len_buf_content
		)
{
	//
	// var
	//
	
	FILE*	f_main				= NULL;
	size_t	_len_buf_content	= 0;
	int		_ret				= 0;


	//
	// main
	//
	

	f_main = fopen(path_file, "r");
	if (f_main == NULL)
	{
		printf("in read_bin<%d><%s>\npath:<%s>\n",
				errno, strerror(errno), path_file);
		// error //
		return 1;
	}


	// finding the actual len of the file //
	
	_ret = fseek(f_main, 0L, SEEK_END);
	if (_ret != 0)
	{
		// error
		return 2;
	}


	_len_buf_content = ftell(f_main);
	if (-1 == _len_buf_content)
	{
		// error
		return 3;
	}

	_ret = fseek(f_main, 0L, SEEK_SET);
	if (_ret != 0)
	{
		// error
		return 4;
	}


	// checking if the given len is less than the actual size
	if (*len_buf_content != 0)
	{
		if (*len_buf_content < _len_buf_content)
		{
			// if only part of the file is requested by caller
			_len_buf_content = *len_buf_content;
		}
		else
		{
			// if more tha what's acutally there is requested
			*len_buf_content = _len_buf_content;
		}
	}
	else
	{
		// if nothing was requested it assumes the entire file is requested
		*len_buf_content = _len_buf_content;
	}


	// allocate required bytes to store content of the file
	*buf_content = calloc(1, *len_buf_content);
	if (*buf_content == 0)
	{
		fclose(f_main);
		return 5;
	}

	// now read the file
	_ret = fread(
			*buf_content,
			1,
			*len_buf_content,
			f_main
			);

	if (_ret == 0)
	{
		free(*buf_content);
		fclose(f_main);
		return 6;
	}

	// and of course close the file
	fclose(f_main);
	
	// ret //
	return 0;
}


int
storage_write_bin(
		const char*			path_file,
		const uint8_t*		buf_content,
		const size_t		len_buf_content
		)
{
	//
	// var
	//

	FILE*	f_main	= NULL;
	int		_ret	= 0;


	//
	// main
	//
	
	printf("write bin: file:<%s>\n", path_file);

	f_main = fopen(path_file, "w");

	if (f_main == NULL)
	{
		// error //
		return 9;
	}
	
	_ret = fwrite(
			buf_content,
			1,
			len_buf_content,
			f_main
			);
	
	if (_ret == 0)
	{
		fclose(f_main);
		return 11;
	}

	// and of course close the file
	fclose(f_main);


	return 0;
}

int
storage_remove_file(
		char*	path_file
		)
{
	return 0;
}
