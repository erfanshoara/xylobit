#ifndef _XYLOBIT_UTI_H
#define _XYLOBIT_UTI_H

//
// lib //
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "esp_log.h"
#include "esp_err.h"


//
// def //
//


//
// type //
//


//
// var //
//

extern int UTI_ERROR;

//
// func //
//

int
uti_copy_str_len(
		const char*	str_from,
		char*		str_to,
		size_t*		len_str,
		bool		do_null_term
		);


int
uti_alloc_2d(
		void***	lst2d_alloc,
		size_t	size_unit,
		size_t	len_d1,
		size_t	len_d2
		);


int
uti_free_2d(
		void***	lst2d_alloc,
		size_t	len_d2
		);


int
uti_check_err(
		int err
		);


#endif
