#include "xylobit_control.h"


//
// var //
//

gpio_num_t CONTROL_LST_PIN[] =
{
	CONTROL_PIN_ONOFF,
	CONTROL_PIN_START,
	CONTROL_PIN_PLAY,
	CONTROL_PIN_MODE,
	CONTROL_PIN_LCD  
};

// it will be initilized in the control_open()
gpio_glitch_filter_handle_t	CONTROL_LST_GFLT_HDL[CONTROL_NUM_PIN];

bool CONTROL_LST_FLAG_TRIG[] =
{
	false,
	false,
	false,
	false,
	false
};

// adc
adc_oneshot_unit_handle_t	CONTROL_ADC_HDL			= NULL;
adc_unit_t					CONTROL_ADC_UNIT_ID		= ADC_UNIT_1;
adc_ulp_mode_t				CONTROL_ADC_ULP_MODE	= ADC_ULP_MODE_DISABLE;
adc_atten_t					CONTROL_ADC_ATTEN		= ADC_ATTEN_DB_11;
adc_channel_t				CONTROL_POT_VOLUME_CHAN = ADC_CHANNEL_3;
adc_channel_t				CONTROL_POT_OCTAVE_CHAN = ADC_CHANNEL_4;

// setting the init values for the controller var
bool				CONTROL_IS_ON			= true;
bool				CONTROL_IS_STARTED		= false;
bool				CONTROL_IS_PLAYING		= false;
control_mode_opr_t	CONTROL_MODE_CURRENT	= CONTROL_MODE_PLAY_LIVE;
control_page_lcd_t	CONTROL_PAGE_CURRENT	= CONTROL_PAGE_LCD_1;
uint8_t				CONTROL_VOLUME_PERC		= 0;
uint8_t				CONTROL_OCTAVE_CURRENT	= 4;

const int CONTROL_TIME_DEBOUNCING = 85;

const char* CONTROL_LST_STR_MODE[] =
{
	"Configuration",
	"Playing Live",
	"Playing Record",
	"Recording"
};

//
// func //
//

void IRAM_ATTR
control_isr_hdl(
		void* arg
		)
{
	CONTROL_LST_FLAG_TRIG[(int)arg] = true;
}


int
control_toggle_controller(
		int code_controller
		)
{
	//
	// main
	//
	
	switch (code_controller)
	{
		case 0:
			CONTROL_IS_ON = !CONTROL_IS_ON;
			break;
		case 1:
			CONTROL_IS_STARTED = !CONTROL_IS_STARTED;
			break;
		case 2:
			CONTROL_IS_PLAYING = !CONTROL_IS_PLAYING;
			break;
		case 3:
			CONTROL_MODE_CURRENT =
				(CONTROL_MODE_CURRENT + 1) % CONTROL_MODE_MAX;
			break;
		case 4:
			CONTROL_PAGE_CURRENT =
				(CONTROL_PAGE_CURRENT + 1) % CONTROL_PAGE_LCD_MAX;
			break;
		default:
			// error //
			return 1;
	}


	return 0;
}


