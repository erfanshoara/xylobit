#ifndef _XYLOBIT_SPEAKER_H
#define _XYLOBIT_SPEAKER_H


//
// lib
//

#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/ledc.h"


//
// def
//

// default values for timer and channel config
#define SPEAKER_PWM_MODE	LEDC_LOW_SPEED_MODE
#define SPEAKER_PWM_RES_BIT	LEDC_TIMER_13_BIT
#define SPEAKER_PWM_TIMER	LEDC_TIMER_0
#define SPEAKER_PWM_CLK		LEDC_AUTO_CLK
#define SPEAKER_PWM_CHANNEL	LEDC_CHANNEL_0
#define SPEAKER_PWM_INTR	LEDC_INTR_DISABLE

// calculates the 50% duty cyle based on the RES_BIT:
//		((RES_BIT ^ 2) - 1) * 50%
#define SPEAKER_PWM_DC50	((1 << (SPEAKER_PWM_RES_BIT - 1)) - 1)
#define SPEAKER_PWM_DC0		0

#define SPEAKER_BEAT_MIN 1
//#define SPEAKER_BEAT_MAX 128

#define SPEAKER_BPM_MAX		220
#define SPEAKER_BPM_MIN		 20

#define SPEAKER_MS_PER_BEAT(BPM, BEAT)	(240000 / ((BPM) * (2 << BEAT)))

#define SPEAKER_CMP_NOTE(NOTE_1, NOTE_2) \
	((NOTE_1.tone		== NOTE_2.tone)		&& \
	 (NOTE_1.octave		== NOTE_2.octave)	&& \
	 (NOTE_1.is_reset	== NOTE_2.is_reset))

#define SPEAKER_PIN 10


//
// type
//

typedef uint8_t octv_t;
enum _beat_t
{
	SPEAKER_BEAT_4,
	SPEAKER_BEAT_2,
	SPEAKER_BEAT_1,
	SPEAKER_BEAT_1H2,
	SPEAKER_BEAT_1H4,
	SPEAKER_BEAT_1H8,
	SPEAKER_BEAT_MAX
};
typedef uint8_t beat_t;

//enum _octv_t

enum _tone
{
	A,
	Ash,
	B,
	C,
	Csh,
	D,
	Dsh,
	E,
	F,
	Fsh,
	G,
	Gsh
};
typedef enum _tone tone_t;

struct _note
{
	octv_t	octave;
	tone_t	tone;
	beat_t	beat;
	bool	is_reset;
};
typedef struct _note Note;


// var //
extern octv_t	SPEAKER_NUM_OCTAVES;
extern uint32_t SPEAKER_LST_FREQ[][12];

extern uint8_t			SPEAKER_BPM;
extern const uint32_t	SPEAKER_FREQ_INIT;
extern bool				SPEAKER_IS_REST;
extern bool				SPEAKER_IS_STOP;

extern ledc_timer_config_t		SPEAKER_PWM_CONF_TIMER;
extern ledc_channel_config_t	SPEAKER_PWM_CONF_CHANNEL;

extern char*	SPEAKER_LST_TONE_NAME[13];
// func //
// configures timer first then channel
int
speaker_open();
 
 
int
speaker_close();
 

int
speaker_play_single_note(
		Note*	note_to_play
		);
 

int
speaker_play_single_rest();
 

// need to config:
// 			INCLUDE_vTaskDelay = 1
// 	check out xTaskDelayUntil as an alternative
int
speaker_play_series_note(
		size_t		num_note,
		Note		lst_note[num_note],
		beat_t		lst_beat[num_note]
		);

 
int
speaker_pause_beat(
		beat_t	beat_in
		);
 

int
speaker_update_rest_status();
 

#endif
