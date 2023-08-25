#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_pthread.h"
#include "esp_log.h"
#include "esp_err.h"

#include "xylobit_control.h"
#include "xylobit_speaker.h"
#include "xylobit_keyboard.h"
#include "xylobit_lcd.h"
#include "xylobit_storage.h"
#include "xylobit_record.h"
#include "xylobit_apwifi.h"
#include "xylobit_website.h"


//
// def
//

#define XYLOBIT_NUM_MUTEX 12


//
// var
//

Note XYLOBIT_NOTE_CURRENT = {
	.octave = 4,
	//.octave = CONTROL_OCTAVE_CURRENT,
	.tone = 0,
	.is_reset = true,
};

// each open() pointer should correspond to the clos in the lst_close
// {
int (*XYLOBIT_LST_FUNC_OPEN[]) (void) =
{
	control_open,
	lcd_open,
	keyboard_open,
	speaker_open,
	storage_open,
	record_open,
	apwifi_open,
	website_open
};

int (*XYLOBIT_LST_FUNC_CLOSE[]) (void) =
{
	control_close,
	lcd_close,
	keyboard_close,
	speaker_close,
	storage_close,
	record_close,
	apwifi_close,
	website_close
};
//}

// threads obj
pthread_attr_t	XYLOBIT_THREAD_ATTR;
//pthread_attr_t	XYLOBIT_THREAD_ATTR = NULL;

pthread_t XYLOBIT_THREAD_CONTROLLER;
pthread_t XYLOBIT_THREAD_DISPLAY;

pthread_t XYLOBIT_THREAD_PLAY_LIVE;
pthread_t XYLOBIT_THREAD_PLAY_RECORD;
pthread_t XYLOBIT_THREAD_RECORD;

// mutex
pthread_mutex_t XYLOBIT_MTX_IS_ON;
pthread_mutex_t XYLOBIT_MTX_IS_STARTED;
pthread_mutex_t XYLOBIT_MTX_IS_PLAYING;
pthread_mutex_t XYLOBIT_MTX_MODE_CURRENT;
pthread_mutex_t XYLOBIT_MTX_LCD_PAGE;
pthread_mutex_t XYLOBIT_MTX_OCTAVE;
pthread_mutex_t XYLOBIT_MTX_VOLUME;
pthread_mutex_t XYLOBIT_MTX_NOTE_CURRENT;
pthread_mutex_t XYLOBIT_MTX_BPM;
pthread_mutex_t XYLOBIT_MTX_ERROR;
pthread_mutex_t XYLOBIT_MTX_RF_NAME;
pthread_mutex_t XYLOBIT_MTX_RF_LST;

// the XYLOBIT_NUM_MUTEX must keep the track of this array
pthread_mutex_t* XYLOBIT_LST_MTX[] =
{
	&XYLOBIT_MTX_IS_ON,
	&XYLOBIT_MTX_IS_STARTED,
	&XYLOBIT_MTX_IS_PLAYING,
	&XYLOBIT_MTX_MODE_CURRENT,
	&XYLOBIT_MTX_LCD_PAGE,
	&XYLOBIT_MTX_OCTAVE,
	&XYLOBIT_MTX_VOLUME,
	&XYLOBIT_MTX_NOTE_CURRENT,
	&XYLOBIT_MTX_BPM,
	&XYLOBIT_MTX_ERROR,
	&XYLOBIT_MTX_RF_NAME,
	&XYLOBIT_MTX_RF_LST
};

pthread_mutexattr_t	XYLOBIT_MTX_ATTR_DEFAULT;

// semaphore
sem_t	XYLOBIT_SEM_LCD_UPDATE;

// condition variables & attr
//pthread_cond_t		XYLOBIT_COND_MODE_CHANGE;
//pthread_condattr_t	XYLOBIT_COND_ATTR;


// var - http uri
// {

//httpd_uri_t		XYLOBIT_HTTP_URI_HOME =
//{
//	.uri		= "/",
//	.method		= HTTP_GET,
//	.handler	= xylobit_http_hdl_home,
//	.user_ctx	= NULL
//};
//
//httpd_uri_t		XYLOBIT_HTTP_URI_GET_EDIT =
//{
//	.uri		= "/edit",
//	.method		= HTTP_GET,
//	.handler	= xylobit_http_hdl_get_edit,
//	.user_ctx	= NULL
//};
//
//httpd_uri_t		XYLOBIT_HTTP_URI_POST_EDIT =
//{
//	.uri		= "/edit",
//	.method		= HTTP_POST,
//	.handler	= xylobit_http_hdl_post_edit,
//	.user_ctx	= NULL
//};



// }


//
// func
//

int
xylobit_open()
{
	//
	// var
	//
	
	size_t _i				= 0;
	size_t num_func_open	= 0;
	size_t num_func_close	= 0;


	//
	//main
	//

	num_func_open = sizeof(XYLOBIT_LST_FUNC_OPEN) /
					sizeof(*XYLOBIT_LST_FUNC_OPEN);

	num_func_close = sizeof(XYLOBIT_LST_FUNC_CLOSE) /
					sizeof(*XYLOBIT_LST_FUNC_CLOSE);

	// opening all modules used by xylobit
	for (_i = 0; _i < num_func_open; _i++)
	{
		// if open was not successfull - close those already open and break
		if (uti_check_err(XYLOBIT_LST_FUNC_OPEN[_i]()))
		{
			// inti _i-- is beacsue if a open() fails it doesn't do anything
			// and no need to call the close on it
			for (; _i > 0; _i--)
			{
				XYLOBIT_LST_FUNC_CLOSE[_i-1]();
			}

			return UTI_ERROR;
		}
	}

	// init condition variable & attr
	// {
	//pthread_condattr_init(&XYLOBIT_COND_ATTR);

	//if (uti_check_err(pthread_cond_init(
	//				&XYLOBIT_COND_MODE_CHANGE,
	//				&XYLOBIT_COND_ATTR
	//				)))
	//{
//	//	pthread_condattr_destroy(&XYLOBIT_COND_ATTR);

	//	// close xylobit devices first
	//	for (_i = 0; _i < num_func_close; _i++)
	//	{
	//		uti_check_err(XYLOBIT_LST_FUNC_CLOSE[_i]());
	//	}

	//	return UTI_ERROR;
	//}
	// }

	// semaphore init
	if (uti_check_err(sem_init(&XYLOBIT_SEM_LCD_UPDATE, 0, 1)))
	{
//		//pthread_condattr_destroy(&XYLOBIT_COND_ATTR);
//		//pthread_cond_destroy(&XYLOBIT_COND_MODE_CHANGE);

		// close xylobit devices first
		for (_i = 0; _i < num_func_close; _i++)
		{
			uti_check_err(XYLOBIT_LST_FUNC_CLOSE[_i]());
		}

		return UTI_ERROR;
	}

	// init mutex attr
	if (uti_check_err(pthread_mutexattr_init(&XYLOBIT_MTX_ATTR_DEFAULT)))
	{
		// destroying the semaphore
		sem_destroy(&XYLOBIT_SEM_LCD_UPDATE);

//		pthread_condattr_destroy(&XYLOBIT_COND_ATTR);
//		pthread_cond_destroy(&XYLOBIT_COND_MODE_CHANGE);

		// close xylobit devices first
		for (_i = 0; _i < num_func_close; _i++)
		{
			uti_check_err(XYLOBIT_LST_FUNC_CLOSE[_i]());
		}

		return UTI_ERROR;
	}

	// init thread attr
	if (uti_check_err(pthread_attr_init(&XYLOBIT_THREAD_ATTR)))
	{
		// destroy the mutex attr created previously
		pthread_mutexattr_destroy(&XYLOBIT_MTX_ATTR_DEFAULT);

		// destroying the semaphore
		sem_destroy(&XYLOBIT_SEM_LCD_UPDATE);

//		pthread_condattr_destroy(&XYLOBIT_COND_ATTR);
//		pthread_cond_destroy(&XYLOBIT_COND_MODE_CHANGE);

		// close xylobit devices first
		for (_i = 0; _i < num_func_close; _i++)
		{
			uti_check_err(XYLOBIT_LST_FUNC_CLOSE[_i]());
		}

		return UTI_ERROR;
	}

	// init all mutexes
	for (_i = 0; _i < XYLOBIT_NUM_MUTEX; _i++)
	{
		if (uti_check_err(pthread_mutex_init(
						XYLOBIT_LST_MTX[_i],
						&XYLOBIT_MTX_ATTR_DEFAULT
						)))
		{
			// destroy the mutexes already created
			for (; _i > 0; _i--)
			{
				uti_check_err(pthread_mutex_destroy(XYLOBIT_LST_MTX[_i-1]));
			}

			// destroy the thread attr
			pthread_attr_destroy(&XYLOBIT_THREAD_ATTR);

			// destroy the mutex attr created previously
			pthread_mutexattr_destroy(&XYLOBIT_MTX_ATTR_DEFAULT);

			// destroying the semaphore
			sem_destroy(&XYLOBIT_SEM_LCD_UPDATE);

//			pthread_condattr_destroy(&XYLOBIT_COND_ATTR);
//			pthread_cond_destroy(&XYLOBIT_COND_MODE_CHANGE);

			// close xylobit devices first
			for (_i = 0; _i < num_func_close; _i++)
			{
				uti_check_err(XYLOBIT_LST_FUNC_CLOSE[_i]());
			}

			return UTI_ERROR;
		}
	}


	return 0;
}


