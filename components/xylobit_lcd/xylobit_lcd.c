#include "xylobit_lcd.h"


//
// var
//

bool LCD_IS_OPEN			= false;
bool LCD_IS_ON				= false;
bool LCD_LED_IS_ON			= false;
bool LCD_CURSOR_IS_ON		= false;
bool LCD_CURSOR_IS_BLINKING	= false;

uint8_t LCD_MASK_CMD_LOW = LCD_BYTE_CMD_W_OF_LOW;
uint8_t LCD_MASK_CMD_HIG = LCD_BYTE_CMD_W_OF_HIG;
uint8_t LCD_MASK_DAT_LOW = LCD_BYTE_DAT_W_OF_LOW;
uint8_t LCD_MASK_DAT_HIG = LCD_BYTE_DAT_W_OF_HIG;


//
// func
//

int
lcd_open()
{
	//
	// var
	//
	
	esp_err_t _err = ESP_OK;
	
    i2c_config_t i2c_conf_driver = {
        .mode				= I2C_MODE_MASTER,
        .sda_io_num			= LCD_PIN_SDA,
        .sda_pullup_en		= GPIO_PULLUP_ENABLE,
        .scl_io_num			= LCD_PIN_SCL,
        .scl_pullup_en		= GPIO_PULLUP_ENABLE,
        .master.clk_speed	= LCD_I2C_FREQ,
    };

	
	//
	// check
	//
	
	if (LCD_IS_OPEN)
	{
		// error //
		return 1;
	}
	

	//
	// main
	//
	
	// configuring i2c driver
    _err = i2c_param_config(LCD_I2C_PORT, &i2c_conf_driver);
	if (_err != ESP_OK)
	{
		// error //
		return 2;
	}

	// installing i2c driver
    _err = i2c_driver_install(
			LCD_I2C_PORT,
			I2C_MODE_MASTER,
			LCD_I2C_FLAG_BUF_RX,
			LCD_I2C_FLAG_BUF_TX,
			LCD_I2C_FLAG_INTR
			);
	if (_err != ESP_OK)
	{
		// error //
		return 3;
	}

	lcd_init();

	LCD_IS_OPEN = true;

	return 0;
}


int
lcd_close()
{
	//
	// chk
	//
	
	if (!LCD_IS_OPEN)
	{
		return 5;
	}


	//
	// main
	//
	
	lcd_clear();
	lcd_home();
	lcd_switch(false, false, false);

	vTaskDelay(100 / portTICK_PERIOD_MS);

    i2c_driver_delete(LCD_I2C_PORT);


	LCD_IS_OPEN = false;

	return 0;
}


int
lcd_led(
		bool turn_on
	   )
{
	if (turn_on)
	{
		LCD_MASK_CMD_LOW = LCD_BYTE_CMD_W_ON_LOW;
		LCD_MASK_CMD_HIG = LCD_BYTE_CMD_W_ON_HIG;
		LCD_MASK_DAT_LOW = LCD_BYTE_DAT_W_ON_LOW;
		LCD_MASK_DAT_HIG = LCD_BYTE_DAT_W_ON_HIG;
	}
	else
	{
		LCD_MASK_CMD_LOW = LCD_BYTE_CMD_W_OF_LOW;
		LCD_MASK_CMD_HIG = LCD_BYTE_CMD_W_OF_HIG;
		LCD_MASK_DAT_LOW = LCD_BYTE_DAT_W_OF_LOW;
		LCD_MASK_DAT_HIG = LCD_BYTE_DAT_W_OF_HIG;
	}

	lcd_i2c_write_byte(LCD_MASK_CMD_LOW);


	return 0;
}


/*
 * it is required to call this to setup the lcd for 1 bit width communation
 * since xylobit circuit uses 3 pin setup for the lcd - 4-bit interface
 */
int
lcd_init()
{
	//
	// var
	//

	int _i = 0;
	// delay required for init_lcd:
	// 	> 15	ms
	// 	> 4.1	ms
	// 	> 100	us
	//
	// 	>> taking 20 ms for all
	int time_delay = 20;
	
	size_t len_lst_cmd = 28;
	uint8_t lst_cmd[] =
	{
		// the three Function Set
		0x3C, 0x38,
		0x3C, 0x38,
		0x3C, 0x38,

		// 4th function set - setting the interface to be 4 bits
		0x2C, 0x28,

		// from here interface is 4 bit

		// setting the font and num of lines - 5x8 dot, 2lines
		0x2C, 0x28,
		0x8C, 0x88,

		// Display off
		0x0C, 0x08,
		0x8C, 0x88,

		// Display clear
		0x0C, 0x08,
		0x1C, 0x18,

		// Entery mode set
		0x0C, 0x08,
		0x6C, 0x68,

		// Display ON + cursor
		0x0C, 0x08,
		0xEC, 0xE8,
	};


	//
	// main
	//
	
	// Function Set #1, 2, 3 - CMD: [0, 5]
	for (_i = 0; _i < 3; _i++)
	{
		vTaskDelay(time_delay / portTICK_PERIOD_MS);
		lcd_i2c_write_byte(lst_cmd[(_i * 2)]);
		lcd_i2c_write_byte(lst_cmd[(_i * 2) + 1]);
	}
	
	// set 4bit, lines+font, off, clear, entery, on
	for (_i = 6; _i < len_lst_cmd; _i++)
	{
		lcd_i2c_write_byte(lst_cmd[_i]);
	}

	lcd_led(true);


	LCD_IS_ON				= true;
	LCD_LED_IS_ON			= true;
	LCD_CURSOR_IS_ON		= true;
	LCD_CURSOR_IS_BLINKING	= false;

	return 0;
}


