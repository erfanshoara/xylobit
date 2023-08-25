#ifndef _XYLOBIT_CONTROL_H
#define _XYLOBIT_CONTROL_H


//
// lib //
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"


//
// def //
//

/*
 * pins used by control library:
 *	CONTROL_PIN_ONOFF	: the switch to turn the device on and off
 *
 *	CONTROL_PIN_START	: the switch used to toggle the status -
 *		different in each mode:
 *			> 
 *
 *	CONTROL_PIN_PLAY	: toggling the playing or pausing status of the device
 *		in each mode (differently):
 *			>
 *
 *	CONTROL_PIN_MODE	: changing the current mode device operating in
 *
 *	CONTROL_PIN_LCD		: used for jumping between different pages of LCD
 *		display - in case there are any
 * 	
 */
#define CONTROL_PIN_ONOFF	GPIO_NUM_6
#define CONTROL_PIN_START	GPIO_NUM_7
#define CONTROL_PIN_PLAY	GPIO_NUM_15
#define CONTROL_PIN_MODE	GPIO_NUM_16
#define CONTROL_PIN_LCD		GPIO_NUM_17

// the number of pushbuttons and switches only - NOT included the pots
#define CONTROL_NUM_PIN		5
// !!! The number of octave should always be synched with the speaker lib !!!
#define CONTROL_NUM_OCTAVE	8

#define CONTROL_FLAG_INTR	0

// ADC values for convert and config
// CONTROL_ADC_BITW is defined to be used in the POT and ADC macros
#define CONTROL_ADC_BITW		ADC_BITWIDTH_12
#define CONTROL_ADC_VALUE_MAX	((1 << CONTROL_ADC_BITW) - 1)

// this margin is used to seperate the values when reading the octave pot
// in discrete
#define CONTROL_POT_OCTAVE_MARGIN \
	((CONTROL_ADC_VALUE_MAX) / ((CONTROL_NUM_OCTAVE - 1) * 2))
/*
 * > desc
 * 	this macro will convert the the digital value read from the volume pot and
 * 	convert it to an int of [0, 100] - representing percentage
 *
 * > in:
 * 	> A: int value with the ADC_VALUE_MAX
 *
 * > out:
 * 	> - : the percentage value
 *
 */
#define CONTROL_CONVERT_VOLUME(A) ((10 * A) / CONTROL_ADC_VALUE_MAX)

/*
 * > desc
 * 	this macro will convert the the digital value read from the octave pot and
 * 	convert it to an int of [0, NUM_OCTAVE-1] - representing index of the octave
 * 	- *** MUST BE SYNCHED WITH SPEAKER LIB ***
 *
 * > in:
 * 	> A: int value with the ADC_VALUE_MAX
 *
 * > out:
 * 	> - : the octave index
 *
 */
#define CONTROL_CONVERT_OCTAVE(A) \
	(((CONTROL_NUM_OCTAVE - 1) * (A + CONTROL_POT_OCTAVE_MARGIN)) / \
	 (CONTROL_ADC_VALUE_MAX))


//
// type //
//

enum _control_mode_opr_t
{
	CONTROL_MODE_CONFIG,
	CONTROL_MODE_PLAY_LIVE,
	CONTROL_MODE_PLAY_RECORD,
	CONTROL_MODE_RECORDING,
	CONTROL_MODE_MAX
};
typedef enum _control_mode_opr_t control_mode_opr_t;

enum _control_lcd_page_t
{
	CONTROL_PAGE_LCD_1,
	CONTROL_PAGE_LCD_2,
	CONTROL_PAGE_LCD_3,
	CONTROL_PAGE_LCD_MAX
};
typedef enum _control_lcd_page_t control_page_lcd_t;


//
// var //
//

// config var for gpio and the gpio_intr
extern gpio_num_t					CONTROL_LST_PIN[CONTROL_NUM_PIN];
extern gpio_glitch_filter_handle_t	CONTROL_LST_GFLT_HDL[CONTROL_NUM_PIN];
extern bool							CONTROL_LST_FLAG_TRIG[CONTROL_NUM_PIN];

//config var for ADC - used for pots
extern adc_oneshot_unit_handle_t	CONTROL_ADC_HDL;
extern adc_unit_t					CONTROL_ADC_UNIT_ID;
extern adc_ulp_mode_t				CONTROL_ADC_ULP_MODE;
extern adc_atten_t					CONTROL_ADC_ATTEN;
extern adc_channel_t				CONTROL_POT_OCTAVE_CHAN;
extern adc_channel_t				CONTROL_POT_VOLUME_CHAN;

extern bool					CONTROL_IS_ON;
extern bool					CONTROL_IS_STARTED;
extern bool					CONTROL_IS_PLAYING;
extern control_mode_opr_t	CONTROL_MODE_CURRENT;
extern control_page_lcd_t	CONTROL_PAGE_CURRENT;
extern uint8_t				CONTROL_VOLUME_PERC;
extern uint8_t				CONTROL_OCTAVE_CURRENT;

extern const int CONTROL_TIME_DEBOUNCING;

extern const char* CONTROL_LST_STR_MODE[];


//
// func //
//

int
control_open();


int
control_close();


// checkout matrix_keyboard for e.g.
//void IRAM_ATTR
//control_isr_hdl(
//		void* arg
//		);


int
control_toggle_controller(
		int code_controller
		);


void
control_update_volume(
		uint8_t* new_volume
		);


void
control_update_octave(
		uint8_t* new_octave
		);


#endif