int
xylobit_close()
{
	//
	// var
	//
	
	size_t _i				= 0;
	size_t num_func_close	= 0;

	//
	// main
	//

	num_func_close = sizeof(XYLOBIT_LST_FUNC_CLOSE) /
					sizeof(*XYLOBIT_LST_FUNC_CLOSE);

	// destroying the mutexes
	for (_i = 0; _i < XYLOBIT_NUM_MUTEX; _i++)
	{
		uti_check_err(pthread_mutex_destroy(XYLOBIT_LST_MTX[_i]));
	}

	// destroy the thread attr
	uti_check_err(pthread_attr_destroy(&XYLOBIT_THREAD_ATTR));

	// destroy the mutex attr
	uti_check_err(pthread_mutexattr_destroy(&XYLOBIT_MTX_ATTR_DEFAULT));

	// destroying condition variable & attr
	// {
//	pthread_condattr_destroy(&XYLOBIT_COND_ATTR);
//	pthread_cond_destroy(&XYLOBIT_COND_MODE_CHANGE);
	// }
	
	// semaphore
	uti_check_err(sem_destroy(&XYLOBIT_SEM_LCD_UPDATE));
	
	
	// xylobit devices //
	for (_i = 0; _i < num_func_close; _i++)
	{
		//pausing for 1s and stop speaker - not sure if needed
		//vTaskDelay((1000) / portTICK_PERIOD_MS);
		uti_check_err(XYLOBIT_LST_FUNC_CLOSE[_i]());
	}


	return 0;
}


// func - threads
//{

static void*
xylobit_thread_play_live(
		void* arg
		)
{
	//
	// var
	//
	
	int time_delay = 50;
	bool has_stopped	= true;

	Note _note_old = {};

	// local instance of the global var
	pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
	bool _is_on = CONTROL_IS_ON;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

	pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
	bool _is_started = CONTROL_IS_STARTED;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

	pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
	control_mode_opr_t _mode_current = CONTROL_MODE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

	pthread_mutex_lock(&XYLOBIT_MTX_NOTE_CURRENT);
	Note _note_current = XYLOBIT_NOTE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_NOTE_CURRENT);

	//pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
	//octv_t _octave_current = CONTROL_OCTAVE_CURRENT;
	//pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);

	//
	// main
	//
	
	_note_old = _note_current;

	speaker_play_single_rest();

	while (
			_is_on										&&
			(_mode_current == CONTROL_MODE_PLAY_LIVE)
		  )
	{

		pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
		_is_started = CONTROL_IS_STARTED;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

		if (_is_started)
		{
			has_stopped = false;

			// getting the new tone
			_note_current.tone =
				KEYBOARD_KEY_TO_TONE(keyboard_get_key_single());

			// getting the new octave
			pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
			_note_current.octave	= CONTROL_OCTAVE_CURRENT;
			pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);

			// adjusting the note, and set the update if needed
			if (_note_current.tone == KEYBOARD_KEY_TO_TONE(-1))
			{
				_note_current.tone		=	12;
				_note_current.octave	=	0;
				_note_current.is_reset	=	true;
			}
			else
			{
				_note_current.octave	+=	_note_current.tone / 12;
				_note_current.tone		=	_note_current.tone % 12;
				_note_current.is_reset	= 	false;
			}

			// if there needs to be a change in the playing of the speaker
			// 	> octave or tone or is_reset
			if (!SPEAKER_CMP_NOTE(_note_current, _note_old))
			{
				if (_note_current.is_reset)
				{
					speaker_play_single_rest();
				}
				else
				{
					speaker_play_single_note(&_note_current);
				}
				
				pthread_mutex_lock(&XYLOBIT_MTX_NOTE_CURRENT);
				XYLOBIT_NOTE_CURRENT = _note_current;
				pthread_mutex_unlock(&XYLOBIT_MTX_NOTE_CURRENT);

				sem_post(&XYLOBIT_SEM_LCD_UPDATE);
			}

			_note_old = _note_current;
		}
		else
		{
			if (!has_stopped)
			{
				has_stopped = true;

				speaker_play_single_rest();
			}
		}

		vTaskDelay(time_delay / portTICK_PERIOD_MS);

		// updating the local instance of global var
		pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
		_is_on = CONTROL_IS_ON;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

		pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
		_mode_current = CONTROL_MODE_CURRENT;
		pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);
	}

	speaker_play_single_rest();
	return NULL;
}


