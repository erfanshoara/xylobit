#include "xylobit_speaker.h"


//
// var
//

octv_t		SPEAKER_NUM_OCTAVES = 8;
uint32_t	SPEAKER_LST_FREQ[][12] =
{
	{  16,   17,   18,   19,   21,   22,   23,   25,   27,   29,   31,   33},
	{  32,   34,   36,   38,   41,   43,   46,   49,   51,   55,   58,   62},
	{  65,   69,   73,   77,   82,   87,   92,   98,  104,  110,  117,  123},
	{ 131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247},

	{ 262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494},
	{ 523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988},
	{1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976},
	{2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951},

	{4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902},
	{8372, 8870, 9397, 9956} // incomplete - don't need higher than this
};

uint8_t			SPEAKER_BPM			= 60;
const uint32_t	SPEAKER_FREQ_INIT	= 440;
bool			SPEAKER_IS_REST		= true;
bool			SPEAKER_IS_STOP		= true;

ledc_timer_config_t SPEAKER_PWM_CONF_TIMER =
{
	.speed_mode			= SPEAKER_PWM_MODE,
	.duty_resolution	= SPEAKER_PWM_RES_BIT,
	.timer_num			= SPEAKER_PWM_TIMER,
	.freq_hz			= SPEAKER_FREQ_INIT,
	.clk_cfg			= SPEAKER_PWM_CLK
};

ledc_channel_config_t SPEAKER_PWM_CONF_CHANNEL  =
{
	.gpio_num			= SPEAKER_PIN,
	.speed_mode			= SPEAKER_PWM_MODE,
	.channel			= SPEAKER_PWM_CHANNEL,
	.intr_type			= SPEAKER_PWM_INTR,
	.timer_sel			= SPEAKER_PWM_TIMER,
	.duty				= SPEAKER_PWM_DC0,
	.hpoint				= 0x0
};

char*	SPEAKER_LST_TONE_NAME[13] =
{
	"C ",
	"C#",
	"D ",
	"D#",
	"E ",
	"F ",
	"F#",
	"G ",
	"G#",
	"A ",
	"A#",
	"B ",
	"__"
};

//
// func
//

int
speaker_open()
{
	//
	// main
	//
	
	if (SPEAKER_IS_STOP)
	{
		// it will terminate the program if not ESP_OK
		ESP_ERROR_CHECK(ledc_timer_config(&SPEAKER_PWM_CONF_TIMER));

		// the initial freq and DC is whatever the CONF_CHANNEL is initially
		// set to
		ESP_ERROR_CHECK(ledc_channel_config(&SPEAKER_PWM_CONF_CHANNEL));

		SPEAKER_IS_STOP = false;
	}


	return 0;
}


int
speaker_close()
{
	//
	// var
	//
	
	const uint32_t duty_cycle_ending = 0;
	const uint32_t idle_level_ending = 0;

	// main //
	if (!SPEAKER_IS_STOP)
	{
		ESP_ERROR_CHECK(ledc_set_duty(
					SPEAKER_PWM_MODE,
					SPEAKER_PWM_CHANNEL,
					duty_cycle_ending
					));

		ESP_ERROR_CHECK(ledc_update_duty(
					SPEAKER_PWM_MODE,
					SPEAKER_PWM_CHANNEL
					));

		ESP_ERROR_CHECK(ledc_stop(
					SPEAKER_PWM_MODE,
					SPEAKER_PWM_CHANNEL,
					idle_level_ending
					));

		SPEAKER_IS_STOP = true;
	}


	return 0;
}


