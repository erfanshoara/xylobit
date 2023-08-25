#include "xylobit_keyboard.h"


//
// var
//
dedic_gpio_bundle_handle_t KEYBOARD_GPIOD_HDL_OUT	= NULL;
dedic_gpio_bundle_handle_t KEYBOARD_GPIOD_HDL_IN	= NULL;


// func
int
keyboard_conf_gpio(
		size_t		num_gpio,
		const int	lst_gpio[num_gpio],
		uint8_t		code_mode
		)
{
	//
	// var
	//
	int _i = 0;

	gpio_mode_t lst_mode[] =
	{
		GPIO_MODE_INPUT,
		GPIO_MODE_OUTPUT
	};

	gpio_config_t gpio_conf =
	{
		.mode = lst_mode[code_mode],
	};

	//
	// main
	//
	for (_i = 0; _i < num_gpio; _i++)
	{
		gpio_conf.pin_bit_mask = 1ULL << lst_gpio[_i];
		gpio_config(&gpio_conf);
	}

	return 0;
}

int
keyboard_conf_bdl(
		size_t						num_gpio,
		const int					lst_gpio[num_gpio],
		uint8_t						code_mode,
		dedic_gpio_bundle_handle_t*	gpioD_hdl_ret
		)
{
	//
	// var
	//
	dedic_gpio_bundle_config_t gpioD_bdl =
	{
		.gpio_array = lst_gpio,
		.array_size = num_gpio,
		.flags =
		{
			.in_en	= 1 - code_mode,
			.out_en	= code_mode,
		},
	};

	//
	// main
	//
	ESP_ERROR_CHECK(dedic_gpio_new_bundle(&gpioD_bdl, gpioD_hdl_ret));

	return 0;
}

int
keyboard_open()
{
	//
	// var
	//
	int lst_gpio_out[2] =
	{
		KEYBOARD_PIN_CLK,
		KEYBOARD_PIN_SHEN
	};
	int lst_gpio_in[1] =
	{
		KEYBOARD_PIN_DATA
	};
	//
	// main
	//

	// configuring output and input gpio pins
	keyboard_conf_gpio(2, lst_gpio_out, 1);
	keyboard_conf_gpio(1, lst_gpio_in, 0);

	// create one boudle for input pins and boundle for output pins
	keyboard_conf_bdl(2, lst_gpio_out, 1, &KEYBOARD_GPIOD_HDL_OUT);
	keyboard_conf_bdl(1, lst_gpio_in, 0, &KEYBOARD_GPIOD_HDL_IN);

	return 0;
}

int
keyboard_get_keys(
		Note note_played[KEYBOARD_NUM_KEY]
		)
{
	int _i = 0;
	int _key = 0;
	//
	// main
	//
	// loading the channels
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b11, 0b00);
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b01);
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b00);
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b10, 0b10);

	// first clock puls
    if (dedic_gpio_bundle_read_in(KEYBOARD_GPIOD_HDL_IN))
	{

		note_played[_i].tone		= _key;
		note_played[_i].is_reset	= false;
		_i++;
	}
	_key++;

	for (_key = 1; _key < KEYBOARD_NUM_KEY; _key++)
	{
    	dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b01);
    	dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b00);

    	if (dedic_gpio_bundle_read_in(KEYBOARD_GPIOD_HDL_IN))
		{

			note_played[_i].tone = _key;
			note_played[_i].is_reset	= false;
			_i++;
		}
	}

	return 0;
}

int
keyboard_get_key_single_org()
{
	int _key = 1;
	//
	// main
	//
	// loading the channels
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b11, 0b00);
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b01);
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b00);
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b10, 0b10);

	// first clock puls
    if (dedic_gpio_bundle_read_in(KEYBOARD_GPIOD_HDL_IN))
	{
		return 0;
	}

	for (_key = 1; _key < KEYBOARD_NUM_KEY; _key++)
	{
    	dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b01);
    	dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b00);

    	if (dedic_gpio_bundle_read_in(KEYBOARD_GPIOD_HDL_IN))
		{
			return _key;
		}
	}
	return -1;

}


int
keyboard_get_key_single()
{
	int _key = 1;
	//
	// main
	//
	// loading the channels
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b11, 0b00);
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b01);
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b00);
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b10, 0b10);

	// first clock puls
    if (dedic_gpio_bundle_read_in(KEYBOARD_GPIOD_HDL_IN))
	{
		return 7;
	}

	for (_key = 6; _key >= 0; _key--)
	{
    	dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b01);
    	dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b00);

    	if (dedic_gpio_bundle_read_in(KEYBOARD_GPIOD_HDL_IN))
		{
			return _key;
		}
	}

	for (_key = KEYBOARD_NUM_KEY-1; _key > 7; _key--)
	{
    	dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b01);
    	dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b01, 0b00);

    	if (dedic_gpio_bundle_read_in(KEYBOARD_GPIOD_HDL_IN))
		{
			return _key;
		}
	}


	return -1;
}

int
keyboard_close()
{
	int lst_gpio_out[2] =
	{
		KEYBOARD_PIN_CLK,
		KEYBOARD_PIN_SHEN
	};
    dedic_gpio_bundle_write(KEYBOARD_GPIOD_HDL_OUT, 0b11, 0b00);

	dedic_gpio_del_bundle(KEYBOARD_GPIOD_HDL_OUT);
	keyboard_conf_gpio(2, lst_gpio_out, 0);
	
	ESP_ERROR_CHECK(gpio_reset_pin(KEYBOARD_PIN_CLK));
	ESP_ERROR_CHECK(gpio_reset_pin(KEYBOARD_PIN_SHEN));
	ESP_ERROR_CHECK(gpio_reset_pin(KEYBOARD_PIN_DATA));

	return 0;
}