static void*
xylobit_thread_play_record(
		void* arg
		)
{
	//
	// var
	//
	
	//int _i = 0;
	int _i_note = 0;
	int _ret = 0;
	size_t _xlen_str = RECORD_XLEN_RF_NAME;

	// -1 means no key pressed == is_reset
	//int key_pressed = -1;
	//int key_pressed_old = -1;
	//bool is_new_note = true;
	bool has_stopped = true;

	Note _note_old = {};

	record_file_t rf_playing = {};

	// local instance of the global var
	pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
	bool _is_on = CONTROL_IS_ON;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

	pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
	bool _is_started = CONTROL_IS_STARTED;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

	pthread_mutex_lock(&XYLOBIT_MTX_IS_PLAYING);
	bool _is_playing = CONTROL_IS_PLAYING;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_PLAYING);

	pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
	control_mode_opr_t _mode_current = CONTROL_MODE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

	pthread_mutex_lock(&XYLOBIT_MTX_NOTE_CURRENT);
	Note _note_current = XYLOBIT_NOTE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_NOTE_CURRENT);

	//pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
	//octv_t _octave_current = CONTROL_OCTAVE_CURRENT;
	//pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);

	//
	// main
	//
	
	//RECORD_FILE_CURRENT = "1.RF";

	_note_old = _note_current;

	speaker_play_single_rest();

	while (
			_is_on										&&
			(_mode_current == CONTROL_MODE_PLAY_RECORD)
		  )
	{

		pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
		_is_started = CONTROL_IS_STARTED;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);


		if (_is_started)
		{
			if (has_stopped)
			{
				has_stopped = false;

				// selecting the file
				if (RECORD_NUM_FILE == 0)
				{
					// no recorded file
					pthread_mutex_lock(&XYLOBIT_MTX_ERROR);
					UTI_ERROR = 106;
					pthread_mutex_unlock(&XYLOBIT_MTX_ERROR);

					pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
					CONTROL_MODE_CURRENT = CONTROL_MODE_CONFIG;
					pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

					free(rf_playing.lst_note);
					return NULL;
				}
				else if (RECORD_FILE_CURRENT == NULL)
				{
					// if no file selected,
					// first in the list is selected sutomatically
					RECORD_FILE_CURRENT = RECORD_LST_FILE[0];
				}

				_xlen_str = RECORD_XLEN_RF_NAME;

				uti_copy_str_len(
						RECORD_FILE_CURRENT,
						rf_playing.name_file,
						&_xlen_str,
						true
						);


				// load the new notes
				_ret = record_alloc_load(&rf_playing);
				if (_ret != 0)
				{
					pthread_mutex_lock(&XYLOBIT_MTX_ERROR);
					UTI_ERROR = 107;
					pthread_mutex_unlock(&XYLOBIT_MTX_ERROR);

					pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
					CONTROL_MODE_CURRENT = CONTROL_MODE_CONFIG;
					pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

					free(rf_playing.lst_note);
					return NULL;
				}

				_i_note = 0;
			}


			pthread_mutex_lock(&XYLOBIT_MTX_IS_PLAYING);
			_is_playing = CONTROL_IS_PLAYING;
			pthread_mutex_unlock(&XYLOBIT_MTX_IS_PLAYING);

			if (_is_playing)
			{
				// get note
				_note_current = rf_playing.lst_note[_i_note];

				// get octave if set so
				//if (!RECORD_KEEP_ORG_OCTV)
				//{
				//	pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
				//	_note_current.octave = CONTROL_OCTAVE_CURRENT;
				//	pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);
				//}

				// if there needs to be a change in the playing of the speaker
				// 	> octave or tone or is_reset
				if (!SPEAKER_CMP_NOTE(_note_current, _note_old))
				{
					if (_note_current.is_reset)
					{
						speaker_play_single_rest();
					}
					else
					{
						speaker_play_single_note(&_note_current);
					}
					
					pthread_mutex_lock(&XYLOBIT_MTX_NOTE_CURRENT);
					XYLOBIT_NOTE_CURRENT = _note_current;
					pthread_mutex_unlock(&XYLOBIT_MTX_NOTE_CURRENT);

					sem_post(&XYLOBIT_SEM_LCD_UPDATE);
				}

				_note_old = _note_current;

				_i_note++;

				if (_i_note == rf_playing.len_lst_note)
				{
					// break from the while loop and return
					// main will reset start and playing flag and
					// continue accorsingly
					break;
				}
			}
			else
			{
				vTaskDelay(pdMS_TO_TICKS(500));
			}
		}
		else
		{
			if (!has_stopped)
			{
				has_stopped = true;

				_i_note = 0;

				free(rf_playing.lst_note);
				rf_playing.len_lst_note = 0;
				rf_playing.name_file[0] = '\0';

				speaker_play_single_rest();
			}

			vTaskDelay(pdMS_TO_TICKS(500));
		}


		speaker_pause_beat(SPEAKER_BEAT_1H8);


		// updating the local instance of global var
		pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
		_is_on = CONTROL_IS_ON;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

		pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
		_mode_current = CONTROL_MODE_CURRENT;
		pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);
	}

	free(rf_playing.lst_note);

	speaker_play_single_rest();
	return NULL;
}


//int
static void*
xylobit_thread_record(
		void* arg
		)
{
	//
	// var
	//
	
	//int _i		= 0;
	int _ret	= 0;

	// -1 means no key pressed == is_reset
	//int key_pressed		= -1;
	//int key_pressed_old	= -1;
	//int time_delay		= 50;

	//bool is_new_note	= true;
	bool has_stopped	= true;

	Note _note_old = {};

	record_file_t rf_new =
	{
		.lst_note		= NULL,
		.len_lst_note	= 0
	};

	// local instance of the global var
	pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
	bool _is_on = CONTROL_IS_ON;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

	pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
	bool _is_started = CONTROL_IS_STARTED;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

	pthread_mutex_lock(&XYLOBIT_MTX_IS_PLAYING);
	bool _is_playing = CONTROL_IS_PLAYING;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_PLAYING);

	pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
	control_mode_opr_t _mode_current = CONTROL_MODE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

	pthread_mutex_lock(&XYLOBIT_MTX_NOTE_CURRENT);
	Note _note_current = XYLOBIT_NOTE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_NOTE_CURRENT);

	//pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
	//octv_t _octave_current = CONTROL_OCTAVE_CURRENT;
	//pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);


	//
	// main
	//
	
	// creating the rf_new
	//
	_ret = record_get_unique_name(rf_new.name_file);
	if (_ret != 0)
	{
		pthread_mutex_lock(&XYLOBIT_MTX_ERROR);
		UTI_ERROR = 101;
		pthread_mutex_unlock(&XYLOBIT_MTX_ERROR);

		pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
		CONTROL_MODE_CURRENT = CONTROL_MODE_CONFIG;
		pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

		return NULL;
		//return 201;
	}
	

	rf_new.lst_note = calloc(
			RECORD_MAX_NOTE,
			sizeof(Note)
			);

	if (rf_new.lst_note == NULL)
	{
		pthread_mutex_lock(&XYLOBIT_MTX_ERROR);
		UTI_ERROR = 102;
		pthread_mutex_unlock(&XYLOBIT_MTX_ERROR);

		pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
		CONTROL_MODE_CURRENT = CONTROL_MODE_CONFIG;
		pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

		return NULL;
	}
	///

	_note_old = _note_current;

	speaker_play_single_rest();


	while (
			_is_on										&&
			(_mode_current == CONTROL_MODE_RECORDING)
		  )
	{

		pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
		_is_started =	CONTROL_IS_STARTED;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

		if (_is_started)
		{
			has_stopped = false;

			// getting the new tone
			_note_current.tone =
				KEYBOARD_KEY_TO_TONE(keyboard_get_key_single());

			// getting the new octave
			pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
			_note_current.octave	= CONTROL_OCTAVE_CURRENT;
			pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);

			// adjusting the note, and set the update if needed
			if (_note_current.tone == KEYBOARD_KEY_TO_TONE(-1))
			{
				_note_current.tone		=	12;
				_note_current.octave	=	0;
				_note_current.is_reset	=	true;
			}
			else
			{
				_note_current.octave	+=	_note_current.tone / 12;
				_note_current.tone		=	_note_current.tone % 12;
				_note_current.is_reset	= 	false;
			}


			// if there needs to be a change in the playing of the speaker
			// 	> octave or tone or is_reset
			if (!SPEAKER_CMP_NOTE(_note_current, _note_old))
			{
				if (_note_current.is_reset)
				{
					speaker_play_single_rest();
				}
				else
				{
					speaker_play_single_note(&_note_current);
				}
				
				pthread_mutex_lock(&XYLOBIT_MTX_NOTE_CURRENT);
				XYLOBIT_NOTE_CURRENT = _note_current;
				pthread_mutex_unlock(&XYLOBIT_MTX_NOTE_CURRENT);

				sem_post(&XYLOBIT_SEM_LCD_UPDATE);
			}

			_note_old = _note_current;

			// playing part
			//
			pthread_mutex_lock(&XYLOBIT_MTX_IS_PLAYING);
			_is_playing = CONTROL_IS_PLAYING;
			pthread_mutex_unlock(&XYLOBIT_MTX_IS_PLAYING);

			if (_is_playing)
			{
				// add the note to the list
				rf_new.lst_note[rf_new.len_lst_note] = _note_current;

				rf_new.len_lst_note++;

				if (rf_new.len_lst_note == RECORD_MAX_NOTE)
				{
					_is_started = false;
					_is_playing = false;

					pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
					CONTROL_IS_STARTED = false;
					pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

					pthread_mutex_lock(&XYLOBIT_MTX_IS_PLAYING);
					CONTROL_IS_PLAYING = false;
					pthread_mutex_unlock(&XYLOBIT_MTX_IS_PLAYING);

					sem_post(&XYLOBIT_SEM_LCD_UPDATE);
				}
			}
			///
		}
		else
		{
			if (!has_stopped)
			{
				has_stopped = true;

				speaker_play_single_rest();

				// save the notes
				_ret = record_add(&rf_new);
				if(_ret != 0)
				{
					free(rf_new.lst_note);

					pthread_mutex_lock(&XYLOBIT_MTX_ERROR);
					UTI_ERROR = 103;
					pthread_mutex_unlock(&XYLOBIT_MTX_ERROR);

					pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
					CONTROL_MODE_CURRENT = CONTROL_MODE_CONFIG;
					pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

					return NULL;
				}

				rf_new.len_lst_note = 0;

				_ret = record_get_unique_name(rf_new.name_file);
				if (_ret != 0)
				{
					free(rf_new.lst_note);

					pthread_mutex_lock(&XYLOBIT_MTX_ERROR);
					UTI_ERROR = 104;
					pthread_mutex_unlock(&XYLOBIT_MTX_ERROR);

					pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
					CONTROL_MODE_CURRENT = CONTROL_MODE_CONFIG;
					pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

					return NULL;
				}
			}
		}

		// must change ***********************************************
		//vTaskDelay(time_delay / portTICK_PERIOD_MS);
		speaker_pause_beat(SPEAKER_BEAT_1H8);

		// updating the local instance of global var
		pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
		_is_on = CONTROL_IS_ON;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

		pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
		_mode_current = CONTROL_MODE_CURRENT;
		pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);
	}


	speaker_play_single_rest();

	free(rf_new.lst_note);

	return NULL;
}

