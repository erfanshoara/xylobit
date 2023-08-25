#include "xylobit_website.h"


//
// var
//

bool				WEBSITE_IS_OPEN		= false;
httpd_handle_t	WEBSITE_SERVER = NULL;
char* TAG_WEBSITE = "xylobit_website";

//
// func
//

int
website_open()
{
	//
	// var
	//
	
    httpd_config_t	conf_httpd	= HTTPD_DEFAULT_CONFIG();
	esp_err_t		_err		= ESP_OK;

    conf_httpd.server_port		= WEBSITE_SERVER_PORT;
    conf_httpd.max_open_sockets	= WEBSITE_MAX_SOCKET;
    conf_httpd.lru_purge_enable	= WEBSITE_LRU_PURGE;

    //conf_httpd.task_priority	= WEBSITE_TASK_PRIORITY;
    //conf_httpd.stack_size		= WEBSITE_STACK_SIZE;

	//
	// main
	//
	
	// checking website library if not created yet
	// {
	// }
	

    // Start the httpd server
	_err = httpd_start(&WEBSITE_SERVER, &conf_httpd);
    if (_err != ESP_OK)
	{
		return 1;
    }


	return 0;
}


int
website_close()
{

	httpd_stop(WEBSITE_SERVER);

	return 0;
}


int
website_alloc_inject_syringe(
		const size_t		num_syringe,
		website_syringe_t	lst_syringe[num_syringe],
		const char*			str_org,
		char**				str_new,
		const size_t		len_str_org,
		size_t*				len_str_new
		)
{
	//
	// var
	//
	
	size_t _i		= 0;
	size_t _j		= 0;
	size_t i_needle	= 0;

	//int _err		= 0;

	bool is_esc		= false;
	*len_str_new	= len_str_org;

	//
	// main
	//
	
	// adding the len of all syringe str to len_str_org for len_str_new
	// and alloc that size for str_new
	// {
	for( _i = 0; _i < num_syringe; _i++)
	{
		*len_str_new += lst_syringe[_i].len_str_value;
	}


	*str_new = calloc(1, *len_str_new);
	if (str_new == NULL)
	{
		// error
		return 11;
	}
	// }

	// main injection
	// * _i takes count of str_org, and _j for str_new
	// {
	_j = 0;
	for(_i = 0; _i < len_str_org; _i++)
	{
		// if str_org has a null it will end
		if (str_org[_i] == 0)
		{
			break;
		}

		// escape 
		if (is_esc)
		{
			(*str_new)[_j] = str_org[_i];
			_j++;
			is_esc = false;
		}
		else
		{
			if (str_org[_i] == '\\')
			{
				is_esc = true;
			}
			else if (str_org[_i] == '{')
			{
				if (
						(str_org[_i+1] == '}') &&
						(i_needle < num_syringe)
				   )
				{
					uti_copy_str_len(
							lst_syringe[i_needle].str_value,
							(*str_new)+_j,
							&(lst_syringe[i_needle].len_str_value),
							false
							);

					_j += lst_syringe[i_needle].len_str_value;
					i_needle++;
					_i++;
				}
				else
				{
					(*str_new)[_j] = '{';
					_j++;
				}
			}
			else
			{
				(*str_new)[_j] = str_org[_i];
				_j++;
			}
		}
	}
	// }

	return 0;
}

int
website_alloc_fetch_file(
		const size_t		num_syringe,
		website_syringe_t	lst_syringe[num_syringe],
		const char*			path_file,
		char**				str_new,
		size_t*				len_str_new
		)
{
	//
	// var
	//
	
	char*	str_file = NULL;
	size_t	len_str_file = 0;
	int		_err = 0;

	//
	// main
	//
	
	_err = storage_alloc_read(
			path_file,
			&str_file,
			&len_str_file
			);

	if (_err != 0)
	{
		// error //
		return 23;
	}


	_err = website_alloc_inject_syringe(
			num_syringe,
			lst_syringe,
			str_file,
			str_new,
			len_str_file,
			len_str_new
			);

	if (_err != 0)
	{
		// error //
		free(str_file);
		return 24;
	}

	// ret //
	free(str_file);
	return 0;
}


int
website_extract_post_val(
		size_t				len_str_recv,
		const char*			str_recv,
		size_t*				len_lst_val,
		website_post_val_t	lst_val[*len_lst_val]
		)
{
	//
	// var
	//
	
	size_t _i		= 0;
	size_t _i_val	= 0;
	size_t _i_str	= 0;

	//int _e = 0;

	bool is_reading_name = true;

	//
	// main
	//
	

	for (_i = 0; (_i < len_str_recv) && (_i_val < *len_lst_val); _i++)
	{
		if (is_reading_name)
		{
			if(str_recv[_i] == '=')
			{
				is_reading_name = false;

				lst_val[_i_val].str_name[_i_str] = '\0';
				_i_str = 0;
			}
			else
			{
				if (_i_str < (lst_val[_i_val].len_str_name -1))
				{
					lst_val[_i_val].str_name[_i_str] = str_recv[_i];

					_i_str++;
				}
				else if (_i_str == (lst_val[_i_val].len_str_name -1))
				{
					lst_val[_i_val].str_name[_i_str] = '\0';
					// no increment here
					// - it will stay on the terminating null
				}
			}
		}
		else
		{
			if(str_recv[_i] == '&')
			{
				is_reading_name = true;

				lst_val[_i_val].str_value[_i_str] = '\0';
				_i_str = 0;

				_i_val++;
			}
			else
			{
				if (_i_str < (lst_val[_i_val].len_str_value -1))
				{
					lst_val[_i_val].str_value[_i_str] = str_recv[_i];

					_i_str++;
				}
				else if (_i_str == (lst_val[_i_val].len_str_value -1))
				{
					lst_val[_i_val].str_value[_i_str] = '\0';
					// no increment here
					// - it will stay on the terminating null
				}
			}
		}
	}

	//
	// end
	//

	*len_lst_val = _i_val + 1;

	return 0;
}


int
website_send_chunck(
		httpd_req_t*		req_get,
		size_t				len_buf,
		size_t				xlen_chunck,
		const char*			buf_send
		)
{
	//
	// var
	//
	
	int _i = 0;
	int _len_chuck = 0;

	esp_err_t _e = ESP_OK;


	//
	// main
	//

	_i = 0;
	_len_chuck = (len_buf > (_i + xlen_chunck)) ? xlen_chunck : (len_buf - _i);
	while (_i < len_buf)
	{
		//printf("from <%d> for <%d>\n", i, j);

		_e = httpd_resp_send_chunk(
				req_get,
				(buf_send + _i),
				_len_chuck
				);

		if (_e != ESP_OK)
		{
			return 45;
		}

		_i += _len_chuck;

		_len_chuck = (len_buf > (_i + xlen_chunck)) ?
			xlen_chunck : (len_buf - _i);
	}


	return 0;
}

