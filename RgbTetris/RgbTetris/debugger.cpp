// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "debugger.h"

#include <stdint.h>
#include <stdlib.h>

#include "accelerometer.h"
#include "buttons.h"
#include "colors.h"
#include "move_helper.h"
#include "number_display.h"
#include "options.h"
#include "timer.h"
#include "ws2812_matrix.h"

namespace
{
constexpr uint8_t target_redraw_counter = 7;
constexpr uint8_t max_gradient_id = 4;
constexpr uint8_t gradient_step_count = 50;

void process_gradient(uint8_t& gradient_id, uint8_t& gradient_counter,
	uint8_t max_brightness, const util::coord& coord)
{
	color::rgb from { 0, 0, 0 }, to { 0, 0, 0 };
	switch(gradient_id)
	{
		case 0:
			from.r = 0xff;
			to.g = 0xff;
			break;
			
		case 1:
			from.g = 0xff;
			to.b = 0xff;
			break;
			
		case 2:
			from.b = 0xff;
			to.r = to.g = 0xff;
			break;
			
		case 3:
			from.r = from.g = 0xff;
			to.r = 0xff;
			break;
		
		default:
			return;
	}
	
	color::rgb rgb;
	color::gradient(from, to, gradient_step_count, gradient_counter, rgb);
	color::scale_to_brightness(rgb, max_brightness);
	ws2812_matrix::set_pixel_color(coord, rgb);
	ws2812_matrix::show();
	
	if(++gradient_counter == gradient_step_count)
	{
		if(++gradient_id == max_gradient_id)
			gradient_id = 0;
		
		gradient_counter = 0;
	}
}

void init(bool start)
{
	ws2812_matrix::clear();
	buttons::enable_repeat(buttons::mask_fwd | buttons::mask_back
		| buttons::mask_right | buttons::mask_left, start);
	buttons::flush_pressed();
}

void display_coords(const util::coord& coord)
{
	uint8_t number_display_buffer[number_display::max_digits + 1] = {};
	
	itoa(coord.x, reinterpret_cast<char*>(number_display_buffer + 1), 10);
	itoa(coord.y, reinterpret_cast<char*>(number_display_buffer + 3), 10);
	
	for(uint8_t i = 0; i != number_display::max_digits; ++i)
	{
		if(number_display_buffer[i])
		{
			number_display_buffer[i]
				= number_display::get_symbol_data(number_display_buffer[i] - '0');
		}
	}
	
	number_display::output_data(number_display_buffer);
}
}

void debugger::run()
{
	init(true);
	
	uint8_t new_x = 0, new_y = 0;
	uint8_t redraw_counter = 0;
	uint8_t max_brightness = options::get_max_brightness();
	uint8_t gradient_id = 0, gradient_counter = 0;
	bool accelerometer_enabled = options::is_accelerometer_enabled();
	accelerometer::speed_state x_speed_state(9), y_speed_state(9);
	
	util::coord coord { 0, 0 };
	display_coords(coord);
	
	while(true)
	{
		timer::wait_for_interrupt();
		
		auto button_up_state = buttons::get_button_status(buttons::button_up);
		auto button_down_state = buttons::get_button_status(buttons::button_down);
		if(button_up_state == buttons::button_status_still_pressed
			&& button_down_state == buttons::button_status_still_pressed)
		{
			break;
		}
		
		new_x = coord.x;
		new_y = coord.y;
		
		uint8_t move_dir = move_helper::process_speed(&x_speed_state, &y_speed_state, accelerometer_enabled);
		if(move_dir & move_direction_left)
		{
			if(new_x < ws2812_matrix::width - 1)
				++new_x;
		}	
		else if(move_dir & move_direction_right)
		{
			if(new_x)
				--new_x;
		}
			
		if(move_dir & move_direction_up)
		{
			if(new_y < ws2812_matrix::height - 1)
				++new_y;
		}
		else if(move_dir & move_direction_down)
		{
			if(new_y)
				--new_y;
		}
		
		if(new_x != coord.x || new_y != coord.y)
		{
			ws2812_matrix::clear_pixel(coord);
			coord.x = new_x;
			coord.y = new_y;
			redraw_counter = target_redraw_counter - 1;
			display_coords(coord);
		}
		
		if(++redraw_counter == target_redraw_counter)
		{
			redraw_counter = 0;
			process_gradient(gradient_id, gradient_counter, max_brightness, coord);
		}
	}
	
	init(false);
}