static void*
xylobit_thread_controller(
		void* arg
		)
{
	//
	// var
	//
	
	int _i = 0;
	
	// this bool is used to check if there has been any update on the pins
	// so that it doesn't delay twice and only have the final delay if there
	// hasn't been any delay
	bool has_updated = false;

	uint8_t	new_volume		= 0;
	uint8_t	new_octave		= 0;

	pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
	uint8_t	_octave_current = CONTROL_OCTAVE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);

	pthread_mutex_lock(&XYLOBIT_MTX_VOLUME);
	uint8_t	_volume_current = CONTROL_VOLUME_PERC;
	pthread_mutex_unlock(&XYLOBIT_MTX_VOLUME);

	pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
	bool _is_on = CONTROL_IS_ON;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);


	//
	// main
	//

	while (_is_on)
	{
		// checking for updates on intr - for each button and switch
		for (_i = 0; _i < CONTROL_NUM_PIN; _i++)
		{
			// the pin should also be LOW (0) otherwise will ignore
			// 	> this is to avoid the bouncing on the rising edge, bcause the 
			// 	debouncing delay only avoids the bouncing caused by the falling
			// 	edge and can't possibly avoid the one cause by the rising edge
			// 	- this is also because the intr is meant to trigger on the
			// 	falling edge only
			if (
					CONTROL_LST_FLAG_TRIG[_i]					&&
					!(bool)gpio_get_level(CONTROL_LST_PIN[_i])
					)
			{
				pthread_mutex_lock(XYLOBIT_LST_MTX[_i]);
				control_toggle_controller(_i);
				pthread_mutex_unlock(XYLOBIT_LST_MTX[_i]);

				has_updated = true;

				vTaskDelay(CONTROL_TIME_DEBOUNCING / portTICK_PERIOD_MS);
				CONTROL_LST_FLAG_TRIG[_i] = false;
			}
		}

		// checking any change on the pots


		control_update_volume(&new_volume);
		if (new_volume != _volume_current)
		{
			pthread_mutex_lock(&XYLOBIT_MTX_VOLUME);
			CONTROL_VOLUME_PERC = new_volume;
			pthread_mutex_unlock(&XYLOBIT_MTX_VOLUME);

			_volume_current = new_volume;
			sem_post(&XYLOBIT_SEM_LCD_UPDATE);
		}

		control_update_octave(&new_octave);
		if (new_octave != _octave_current)
		{
			pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
			CONTROL_OCTAVE_CURRENT = new_octave;
			pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);

			_octave_current = new_octave;
			sem_post(&XYLOBIT_SEM_LCD_UPDATE);
		}


		if (!has_updated)
		{
			vTaskDelay(CONTROL_TIME_DEBOUNCING / portTICK_PERIOD_MS);
		}
		else
		{
			sem_post(&XYLOBIT_SEM_LCD_UPDATE);
		}

		has_updated = false;

		pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
		_is_on = CONTROL_IS_ON;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);
	}

	return NULL;
}