int
lcd_i2c_write_byte(
		const uint8_t byte_data
		)
{
	//
	// var
	//

    i2c_cmd_handle_t i2c_cmd_hdl = i2c_cmd_link_create();


	//
	// main
	//

	// >>>	starting the sequence ///
	// start bit
    i2c_master_start(i2c_cmd_hdl);

	// writing the LCD address
    i2c_master_write_byte(
			i2c_cmd_hdl,
			((LCD_I2C_ADDR << 1) | I2C_MASTER_WRITE),
			LCD_I2C_FLAG_ACK
			);

	// writing the data
	i2c_master_write_byte(
			i2c_cmd_hdl,
			byte_data,
			LCD_I2C_FLAG_ACK
			);
	
	// the stop bit
    i2c_master_stop(i2c_cmd_hdl);
	// <<<	end of the sequence ///

	// sending the sequence
    i2c_master_cmd_begin(
			LCD_I2C_PORT,
			i2c_cmd_hdl,
			(LCD_I2C_CMD_DELAY / portTICK_PERIOD_MS)
			);

    i2c_cmd_link_delete(i2c_cmd_hdl);


	return 0;
}


int
lcd_switch(
		bool display_on,
		bool cursor_on,
		bool cursor_blink
		)
{
	//
	// var
	//
	
	uint8_t byte_flag = 0x80;
	

	//
	// main
	//
	
	LCD_IS_ON = display_on;
	if (display_on)
	{
		byte_flag |= 0x40;
	}

	if (cursor_on)
	{
		byte_flag |= 0x20;
	}

	if (cursor_blink)
	{
		byte_flag |= 0x10;
	}

	lcd_i2c_write_byte(LCD_MASK_CMD_HIG);
	lcd_i2c_write_byte(LCD_MASK_CMD_LOW);
	lcd_i2c_write_byte(byte_flag | LCD_MASK_CMD_HIG);
	lcd_i2c_write_byte(byte_flag | LCD_MASK_CMD_LOW);
	

	return 0;
}

int
lcd_move_cursor(
		bool	move_right,
		uint8_t	num_units
		)
{
	//
	// var
	//
	
	uint8_t _i = 0;

	uint8_t byte_high	= 0x10;
	uint8_t byte_low	= 0x00;


	//
	// main
	//

	if (num_units > 56)
	{
		return 4;
	}

	if (move_right)
	{
		byte_low |= 0x40;
	}

	for (_i = 0; _i < num_units; _i++)
	{
		lcd_i2c_write_byte(byte_high	| LCD_MASK_CMD_HIG);
		lcd_i2c_write_byte(byte_high	| LCD_MASK_CMD_LOW);
		lcd_i2c_write_byte(byte_low		| LCD_MASK_CMD_HIG);
		lcd_i2c_write_byte(byte_low		| LCD_MASK_CMD_LOW);
	}


	return 0;
}


int
lcd_clear()
{
	lcd_i2c_write_byte(0x00 | LCD_MASK_CMD_HIG);
	lcd_i2c_write_byte(0x00 | LCD_MASK_CMD_LOW);
	lcd_i2c_write_byte(0x10 | LCD_MASK_CMD_HIG);
	lcd_i2c_write_byte(0x10 | LCD_MASK_CMD_LOW);

	return 0;
}


int
lcd_home()
{
	lcd_i2c_write_byte(0x00 | LCD_MASK_CMD_HIG);
	lcd_i2c_write_byte(0x00 | LCD_MASK_CMD_LOW);
	lcd_i2c_write_byte(0x20 | LCD_MASK_CMD_HIG);
	lcd_i2c_write_byte(0x20 | LCD_MASK_CMD_LOW);

	return 0;
}


int
lcd_write(
		const uint8_t	len_str_line1,
		const char*		str_line1,
		const uint8_t	len_str_line2,
		const char*		str_line2
		)
{
	//
	// var
	//
	
	uint8_t _i		= 0;
	uint8_t _line	= 0;

	uint8_t x_high	= 0;
	uint8_t x_low	= 0;

	const char* lst_lines[] = {
		str_line1,
		str_line2
	};

	const uint8_t lst_len[] = {
		len_str_line1,
		len_str_line2
	};


	//
	// chk
	//
	
	if (
			(len_str_line1 > LCD_LINE_WIDTH) ||
			(len_str_line2 > LCD_LINE_WIDTH)
	   )
	{
		return 1;
	}
	

	//
	// main
	//
	
	// clear
	lcd_clear();

	// return home
	lcd_home();
	
	// rewrite both lines
	for (_line = 0; _line < LCD_LINE_TOTAL; _line++)
	{
		for (_i = 0; _i < lst_len[_line]; _i++)
		{
			if (
					(lst_lines[_line][_i] < LCD_CHAR_LIM_LOW) ||
					(lst_lines[_line][_i] > LCD_CHAR_LIM_HIG)
			   )
			{
				return 2;
			}
			
			x_high	= lst_lines[_line][_i] & 0xF0;
			x_low	= (lst_lines[_line][_i] & 0x0F) << 4;

			lcd_i2c_write_byte(x_high	| LCD_MASK_DAT_HIG);
			lcd_i2c_write_byte(x_high	| LCD_MASK_DAT_LOW);
			lcd_i2c_write_byte(x_low	| LCD_MASK_DAT_HIG);
			lcd_i2c_write_byte(x_low	| LCD_MASK_DAT_LOW);
		}

		// go to line 2
		// 40 - lst_len[_line]
		lcd_move_cursor(
				true,
				(LCD_LINE_DISTANCE - lst_len[_line])
				);
	}


	return 0;
}