int
control_open()
{
	//
	// var
	//
	
	int _i = 0;
	
	gpio_config_t gpio_conf_default =
	{
		.mode			= GPIO_MODE_INPUT,
		.pull_up_en		= GPIO_PULLUP_DISABLE,
		.pull_down_en	= GPIO_PULLDOWN_DISABLE,
		.intr_type		= GPIO_INTR_NEGEDGE,

	};

	gpio_pin_glitch_filter_config_t gflt_conf_default = { };

	// ADC
	adc_oneshot_unit_init_cfg_t	pot_adc_unit_conf = {
		.unit_id	= CONTROL_ADC_UNIT_ID,
		.ulp_mode	= CONTROL_ADC_ULP_MODE,
	};

	adc_oneshot_chan_cfg_t pot_adc_chan_conf = {
		.bitwidth	= CONTROL_ADC_BITW,
		.atten		= CONTROL_ADC_ATTEN,
	};
	

	//
	// main
	//

	// installing the isr service for gpio - using isr_service() rather than
	// isr_register() in case  there will be different handler for each intr
	ESP_ERROR_CHECK(gpio_install_isr_service(CONTROL_FLAG_INTR));

	// configuring the gpio and intr for all pins
	for (_i = 0; _i < CONTROL_NUM_PIN; _i++)
	{
		// configuring gpio
		gpio_conf_default.pin_bit_mask = 1ULL << CONTROL_LST_PIN[_i];
		ESP_ERROR_CHECK(gpio_config(&gpio_conf_default));

		// configuring the filter for gpio pin
		gflt_conf_default.gpio_num = CONTROL_LST_PIN[_i];
		ESP_ERROR_CHECK(gpio_new_pin_glitch_filter(
					&gflt_conf_default,
					&CONTROL_LST_GFLT_HDL[_i]
					));

		ESP_ERROR_CHECK(gpio_glitch_filter_enable(CONTROL_LST_GFLT_HDL[_i]));

		// adding the intr hdlr
		ESP_ERROR_CHECK(gpio_isr_handler_add(
					CONTROL_LST_PIN[_i],
					control_isr_hdl,
					(void*) _i
					));
	}

	// disabling the pulldown on adc channels
	//gpio_conf_default.pin_bit_mask = 1ULL << GPIO_NUM_5;
	//ESP_ERROR_CHECK(gpio_config(&gpio_conf_default));

	// configuring the adc
	ESP_ERROR_CHECK(adc_oneshot_new_unit(
				&pot_adc_unit_conf,
				&CONTROL_ADC_HDL
				));

	ESP_ERROR_CHECK(adc_oneshot_config_channel(
				CONTROL_ADC_HDL,
				CONTROL_POT_VOLUME_CHAN,
				&pot_adc_chan_conf
				));

	ESP_ERROR_CHECK(adc_oneshot_config_channel(
				CONTROL_ADC_HDL,
				CONTROL_POT_OCTAVE_CHAN,
				&pot_adc_chan_conf
				));

	return 0;
}


int
control_close()
{
	//
	// var
	//
	
	int _i = 0;


	//
	// main
	//
	ESP_ERROR_CHECK(adc_oneshot_del_unit(CONTROL_ADC_HDL));
	
	for (_i = 0; _i < CONTROL_NUM_PIN; _i++)
	{
		// disabling first and deleting the glitch for all pins
		ESP_ERROR_CHECK(gpio_glitch_filter_disable(CONTROL_LST_GFLT_HDL[_i]));
		ESP_ERROR_CHECK(gpio_del_glitch_filter(CONTROL_LST_GFLT_HDL[_i]));

		// disable the intr
		ESP_ERROR_CHECK(gpio_intr_disable(CONTROL_LST_PIN[_i]));

		// reseting the gpio pin
		ESP_ERROR_CHECK(gpio_reset_pin(CONTROL_LST_PIN[_i]));
	}

	// freeing up resources used by isr service
	gpio_uninstall_isr_service();


	return 0;
}



void
control_update_volume(
		uint8_t* new_volume
		)
{
	//
	// var
	//
	
	int		adc_raw_data	= 0;


	//
	// main
	//

	ESP_ERROR_CHECK(adc_oneshot_read(
				CONTROL_ADC_HDL,
				CONTROL_POT_VOLUME_CHAN,
				&adc_raw_data
				));

	*new_volume = CONTROL_CONVERT_VOLUME(adc_raw_data);

}


void
control_update_octave(
		uint8_t* new_octave
		)
{
	//
	// var
	//
	
	int		adc_raw_data	= 0;


	//
	// main
	//

	ESP_ERROR_CHECK(adc_oneshot_read(
				CONTROL_ADC_HDL,
				CONTROL_POT_OCTAVE_CHAN,
				&adc_raw_data
				));

	*new_octave = CONTROL_CONVERT_OCTAVE(adc_raw_data);
	//printf("<%d><%d>\n", adc_raw_data, *new_octave);

}