static void*
xylobit_thread_display_lcd(
		void* arg
		)
{
	//
	// var
	//

	int _i = 0;
	//int _err = 0;
	int _sem_val = 0;

	int time_delay_display = 500;
	
	//size_t len_str_line1 = 20;
	//char str_line1[20];

	size_t len_str_line2 = 20;
	char str_line2[20];

	// local inst of gloal var
	pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
	bool _is_on = CONTROL_IS_ON;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

	pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
	bool _is_started = CONTROL_IS_STARTED;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

	pthread_mutex_lock(&XYLOBIT_MTX_IS_PLAYING);
	bool _is_playing = CONTROL_IS_PLAYING;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_PLAYING);

	pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
	control_mode_opr_t _mode_current = CONTROL_MODE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

	pthread_mutex_lock(&XYLOBIT_MTX_LCD_PAGE);
	control_page_lcd_t _page_lcd = CONTROL_PAGE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_LCD_PAGE);
	
	pthread_mutex_lock(&XYLOBIT_MTX_VOLUME);
	uint8_t	_volume_current = CONTROL_VOLUME_PERC;
	pthread_mutex_unlock(&XYLOBIT_MTX_VOLUME);

	pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
	octv_t _octave_current = CONTROL_OCTAVE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);

	pthread_mutex_lock(&XYLOBIT_MTX_NOTE_CURRENT);
	Note _note_current = XYLOBIT_NOTE_CURRENT;
	pthread_mutex_unlock(&XYLOBIT_MTX_NOTE_CURRENT);

	pthread_mutex_lock(&XYLOBIT_MTX_BPM);
	beat_t _beat_current = SPEAKER_BPM;
	pthread_mutex_unlock(&XYLOBIT_MTX_BPM);


	//
	// main
	//
	
	while (_is_on)
	{

		// wait till there's an update,
		// and clear if there is more overhead values
		sem_getvalue(&XYLOBIT_SEM_LCD_UPDATE, &_sem_val);

		if (_sem_val == 0)
		{
			sem_wait(&XYLOBIT_SEM_LCD_UPDATE);
		}
		else
		{
			for (_i = 0; _i < _sem_val; _i++)
			{
				sem_wait(&XYLOBIT_SEM_LCD_UPDATE);
			}
		}

		// update //

		pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
		_is_on = CONTROL_IS_ON;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

		pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
		_is_started = CONTROL_IS_STARTED;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

		pthread_mutex_lock(&XYLOBIT_MTX_IS_PLAYING);
		_is_playing = CONTROL_IS_PLAYING;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_PLAYING);

		pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
		_mode_current = CONTROL_MODE_CURRENT;
		pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

		pthread_mutex_lock(&XYLOBIT_MTX_LCD_PAGE);
		_page_lcd = CONTROL_PAGE_CURRENT;
		pthread_mutex_unlock(&XYLOBIT_MTX_LCD_PAGE);

		pthread_mutex_lock(&XYLOBIT_MTX_VOLUME);
		_volume_current = CONTROL_VOLUME_PERC;
		pthread_mutex_unlock(&XYLOBIT_MTX_VOLUME);

		pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
		_octave_current = CONTROL_OCTAVE_CURRENT;
		pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);

		pthread_mutex_lock(&XYLOBIT_MTX_NOTE_CURRENT);
		_note_current = XYLOBIT_NOTE_CURRENT;
		pthread_mutex_unlock(&XYLOBIT_MTX_NOTE_CURRENT);

		pthread_mutex_lock(&XYLOBIT_MTX_BPM);
		_beat_current = SPEAKER_BPM;
		pthread_mutex_unlock(&XYLOBIT_MTX_BPM);


		if (_page_lcd == CONTROL_PAGE_LCD_1)
		{
			if (_mode_current == CONTROL_MODE_PLAY_LIVE)
			{
				if (_is_started)
				{
					snprintf(str_line2, 20, "Playing %s, in:%1.d",
							SPEAKER_LST_TONE_NAME[(_note_current.tone) % 13],
							_octave_current
							);
				}
				else
				{
					len_str_line2 = 16;

					uti_copy_str_len(
							"Stopped Playing!",
							str_line2,
							&len_str_line2,
							false
							);
				}
				// show new values

				lcd_write(
						16,
						"Page_1 Play Live",
						16,
						str_line2
						);
			}
			else if (_mode_current == CONTROL_MODE_PLAY_RECORD)
			{
				len_str_line2 = 16;

				if (_is_started)
				{
					if (_is_playing)
					{
						uti_copy_str_len(
								"Started, Playing",
								str_line2,
								&len_str_line2,
								false
								);
					}
					else
					{
						uti_copy_str_len(
								"Started, Paused ",
								str_line2,
								&len_str_line2,
								false
								);
					}
				}
				else
				{
					uti_copy_str_len(
							"Record's Stopped",
							str_line2,
							&len_str_line2,
							false
							);
				}

				// show new values
				lcd_write(
						16,
						"Page_1 Replaying",
						16,
						str_line2
						);
			}
			else if (_mode_current == CONTROL_MODE_RECORDING)
			{
				len_str_line2 = 16;

				if (_is_started)
				{
					if (_is_playing)
					{
						uti_copy_str_len(
								"Start, Recording",
								str_line2,
								&len_str_line2,
								false
								);
					}
					else
					{
						uti_copy_str_len(
								"Started, Paused ",
								str_line2,
								&len_str_line2,
								false
								);
					}
				}
				else
				{
					uti_copy_str_len(
							"Record's Stopped",
							str_line2,
							&len_str_line2,
							false
							);
				}

				// show new values
				lcd_write(
						16,
						"Page_1 Recording",
						16,
						str_line2
						);
			}
			else if (_mode_current == CONTROL_MODE_CONFIG)
			{
				pthread_mutex_lock(&XYLOBIT_MTX_ERROR);

				if (UTI_ERROR == 0)
				{
					len_str_line2 = 16;

					uti_copy_str_len(
							"No Error, V.1.0 ",
							str_line2,
							&len_str_line2,
							false
							);
				}
				else
				{
					snprintf(str_line2, 20, "Err %4.d, V.1.0 ",
							(UTI_ERROR % 10000)
							);
				}

				pthread_mutex_unlock(&XYLOBIT_MTX_ERROR);

				lcd_write(
						16,
						"Page_1 Configure",
						16,
						str_line2
						);
			}
			else
			{
				lcd_write(
						16,
						"INVALID PAGE",
						16,
						"Err 1111, V.1.0 "
						);
			}
		}
		else if (_page_lcd == CONTROL_PAGE_LCD_2)
		{
			snprintf(str_line2, 20, "BPM%3.d Volume:%2.d",
					(_beat_current % 221),
					(_volume_current % 11)
					);

			lcd_write(
					16,
					"Page_2 Note Info",
					16,
					str_line2
					);
		}
		else
		{
			pthread_mutex_lock(&XYLOBIT_MTX_RF_NAME);

			if (RECORD_FILE_CURRENT == NULL)
			{
				len_str_line2 = 16;

				uti_copy_str_len(
						"RF: -None-      ",
						str_line2,
						&len_str_line2,
						false
						);
			}
			else
			{
				snprintf(str_line2, 20, "RF: %.10s",
						RECORD_FILE_CURRENT
						);
			}

			pthread_mutex_unlock(&XYLOBIT_MTX_RF_NAME);

			lcd_write(
					16,
					"Page_3: Settings",
					16,
					str_line2
					);
		}
	
		vTaskDelay(time_delay_display / portTICK_PERIOD_MS);
	}


	return NULL;
}


/*
static void*
xylobit_thread_play_record(
		void* arg
		)
{
	//size_t num_note = 24;
	//Note lst_note[num_note];

	//tone_t lst_tone[] = {
	//	B, E, G, Fsh, E, G, E, Fsh, E, C, D, B,
	//	B, E, G, Fsh, E, G, E, Fsh, E, B, Ash, A
	//};
	//beat_t lst_beat[num_note];
	//beat_t _lst_beat[] =
	//{
	//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
	//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2
	//};

	//for (_i = 0; _i < num_note; _i++)
	//{
	//	lst_note[_i] = (Note){4, lst_tone[_i], false};
	//	lst_beat[_i] = _lst_beat[_i];
	//}
}
*/
	 	
//}


// func - http hdl
// {

