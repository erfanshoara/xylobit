#include "xylobit_uti.h"


//
// var
//

int UTI_ERROR = 0;


//
// func
//

/*
 */
int
uti_copy_str_len(
		const char*	str_from,
		char*		str_to,
		size_t*		len_str,
		bool		do_null_term
		)
{
	//
	// var
	//
	
	int _i = 0;


	//
	// main
	//

	if (do_null_term)
	{
		// leaving one char at the end for NULL str_from is longer than len_str
		(*len_str)--;
	}


	for (
			_i = 0;
			(_i < *len_str) && (str_from[_i] != 0);
			_i++
			)
	{
		str_to[_i] = str_from[_i];
		//printf("<%d><%d>str cp from<%c>to<%c>\n",
		//		_i, *len_str, str_to[_i], str_from[_i]);
	}


	if (do_null_term)
	{
		str_to[_i] = 0;
	}

	*len_str = _i;

	return 0;
}


int
uti_alloc_2d(
		void***	lst2d_alloc,
		size_t	size_unit,
		size_t	len_d1,
		size_t	len_d2
		)
{
	//
	// var
	//
	
	int _i = 0;
	int _j = 0;

	// based on the system 64bit, 32bit etc
	const size_t size_pointer = sizeof(char*);


	//
	// main
	//
	
	// d2 alloc
	*lst2d_alloc = calloc(len_d2, size_pointer);

	if (*lst2d_alloc == NULL)
	{
		// error //
		return 2;
	}
	

	for (_i = 0; _i < len_d2; _i++)
	{
		(*lst2d_alloc)[_i] = calloc(len_d1, size_unit);

		if ((*lst2d_alloc)[_i] == NULL)
		{
			for (_j = _i; _j >= 0; _j--)
			{
				free((*lst2d_alloc)[_j]);
			}

			free(*lst2d_alloc);
		}
	}

	return 0;
}


int
uti_free_2d(
		void***	lst2d_alloc,
		size_t	len_d2
		)
{
	//
	// var
	//
	
	int _i = 0;


	//
	// main
	//
	
	for (_i = 0; _i < len_d2; _i++)
	{
		free((*lst2d_alloc)[_i]);
	}

	free(*lst2d_alloc);


	return 0;
}

int
uti_check_err(
		int err
		)
{
	//
	// var
	//
	
	char *task_local = "Xylobit_Error";


	//
	// main
	//
	
	if (err)
	{
		UTI_ERROR = err;
		ESP_LOGE(task_local, ">>>Error[%d]", err);
	}
	

	return err;
}
