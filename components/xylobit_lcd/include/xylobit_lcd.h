#ifndef _XYLOBIT_LCD_H
#define _XYLOBIT_LCD_H

//
// lib //
//

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"


//
// def //
//

#define LCD_PIN_SDA		1
#define LCD_PIN_SCL		2

#define LCD_I2C_PORT	I2C_NUM_0
#define LCD_I2C_ADDR	0x27

#define LCD_I2C_FREQ			100000
#define LCD_I2C_FLAG_ACK		0x01
#define LCD_I2C_FLAG_BUF_TX		0x00
#define LCD_I2C_FLAG_BUF_RX		0x00
#define LCD_I2C_FLAG_INTR		0x00

#define LCD_I2C_CMD_DELAY		10

#define LCD_LINE_DISTANCE	40
#define LCD_LINE_WIDTH		16
#define LCD_LINE_TOTAL		2

#define LCD_CHAR_LIM_LOW	0x20
#define LCD_CHAR_LIM_HIG	0x7F

#define LCD_BYTE_CMD_W_ON_LOW	0x08
#define LCD_BYTE_CMD_W_ON_HIG	0x0C
#define LCD_BYTE_CMD_W_OF_LOW	0x00
#define LCD_BYTE_CMD_W_OF_HIG	0x04

#define LCD_BYTE_DAT_W_ON_LOW	0x09
#define LCD_BYTE_DAT_W_ON_HIG	0x0D
#define LCD_BYTE_DAT_W_OF_LOW	0x01
#define LCD_BYTE_DAT_W_OF_HIG	0x05


//
// type //
//


//
// var //
//

extern bool LCD_IS_OPEN;
extern bool LCD_IS_ON;
extern bool LCD_LED_IS_ON;
extern bool LCD_CURSOR_IS_ON;
extern bool LCD_CURSOR_IS_BLINKING;

extern uint8_t LCD_MASK_CMD_LOW;
extern uint8_t LCD_MASK_CMD_HIG;
extern uint8_t LCD_MASK_DAT_LOW;
extern uint8_t LCD_MASK_DAT_HIG;

//
// func //
//

int
lcd_open();


int
lcd_close();


int
lcd_led(
		bool turn_on
	   );


int
lcd_init();


int
lcd_i2c_write_byte(
		const uint8_t byte_data
		);


int
lcd_switch(
		bool display_on,
		bool cursor_on,
		bool cursor_blink
		);


int
lcd_move_cursor(
		bool	move_right,
		uint8_t	num_units
		);


int
lcd_clear();


int
lcd_home();


int
lcd_write(
		const uint8_t	len_str_line1,
		const char*		str_line1,
		const uint8_t	len_str_line2,
		const char*		str_line2
		);


#endif