esp_err_t
xylobit_http_hdl_home(
		httpd_req_t*	req_get
		)
{
	//
	// var
	//

	int _err = 0;
	int _i = 0;
	
	char* path_file_home = WEBSITE_DIR"/home.wxb";

	char* str_resp	= NULL;
	size_t len_str_resp = 0;

	char* str_error_format	= "<h1>Error fetching data (%d)</h1>";
	char str_error[50];


	size_t len_lst_syringe_home = 9;
	website_syringe_t lst_syringe_home[len_lst_syringe_home];

	website_syringe_t lst_syringe_lookup[][4] =
	{
		// mode
		{
			{ .str_value = "Configuration",		.len_str_value = 13 },
			{ .str_value = "Playing Live",		.len_str_value = 12 },
			{ .str_value = "Playing Record",	.len_str_value = 14 },
			{ .str_value = "Recording",			.len_str_value =  9 }
		},
		// name of LCD pages
		{
			{ .str_value = "Main Page",			.len_str_value =  9 },
			{ .str_value = "Mode Info", 		.len_str_value =  9 },
			{ .str_value = "Keyboard Info", 	.len_str_value = 13 }
		}
	};

	// just to store the str values - for safty
	char lst_str_syringe[4][32];
	char _str_lst_files[200];

	int _len_str_append = 0;


	//
	// main
	//
	
	// loading home syringe values
	// {

	pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
	lst_syringe_home[0].str_value = CONTROL_IS_STARTED ? "ON" : "OFF";
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);
	lst_syringe_home[0].len_str_value = strlen(lst_syringe_home[0].str_value);

	pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
	lst_syringe_home[1] = lst_syringe_lookup[0][CONTROL_MODE_CURRENT];
	pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

	pthread_mutex_lock(&XYLOBIT_MTX_IS_PLAYING);
	lst_syringe_home[2].str_value		= CONTROL_IS_PLAYING ?
		"Playing" : "Paused";
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_PLAYING);
	lst_syringe_home[2].len_str_value = strlen(lst_syringe_home[2].str_value);

	pthread_mutex_lock(&XYLOBIT_MTX_BPM);
	snprintf(lst_str_syringe[0], 10, "%3.d", SPEAKER_BPM);
	pthread_mutex_unlock(&XYLOBIT_MTX_BPM);
	lst_syringe_home[3].str_value		= lst_str_syringe[0];
	lst_syringe_home[3].len_str_value	= 3;

	pthread_mutex_lock(&XYLOBIT_MTX_OCTAVE);
	snprintf(lst_str_syringe[1], 10, "%1.d", CONTROL_OCTAVE_CURRENT);
	pthread_mutex_unlock(&XYLOBIT_MTX_OCTAVE);
	lst_syringe_home[4].str_value		= lst_str_syringe[1];
	lst_syringe_home[4].len_str_value	= 1;

	pthread_mutex_lock(&XYLOBIT_MTX_LCD_PAGE);
	lst_syringe_home[5] = lst_syringe_lookup[1][CONTROL_PAGE_CURRENT];
	pthread_mutex_unlock(&XYLOBIT_MTX_LCD_PAGE);

	lst_syringe_home[6].len_str_value = RECORD_XLEN_RF_NAME;

	pthread_mutex_lock(&XYLOBIT_MTX_RF_NAME);
	if (
			(RECORD_FILE_CURRENT 	== NULL) ||
			(RECORD_NUM_FILE		== 0)
	   )
	{
		lst_syringe_home[6].str_value = "None";
		lst_syringe_home[6].len_str_value = 4;
	}
	else
	{
		uti_copy_str_len(
				RECORD_FILE_CURRENT,
				lst_str_syringe[2],
				&(lst_syringe_home[6].len_str_value),
				true
				);

		lst_syringe_home[6].str_value = lst_str_syringe[2];
	}
	pthread_mutex_unlock(&XYLOBIT_MTX_RF_NAME);

	// doing 7, 8 - num of file and list of files together
	// {
	pthread_mutex_lock(&XYLOBIT_MTX_RF_LST);
	snprintf(lst_str_syringe[3], 10, "%2.d", RECORD_NUM_FILE);
	lst_syringe_home[8].len_str_value = 0;

	for (_i = 0; _i < RECORD_NUM_FILE; _i++)
	{
		_len_str_append = snprintf(
				&(_str_lst_files[lst_syringe_home[8].len_str_value]),
				RECORD_XLEN_RF_NAME + 14,
				"<li>%s</li>\n",
				RECORD_LST_FILE[_i]
				);

		lst_syringe_home[8].len_str_value += _len_str_append;
	}

	pthread_mutex_unlock(&XYLOBIT_MTX_RF_LST);
	lst_syringe_home[7].str_value		= lst_str_syringe[3];
	lst_syringe_home[8].str_value		= _str_lst_files;

	lst_syringe_home[7].len_str_value	= 2;
	// }

	// }

	_err = website_alloc_fetch_file(
			9,
			lst_syringe_home,
			path_file_home,
			&str_resp,
			&len_str_resp
			);

	if (_err == 0)
	{
	    httpd_resp_send(req_get, str_resp, HTTPD_RESP_USE_STRLEN);
		free(str_resp);
		return ESP_OK;
	}
	else
	{
		snprintf(str_error, 100, str_error_format, _err);
		httpd_resp_send(req_get, str_error, HTTPD_RESP_USE_STRLEN);
		return ESP_OK;
	}


	//
	// end
	//
	
	free(str_resp);

	return ESP_OK;
}

httpd_uri_t		XYLOBIT_HTTP_URI_ROOT =
{
	.uri		= "/",
	.method		= HTTP_GET,
	.handler	= xylobit_http_hdl_home,
	.user_ctx	= NULL
};

httpd_uri_t		XYLOBIT_HTTP_URI_HOME =
{
	.uri		= "/home",
	.method		= HTTP_GET,
	.handler	= xylobit_http_hdl_home,
	.user_ctx	= NULL
};


esp_err_t
xylobit_http_hdl_get_edit(
		httpd_req_t*	req_get
		)
{
	//
	// var
	//
	
	int _i = 0;
	int _e = 0;

	char* path_file_edit = WEBSITE_DIR"/edit.wxb";

	char* str_resp = NULL;
	size_t	len_str_resp = 0;

	char* str_error_format	= "<h1>Error fetching data (%d)</h1>";
	char str_error[50];
	
	size_t len_lst_syringe_edit = 4;
	website_syringe_t lst_syringe_edit[len_lst_syringe_edit];

	int8_t _bpm_perctage = 0;

	char _str_bpm_percentage[8];
	char _str_lst_files[400];

	int _len_str_append = 0;

	//
	// main
	//

	pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
	if (CONTROL_IS_STARTED)
	{
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

		lst_syringe_edit[0].str_value = "checked";
		lst_syringe_edit[0].len_str_value = 7;

		lst_syringe_edit[1].str_value = "";
		lst_syringe_edit[1].len_str_value = 0;
	}
	else
	{
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

		lst_syringe_edit[1].str_value = "checked";
		lst_syringe_edit[1].len_str_value = 7;

		lst_syringe_edit[0].str_value = "";
		lst_syringe_edit[0].len_str_value = 0;
	}


	pthread_mutex_lock(&XYLOBIT_MTX_BPM);
	_bpm_perctage = SPEAKER_BPM;
	pthread_mutex_unlock(&XYLOBIT_MTX_BPM);
	snprintf(_str_bpm_percentage, 8, "%d", _bpm_perctage);
	lst_syringe_edit[2].str_value		= _str_bpm_percentage;
	lst_syringe_edit[2].len_str_value	= 3;

	// list of options
	// {
	pthread_mutex_lock(&XYLOBIT_MTX_RF_LST);
	lst_syringe_edit[3].len_str_value = 0;

	for (_i = 0; _i < RECORD_NUM_FILE; _i++)
	{
		if (RECORD_FILE_CURRENT == NULL)
		{
			_len_str_append = snprintf(
					&(_str_lst_files[lst_syringe_edit[3].len_str_value]),
					RECORD_XLEN_RF_NAME + 32,
					"<option value='%d' >%s</option>\n",
					_i,
					RECORD_LST_FILE[_i]
					);

			lst_syringe_edit[3].len_str_value += _len_str_append;
		}
		else if (0 == strcmp(
					RECORD_LST_FILE[_i],
					RECORD_FILE_CURRENT
				  )
		   )
		{
			_len_str_append = snprintf(
					&(_str_lst_files[lst_syringe_edit[3].len_str_value]),
					RECORD_XLEN_RF_NAME + 55,
					"<option selected='selected' value='%d' >'%s'</option>\n",
					_i,
					RECORD_LST_FILE[_i]
					);

			lst_syringe_edit[3].len_str_value += _len_str_append;
		}
		else
		{
			_len_str_append = snprintf(
					&(_str_lst_files[lst_syringe_edit[3].len_str_value]),
					RECORD_XLEN_RF_NAME + 35,
					"<option value='%d' >'%s'</option>\n",
					_i,
					RECORD_LST_FILE[_i]
					);

			lst_syringe_edit[3].len_str_value += _len_str_append;
		}
	}

	pthread_mutex_unlock(&XYLOBIT_MTX_RF_LST);

	lst_syringe_edit[3].str_value		= _str_lst_files;

	// }

	_e = website_alloc_fetch_file(
			len_lst_syringe_edit,
			lst_syringe_edit,
			path_file_edit,
			&str_resp,
			&len_str_resp
			);

	if (_e == 0)
	{
	    httpd_resp_send(req_get, str_resp, HTTPD_RESP_USE_STRLEN);
		free(str_resp);

		return ESP_OK;
	}
	else
	{
		snprintf(str_error, 50, str_error_format, _e);
		httpd_resp_send(req_get, str_error, HTTPD_RESP_USE_STRLEN);

		return ESP_OK;
	}

	return ESP_OK;
}
httpd_uri_t		XYLOBIT_HTTP_URI_GET_EDIT =
{
	.uri		= "/edit",
	.method		= HTTP_GET,
	.handler	= xylobit_http_hdl_get_edit,
	.user_ctx	= NULL
};



