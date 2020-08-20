/** MEGA TETRIS board code.
*   Contains 5 games (snake, space invaders, tetris, asteroids, maze), UART and DEBUG modes.
*   All games can be controlled using buttons or accelerometer. All of them can be paused.
*   All games save high scores, which are displayed in main menu.
*
*   Copyright (c) 2016 Denis T (dragondreamer); https://github.com/dragon-dreamer/RgbTetris
*   Feel free to contact me:
*     dragondreamer [ @ ] live.com.
*
*   This code is developed by me (except some modules and code fragments, where this is explicitly noted).
*   I also developed and tested schematic and PCB for this firmware.
*   Moreover, you can find Winamp plugin that uses UART mode to display some nice effects on RGB LED display
*   when playing music. This plugin is also my work.
*   There is also Board Debug Console software written in C# for Windows, which can help debug
*   board hardware and software. 
*
*    SPDX-License-Identifier: GPL-3.0 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>

#include <util/delay.h>

#include "accelerometer.h"
#include "adxl345.h"
#include "buttons.h"
#include "debugger.h"
#include "flight.h"
#include "i2c_master.h"
#include "mode_selector.h"
#include "maze.h"
#include "number_display.h"
#include "options.h"
#include "options_reset.h"
#include "tetris.h"
#include "snake.h"
#include "space_invaders.h"
#include "timer.h"
#include "uart.h"
#include "ws2812_matrix.h"

namespace
{
void ioinit()
{
	//Pull-ups on floating/unused/UART pins
	PORTA |= _BV(PA4) | _BV(PA5) | _BV(PA6) | _BV(PA7);
	PORTB = 0xff;
	PORTD = 0xff;
	
	/** All input on PORTC
	*  PC0, PC1 - i2c
	*  PC2 - PC7 - buttons */
	
	ACSR |= _BV(ACD); //Turn analogue comparator off
	
	power_adc_disable();
	power_usart0_disable();
}
} //namespace

int main()
{
	//Initialize ports and disable unneeded devices
	ioinit();
	//Initialize number display
	number_display::init();
	//Initialize LED display matrix
	ws2812_matrix::init();
	//Clear number display
	number_display::clear();
	
	//Initialize 144 Hz timer
	timer::init();
	//Initialize buttons
	buttons::init();
	
	//Enable interrupts
	sei();
	
	_delay_ms(200);
	
	//Initialize TWI
	i2c_master::init();
	//Initialize adxl345 accelerometer
	if(adxl345::begin())
	{
		adxl345::enable_measurements(options::is_accelerometer_enabled());
	}
	else
	{
		options::set_accelerometer_enabled(false);
		adxl345::enable_measurements(false);
	}
	
	//Try to set range anyway, as user can try to enable accelerometer later
	adxl345::set_range(adxl345::range::range_2g);
	//Initialize accelerometer high-level wrapper
	accelerometer::init();
	
	//Clear LED matrix
	ws2812_matrix::clear();
	ws2812_matrix::show();
	
	//Reset saved options, if "up" and "down" buttons
	//are pressed simultaneously
	options_reset::reset_if_needed();
	
	//Reset pressed buttons states
	buttons::flush_pressed();
	
	//Main loop
	while(true)
	{
		//Scroll through options: "up" and "down" buttons
		//Select option: "right" button
		buttons::flush_pressed();
		auto res = mode_selector::select_mode();
		buttons::flush_pressed();
		switch(res)
		{
			case mode_selector::mode_options:
				//Scroll through options: "up" and "down" buttons
				//Back to main menu: "left" button
				//Change checkbox value: "right" button"
				//Change brightness level: "fwd" and "back" buttons
				mode_selector::edit_options();
				break;
			
			case mode_selector::mode_snake:
				//Pause any game: "up" and "down" buttons simultaneously
				//Resume game: "right" button
				//Exit game (from pause menu): "left" button
				//All games support both accelerometer and button control.
				snake::run();
				break;
				
			case mode_selector::mode_space_invaders:
				space_invaders::run();
				break;
				
			case mode_selector::mode_tetris:
				tetris::run();
				break;
				
			case mode_selector::mode_asteroids:
				flight::run();
				break;
				
			case mode_selector::mode_maze:
				maze::run();
				break;
				
			case mode_selector::mode_uart:
				//Exit UART mode: "up" and "down" buttons simultaneously
				uart::run();
				break;
			
			case mode_selector::mode_debug:
				//Exit debug mode: "up" and "down" buttons simultaneously
				debugger::run();
				break;
			
			default:
				break;
		}
	}
}
