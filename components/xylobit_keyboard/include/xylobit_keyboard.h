#ifndef _XYLOBIT_KEYBOARD_H
#define _XYLOBIT_KEYBOARD_H

//
// lib //
//
//#include <stdint.h>
//#include <stdbool.h>
//
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "driver/dedic_gpio.h"
#include "driver/gpio.h"

#include "xylobit_speaker.h"

//
// def //
//
#define KEYBOARD_NUM_KEY	16
#define KEYBOARD_KEY_OFFSET	9

#define KEYBOARD_PIN_CLK	47
#define KEYBOARD_PIN_SHEN	41
#define KEYBOARD_PIN_DATA	42

#define KEYBOARD_KEY_TO_TONE(KEY) ((KEY) + (KEYBOARD_KEY_OFFSET))

//
// type //
//


// var //
extern dedic_gpio_bundle_handle_t KEYBOARD_GPIOD_HDL_OUT;
extern dedic_gpio_bundle_handle_t KEYBOARD_GPIOD_HDL_IN;


// func //

int
keyboard_open();


/*
code_mode:
	0: input
	1: output
 */
int
keyboard_conf_gpio(
		size_t		num_gpio,
		const int	lst_gpio[num_gpio],
		uint8_t		code_mode
		);


int
keyboard_conf_bdl(
		size_t						num_gpio,
		const int					lst_gpio[num_gpio],
		uint8_t						code_mode,
		dedic_gpio_bundle_handle_t*	gpioD_hdl_ret
		);


int
keyboard_get_keys(
		Note note_played[KEYBOARD_NUM_KEY]
		);


int
keyboard_get_key_single();


int
keyboard_close();


#endif