esp_err_t
xylobit_http_hdl_post_edit(
		httpd_req_t*	req_post
		)
{
	//
	// var
	//
	
	int _i = 0;
	int _e = 0;

	size_t	len_buf_recv	= 50;
	size_t	_len_buf_recv	= 50;
    char	buf_recv[len_buf_recv];

	size_t len_lst_values	= 3;
	size_t _len_lst_values	= 3;
	website_post_val_t lst_values[len_lst_values];

	bool is_started = false;
	int record_file_index = 0;
	uint8_t bpm = 0;

	char* _ptr_strtol;

	char _lst_buf_names[len_lst_values][10];
	char _lst_buf_values[len_lst_values][10];

	// "onoff"		: {0, 1}
	// "BPM"		: [20-220]
	// "BPM"		: [0-100]
	// "record"		: [0-10]
	size_t	_lst_len_buf[][2] =
	{
		{ 6, 2 },
		{ 4, 4 },
		{ 7, 3 }
	};


	//
	// main
	//
	
	_e = httpd_req_recv(req_post, buf_recv, len_buf_recv);

	if (_e <= 0)
	{
		if (_e == HTTPD_SOCK_ERR_TIMEOUT)
		{
			httpd_resp_send_408(req_post);
		}

		return ESP_FAIL;
	}

	_len_buf_recv = (req_post->content_len < len_buf_recv) ?
		req_post->content_len : len_buf_recv;


	for (_i = 0; _i < len_lst_values; _i++)
	{
		lst_values[_i].len_str_name		= _lst_len_buf[_i][0];
		lst_values[_i].len_str_value	= _lst_len_buf[_i][1];
		lst_values[_i].str_name			= _lst_buf_names[_i];
		lst_values[_i].str_value		= _lst_buf_values[_i];
	}

	_e = website_extract_post_val(
			_len_buf_recv,
			buf_recv,
			&_len_lst_values,
			lst_values
			);

	// converting to the local variables first,
	// then load to the global var 
	// {

	is_started = (lst_values[0].str_value[0] == '1');

	bpm = strtol(lst_values[1].str_value, &_ptr_strtol, 10);

	if (
			(*_ptr_strtol == '\0')	&&
			(bpm > SPEAKER_BPM_MIN)	&&
			(bpm < SPEAKER_BPM_MAX)
	   )
	{
		pthread_mutex_lock(&XYLOBIT_MTX_BPM);
		SPEAKER_BPM = bpm;
		pthread_mutex_unlock(&XYLOBIT_MTX_BPM);
	}

	// this a temporary thing to convert ascii char to int
	// - it must change
	record_file_index = lst_values[2].str_value[0] - 0x30;

	// loading

	pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
	CONTROL_IS_STARTED = is_started;
	pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

	pthread_mutex_lock(&XYLOBIT_MTX_RF_LST);
	if (
			(record_file_index < 0) ||
			(record_file_index > RECORD_NUM_FILE)
	   )
	{
		record_file_index = 0;
	}
	else
	{
		pthread_mutex_lock(&XYLOBIT_MTX_RF_NAME);
		RECORD_FILE_CURRENT = RECORD_LST_FILE[record_file_index];
		pthread_mutex_unlock(&XYLOBIT_MTX_RF_NAME);
	}
	pthread_mutex_unlock(&XYLOBIT_MTX_RF_LST);
	// }
	
	// updating the lcd
	sem_post(&XYLOBIT_SEM_LCD_UPDATE);

	// redirect back to /home page
	httpd_resp_set_status(req_post, "303 See Other");
	httpd_resp_set_hdr(req_post, "Location", "/home");
	httpd_resp_send(req_post, NULL, 0);

	return ESP_OK;
}
httpd_uri_t		XYLOBIT_HTTP_URI_POST_EDIT =
{
	.uri		= "/edit",
	.method		= HTTP_POST,
	.handler	= xylobit_http_hdl_post_edit,
	.user_ctx	= NULL
};

esp_err_t
xylobit_http_hdl_download(
		httpd_req_t*	req_get
		)
{
	//
	// var
	//
	
	int _e = 0;
	//int _i = 0;

	size_t len_buf_file_content = 0;
	uint8_t* buf_file_content = NULL;

	//
	// main
	//
	
	pthread_mutex_lock(&XYLOBIT_MTX_RF_NAME);
	pthread_mutex_lock(&XYLOBIT_MTX_RF_LST);
	if (
			(RECORD_FILE_CURRENT	== NULL) ||
			(RECORD_NUM_FILE		== 0)
	   )
	{
		httpd_resp_send_err(
				req_get,
				HTTPD_400_BAD_REQUEST,
				"Not a valid name of a record file."
				);

		pthread_mutex_unlock(&XYLOBIT_MTX_RF_NAME);
		pthread_mutex_unlock(&XYLOBIT_MTX_RF_LST);

		return ESP_FAIL;
	}

	_e = record_alloc_read(
			RECORD_FILE_CURRENT,
			&len_buf_file_content,
			&buf_file_content
			);

	pthread_mutex_unlock(&XYLOBIT_MTX_RF_NAME);
	pthread_mutex_unlock(&XYLOBIT_MTX_RF_LST);

	if (_e != 0)
	{
		httpd_resp_send_err(
				req_get,
				HTTPD_500_INTERNAL_SERVER_ERROR,
				"Error reading the record file!"
				);

		return ESP_FAIL;
	}

	httpd_resp_set_type(req_get, "application/octet-stream");
	httpd_resp_set_hdr(
			req_get,
			"Content-Disposition",
			"attachment; filename=\"Xylobit_RecordFile.rf\""
			);

	// sending the file in chunks - incase it's too large
	//_e = website_send_chunck(
	//		req_get,
	//		len_buf_file_content,
	//		100,
	//		(char*)buf_file_content
	//		);
	//
	httpd_resp_send(req_get, (char*)buf_file_content, len_buf_file_content);

	if (_e != 0)
	{
		return ESP_FAIL;
	}


	return ESP_OK;
}
httpd_uri_t		XYLOBIT_HTTP_URI_DOWNLOAD =
{
	.uri		= "/download",
	.method		= HTTP_GET,
	.handler	= xylobit_http_hdl_download,
	.user_ctx	= NULL
};


esp_err_t
xylobit_http_hdl_delete(
		httpd_req_t*	req_get
		)
{
	//
	// var
	//
	
	int _e = 0;


	//
	// main
	//

	pthread_mutex_lock(&XYLOBIT_MTX_RF_NAME);
	pthread_mutex_lock(&XYLOBIT_MTX_RF_LST);
	if (
			(RECORD_FILE_CURRENT	== NULL) ||
			(RECORD_NUM_FILE		== 0)
	   )
	{
		httpd_resp_send_err(
				req_get,
				HTTPD_400_BAD_REQUEST,
				"Not a valid name of a record file."
				);

		pthread_mutex_unlock(&XYLOBIT_MTX_RF_NAME);
		pthread_mutex_unlock(&XYLOBIT_MTX_RF_LST);

		return ESP_FAIL;
	}

	// removing the record file
	_e = record_remove_rf(RECORD_FILE_CURRENT);


	if (_e != 0)
	{
		httpd_resp_send_err(
				req_get,
				HTTPD_500_INTERNAL_SERVER_ERROR,
				"Error removing the record file!"
				);

		return ESP_FAIL;
	}

	record_update_list();

	pthread_mutex_unlock(&XYLOBIT_MTX_RF_NAME);
	pthread_mutex_unlock(&XYLOBIT_MTX_RF_LST);

	// redirect back to /home page
	httpd_resp_set_status(req_get, "303 See Other");
	httpd_resp_set_hdr(req_get, "Location", "/home");
	httpd_resp_send(req_get, NULL, 0);



	return ESP_OK;
}
httpd_uri_t		XYLOBIT_HTTP_URI_DELETE =
{
	.uri		= "/deleterf",
	.method		= HTTP_GET,
	.handler	= xylobit_http_hdl_delete,
	.user_ctx	= NULL
};
// }