int
speaker_play_single_note(
		Note* note_to_play
		)
{
	//
	// var
	//

	//char* task_local = "speaker_play_single";
	//printf("--->Playing(%ld, %d, %d)\n",
	//		SPEAKER_LST_FREQ[note_to_play.octave][note_to_play.tone],
	//		note_to_play.octave,
	//		note_to_play.tone
	//		);
	

	//
	// main
	//
	
	//ESP_LOGW(task_local, "changing the freq to %ld",
	//		SPEAKER_LST_FREQ[note_to_play->octave][note_to_play->tone]
	//		);

	ESP_ERROR_CHECK(ledc_set_freq(
				SPEAKER_PWM_MODE,
				SPEAKER_PWM_TIMER,
				SPEAKER_LST_FREQ[note_to_play->octave][note_to_play->tone]
				));

	if (SPEAKER_IS_REST)
	{
		//ESP_LOGE(task_local, "it was in reset and now changing the dc");
		ESP_ERROR_CHECK(ledc_set_duty(
					SPEAKER_PWM_MODE,
					SPEAKER_PWM_CHANNEL,
					SPEAKER_PWM_DC50
					));

		ESP_ERROR_CHECK(ledc_update_duty(
					SPEAKER_PWM_MODE,
					SPEAKER_PWM_CHANNEL
					));

		SPEAKER_IS_REST = false;
	}


	return 0;
}


int
speaker_play_single_rest()
{
	//char* task_local = "single_reset";
	//
	// main
	//

	if (!SPEAKER_IS_REST)
	{
		//ESP_LOGE(task_local, "IN RESET");
		ESP_ERROR_CHECK(ledc_set_duty(
					SPEAKER_PWM_MODE,
					SPEAKER_PWM_CHANNEL,
					SPEAKER_PWM_DC0
					));

		ESP_ERROR_CHECK(ledc_update_duty(
					SPEAKER_PWM_MODE,
					SPEAKER_PWM_CHANNEL
					));

		SPEAKER_IS_REST = true;
	}


	return 0;
}

// need to config:
// 			INCLUDE_vTaskDelay = 1
// 	check out xTaskDelayUntil as an alternative
int
speaker_play_series_note(
		size_t		num_note,
		Note		lst_note[num_note],
		beat_t		lst_beat[num_note]
		)
{
	//
	// var //
	//
	
	size_t _i = 0;
	//char* task_local = "speaker_play_series";

	// main //
	for (_i = 0; _i < num_note; _i++)
	{
		//printf("in series player: <%d><%d, %d, %d, %d>\n",
		//		_i,
		//		lst_note[_i].octave,
		//		lst_note[_i].tone,
		//		lst_note[_i].is_reset,
		//		lst_beat[_i]
		//	  );
		if (
				(lst_beat[_i]			<  SPEAKER_BEAT_MIN	) ||
				(lst_beat[_i]			>  SPEAKER_BEAT_MAX	)
		   )
		{
			// err //
			return -1;
		}

		if (lst_note[_i].is_reset)
		{
			//ESP_LOGE(task_local, "<there is s a rest!!\n");
			speaker_play_single_rest();
		}
		else
		{
			if (
					(lst_note[_i].octave	>= SPEAKER_NUM_OCTAVES	) ||
					(lst_note[_i].tone		>= 12					)
			   )
			{
				// err //
				return -1;
			}

			speaker_play_single_note(lst_note + _i);
			//ESP_LOGI(task_local, "Playing a note using single note!\n");
		}

		// pause for the beat
		//speaker_pause_beat(lst_beat[_i]);
		speaker_pause_beat(1);
	}

	return 0;
}


int
speaker_pause_beat(
		beat_t	beat_in
		)
{
	//
	// main
	//
	
	vTaskDelay(SPEAKER_MS_PER_BEAT(SPEAKER_BPM, beat_in) / portTICK_PERIOD_MS);
	
	return 0;
}


int speaker_update_rest_status()
{
	//
	// var //
	//
	
	uint32_t dutycycle_new = 0;

	//
	// main //
	//
	
	dutycycle_new = ledc_get_duty(SPEAKER_PWM_MODE, SPEAKER_PWM_CHANNEL);

	if (dutycycle_new == SPEAKER_PWM_DC0)
	{
		SPEAKER_IS_REST = true;
	}
	else if (dutycycle_new == SPEAKER_PWM_DC50)
	{
		SPEAKER_IS_REST = false;
	}
	else
	{
		// err //
		return -1;
	}
	
	return 0;
}