//
// main //
//

void app_main(void)
{
	//
	// var
	//
	
	//int _i = 0;
	int _ret = 0;

	char *task_local = "Xylobit_MAIN";

	bool				_is_on			= CONTROL_IS_ON;
	control_mode_opr_t	_mode_current	= CONTROL_MODE_CURRENT;


	//
	// main //
	//

	// opening all xylibit devices threading var
	//
	ESP_LOGI(task_local, "opening...");

	if (uti_check_err(xylobit_open()))
	{
		ESP_LOGE(task_local, "Error opening Xylobit. Aborting.");
		xylobit_close();
		assert(false);
	}

	ESP_LOGI(task_local, "opened");
	//


	// starting the general threads
	// 	> controller
	// 	> display
	// {
	ESP_LOGI(task_local, "starting <controller>...");

	if (uti_check_err(pthread_create(
			&XYLOBIT_THREAD_CONTROLLER,
			&XYLOBIT_THREAD_ATTR,
			xylobit_thread_controller,
			NULL
			)))
	{
		ESP_LOGE(task_local, "Error starting <controller>. Aborting.");
		xylobit_close();
		assert(false);
	}

	ESP_LOGI(task_local, "started <controller>");


	ESP_LOGI(task_local, "starting <display>...");

	if (uti_check_err(pthread_create(
			&XYLOBIT_THREAD_DISPLAY,
			&XYLOBIT_THREAD_ATTR,
			xylobit_thread_display_lcd,
			NULL
			)))
	{
		ESP_LOGE(task_local, "Error starting <disaply>. Aborting.");

		// closing the controller before shutting down //
		//
		ESP_LOGW(task_local, "Closing <controller>...");

		pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
		CONTROL_IS_ON = false;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

		pthread_join(XYLOBIT_THREAD_CONTROLLER, NULL);

		// xylobit close
		//
		xylobit_close();
		assert(false);
	}

	ESP_LOGI(task_local, "started <display>");
	// }


	// starting the website
	// - registering uri
	// {
	ESP_LOGI(task_local, "starting <website>...");

	httpd_register_uri_handler(
			WEBSITE_SERVER,
			&XYLOBIT_HTTP_URI_ROOT
			);

	httpd_register_uri_handler(
			WEBSITE_SERVER,
			&XYLOBIT_HTTP_URI_HOME
			);

	httpd_register_uri_handler(
			WEBSITE_SERVER,
			&XYLOBIT_HTTP_URI_GET_EDIT
			);

	httpd_register_uri_handler(
			WEBSITE_SERVER,
			&XYLOBIT_HTTP_URI_POST_EDIT
			);

	httpd_register_uri_handler(
			WEBSITE_SERVER,
			&XYLOBIT_HTTP_URI_DOWNLOAD
			);

	httpd_register_uri_handler(
			WEBSITE_SERVER,
			&XYLOBIT_HTTP_URI_DELETE
			);

	ESP_LOGI(task_local, "started <website>");
	// }


	// main while loop of xylobit
	// {
	ESP_LOGI(task_local, "Xylobit is ON now.");


	// temp - just to open up a space if it's full
	// {

	_ret = remove("/xylobit/record/1.RF");

	if (_ret != 0)
	{
		ESP_LOGE(task_local, "error removing <%d><%d><%s>\n",
				_ret, errno, strerror(errno));
	}
	else
	{
		ESP_LOGW(task_local, "successful remove\n");
	}

	// }


	while(_is_on)
	{
		if (_mode_current == CONTROL_MODE_CONFIG)
		{
			//if (uti_check_err(pthread_create(
			//		&XYLOBIT_THREAD_PLAY_LIVE,
			//		&XYLOBIT_THREAD_ATTR,
			//		xylobit_thread_play_live,
			//		NULL
			//		)))
			//{
			//	pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
			//	CONTROL_IS_ON = false;
			//	pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

			//	break;
			//}
			//pthread_join(XYLOBIT_THREAD_PLAY_LIVE, NULL);
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
		else if (_mode_current == CONTROL_MODE_PLAY_LIVE)
		{
			if (uti_check_err(pthread_create(
					&XYLOBIT_THREAD_PLAY_LIVE,
					&XYLOBIT_THREAD_ATTR,
					xylobit_thread_play_live,
					NULL
					)))
			{
				pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
				CONTROL_IS_ON = false;
				pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

				break;
			}
			pthread_join(XYLOBIT_THREAD_PLAY_LIVE, NULL);
		}
		else if (_mode_current == CONTROL_MODE_RECORDING)
		{
			if (uti_check_err(pthread_create(
					&XYLOBIT_THREAD_RECORD,
					&XYLOBIT_THREAD_ATTR,
					xylobit_thread_record,
					NULL
					)))
			{
				pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
				CONTROL_IS_ON = false;
				pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

				break;
			}
			pthread_join(XYLOBIT_THREAD_RECORD, NULL);
		}
		else if (_mode_current == CONTROL_MODE_PLAY_RECORD)
		{
			if (uti_check_err(pthread_create(
					&XYLOBIT_THREAD_PLAY_RECORD,
					&XYLOBIT_THREAD_ATTR,
					xylobit_thread_play_record,
					NULL
					)))
			{
				pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
				CONTROL_IS_ON = false;
				pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

				break;
			}
			pthread_join(XYLOBIT_THREAD_PLAY_RECORD, NULL);
		}
		else
		{
			//
			// error
			//
			ESP_LOGE(task_local,
					"Error<12>, Xylobit is set to an unkown mode.");

			pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
			CONTROL_IS_ON = false;
			pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);

			break;
		}

		vTaskDelay(pdMS_TO_TICKS(500));

		pthread_mutex_lock(&XYLOBIT_MTX_IS_STARTED);
		CONTROL_IS_STARTED = false;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_STARTED);

		pthread_mutex_lock(&XYLOBIT_MTX_IS_PLAYING);
		CONTROL_IS_PLAYING = false;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_PLAYING);

		sem_post(&XYLOBIT_SEM_LCD_UPDATE);

		// while loop updating
		//
		pthread_mutex_lock(&XYLOBIT_MTX_MODE_CURRENT);
		_mode_current = CONTROL_MODE_CURRENT;
		pthread_mutex_unlock(&XYLOBIT_MTX_MODE_CURRENT);

		pthread_mutex_lock(&XYLOBIT_MTX_IS_ON);
		_is_on = CONTROL_IS_ON;
		pthread_mutex_unlock(&XYLOBIT_MTX_IS_ON);
		///
	}

	// }


	// after main loop
	// 	> close all threads
	// 	> close xylobit
	// 	{
	ESP_LOGI(task_local, "finished <display>");
	pthread_join(XYLOBIT_THREAD_DISPLAY, NULL);

	pthread_join(XYLOBIT_THREAD_CONTROLLER, NULL);
	ESP_LOGI(task_local, "finished <controller>");


	// closing all devices
	ESP_LOGI(task_local, "closing...");
	
	// kinda ignoring the error returned from xylobit_close()
	uti_check_err(xylobit_close());

	ESP_LOGI(task_local, "closed");
	// }
}


