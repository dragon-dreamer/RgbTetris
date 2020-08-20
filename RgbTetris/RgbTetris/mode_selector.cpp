// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "mode_selector.h"

#include <stdlib.h>

#include <avr/pgmspace.h>

#include "adxl345.h"
#include "bitmap.h"
#include "buttons.h"
#include "colors.h"
#include "font.h"
#include "number_display.h"
#include "options.h"
#include "timer.h"
#include "ws2812_matrix.h"

namespace
{
const char mode_snake_text[] PROGMEM = "SNAKE";
const char mode_space_invaders_text[] PROGMEM = "SPACE INVADERS";
const char mode_tetris_text[] PROGMEM = "TETRIS";
const char mode_asteroids_text[] PROGMEM = "ASTEROIDS";
const char mode_maze_text[] PROGMEM = "MAZE";
const char mode_options_text[] PROGMEM = "OPTIONS";
const char mode_uart_text[] PROGMEM = "UART";
const char mode_debug_text[] PROGMEM = "DEBUG";

const char option_accelerometer_text[] PROGMEM = "ACCELEROMETER";
const char option_brightness_text[] PROGMEM = "BRIGHTNESS";
const char option_reset_high_scores_text[] PROGMEM = "RESET SCORES";

const char* const mode_text_table[] PROGMEM =
{
	mode_snake_text,
	mode_space_invaders_text,
	mode_tetris_text,
	mode_asteroids_text,
	mode_maze_text,
	mode_options_text,
	mode_uart_text,
	mode_debug_text,
	
	option_accelerometer_text,
	option_brightness_text,
	option_reset_high_scores_text
};

static_assert(sizeof(mode_text_table) / sizeof(mode_text_table[0])
	== mode_selector::max_mode + mode_selector::max_option,
	"Something is wrong with mode_text_table/option_text_table or mode/option enumeration");

uint8_t max_brightness = options::default_max_brightness;
bool is_accelerometer_enabled = false;
bool reset_high_score = false;
uint8_t current_letter = 0;
uint8_t symbol_part = 0;
color::rgb letter_color { 0, 0, 0 };
bool need_redraw = false;
uint8_t rgb_wheel_pos = 0;

constexpr uint8_t text_height = ws2812_matrix::height > 16 ? 12 : 7;
constexpr uint8_t min_color_component_brightness = 5;

void init_letter_color()
{
	symbol_part = 0;
	letter_color = color::get_random_color(min_color_component_brightness, max_brightness);
}

void output_menu_letter(uint8_t item, bool reset)
{
	if(reset)
	{
		ws2812_matrix::clear();
		current_letter = 0;
		init_letter_color();
	}
	
	ws2812_matrix::shift_left(text_height, text_height + font::symbol_height);
	
	const char* text = reinterpret_cast<const char*>(pgm_read_word(&mode_text_table[item]));
	const char value = pgm_read_byte(text + current_letter);
	
	if(value && symbol_part < font::symbol_width)
		font::output_symbol_part(value, 4 - symbol_part, ws2812_matrix::axis_id::x, 0, text_height, letter_color);
	
	need_redraw = true;
	
	if(++symbol_part > font::symbol_width + 1)
	{
		init_letter_color();
		current_letter = value ? current_letter + 1 : 0;
	}
}

const uint8_t empty_checkbox_bitmap[] PROGMEM { 0x66, 0x00, 0x7f, 0x18, 0x86, 0xe1, 0x0f };
const uint8_t checkbox_check_bitmap[] PROGMEM { 0x44, 0xff, 0x69, 0x96 };

constexpr uint8_t checkbox_offset_x = ws2812_matrix::width > 6 ? (ws2812_matrix::width - 6) / 2 : 0;
constexpr uint8_t checkbox_offset_y = text_height - 6;

void show_checkbox(bool enabled)
{
	bitmap::display_bitmap_P(&empty_checkbox_bitmap[0], checkbox_offset_x, checkbox_offset_y, 0, 0,
		color::scale_to_brightness(150, max_brightness));
	if(enabled)
	{
		bitmap::display_bitmap_P(&checkbox_check_bitmap[0], checkbox_offset_x + 1, checkbox_offset_y + 1,
			color::scale_to_brightness(200, max_brightness), 0, 0);
	}
}

void show_accelerometer_checkbox()
{
	show_checkbox(is_accelerometer_enabled);
}

void show_reset_high_score_checkbox()
{
	show_checkbox(reset_high_score);
}

void process_accelerometer()
{
	if(buttons::is_pressed(buttons::button_right))
	{
		is_accelerometer_enabled = !is_accelerometer_enabled;
		show_accelerometer_checkbox();
	}
}

void draw_brightness_value()
{
	number_display::output_number(max_brightness);
}

void draw_brightness_demo_part()
{
	color::rgb rgb;
	ws2812_matrix::shift_right(1, 6);
	for(uint8_t y = 1; y != 7; ++y)
	{
		color::rgb_color_wheel(rgb_wheel_pos++, rgb);
		color::scale_to_brightness(rgb, max_brightness);
		ws2812_matrix::set_pixel_color(ws2812_matrix::width - 1, y, rgb);
	}
}

constexpr uint8_t target_brightness_demo_counter = 5;
void process_brightness(uint8_t& brightness_demo_counter)
{
	if(buttons::is_pressed(buttons::button_fwd))
	{
		if(max_brightness < options::max_available_brightness)
		{
			++max_brightness;
			draw_brightness_value();
		}
	}
	else if(buttons::is_pressed(buttons::button_back))
	{
		if(max_brightness > options::min_available_brightness)
		{
			--max_brightness;
			draw_brightness_value();
		}
	}
	
	if(++brightness_demo_counter == target_brightness_demo_counter)
	{
		brightness_demo_counter = 0;
		draw_brightness_demo_part();
		need_redraw = true;
	}
}

void process_reset_high_scores()
{
	if(buttons::is_pressed(buttons::button_right))
	{
		reset_high_score = !reset_high_score;
		show_reset_high_score_checkbox();
	}
}

void option_changed(mode_selector::option new_option)
{
	buttons::flush_pressed();
	number_display::clear();
	
	switch(new_option)
	{
		case mode_selector::option_accelerometer:
			show_accelerometer_checkbox();
			break;
			
		case mode_selector::option_brightness:
			draw_brightness_value();
			for(uint8_t i = 0; i != ws2812_matrix::width; ++i)
				draw_brightness_demo_part();
			break;
		
		case mode_selector::option_reset_high_scores:
			show_reset_high_score_checkbox();
			break;
		
		default:
			break;
	}
}

void redraw_if_needed()
{
	if(need_redraw)
	{
		need_redraw = false;
		ws2812_matrix::show();
	}
}

void output_high_score(mode_selector::mode mode, uint8_t point_mask)
{
	//mode_selector::mode and game::game_id are interchangeable
	if(mode < static_cast<uint8_t>(game::max_game_id))
		number_display::output_number(options::get_high_score(static_cast<game::game_id>(mode)), point_mask);
	else
		number_display::output_number(0, point_mask, true);
}
} //namespace

constexpr uint8_t target_led_count = 25;
constexpr uint8_t target_menu_letter_count = 9;
void mode_selector::edit_options()
{
	current_letter = 0;
	reset_high_score = false;
	init_letter_color();
	
	buttons::enable_repeat(buttons::mask_fwd | buttons::mask_back, true);
	
	uint8_t menu_letter_count = 0;
	uint8_t brightness_demo_counter = 0;
	
	uint8_t current_mode = option_accelerometer;
	option_changed(static_cast<option>(current_mode));
	
	while(!buttons::is_pressed(buttons::button_left))
	{
		timer::wait_for_interrupt();
		
		if(buttons::is_pressed(buttons::button_up))
		{
			if(current_mode != max_option - 1)
			{
				menu_letter_count = 0;
				++current_mode;
				output_menu_letter(current_mode + max_mode, true);
				option_changed(static_cast<option>(current_mode));
			}
		}
		else if(buttons::is_pressed(buttons::button_down))
		{
			if(current_mode != 0)
			{
				menu_letter_count = 0;
				--current_mode;
				output_menu_letter(current_mode + max_mode, true);
				option_changed(static_cast<option>(current_mode));
			}
		}
		
		switch(current_mode)
		{
			case option::option_accelerometer:
				process_accelerometer();
				break;
			
			case option::option_brightness:
				process_brightness(brightness_demo_counter);
				break;
				
			case option::option_reset_high_scores:
				process_reset_high_scores();
				break;
		}
		
		if(++menu_letter_count == target_menu_letter_count)
		{
			menu_letter_count = 0;
			output_menu_letter(current_mode + max_mode, false);
		}
	
		redraw_if_needed();
	}
	
	ws2812_matrix::clear();
	ws2812_matrix::show();
	
	buttons::enable_repeat(buttons::mask_fwd | buttons::mask_back, false);
	
	options::set_accelerometer_enabled(is_accelerometer_enabled);
	options::set_max_brightness(max_brightness);
	adxl345::enable_measurements(is_accelerometer_enabled);
	
	if(reset_high_score)
		options::reset_high_scores();
}

mode_selector::mode mode_selector::select_mode()
{
	max_brightness = options::get_max_brightness();
	is_accelerometer_enabled = options::is_accelerometer_enabled();
	
	current_letter = 0;
	init_letter_color();
	
	uint8_t point_mask = 1;
	uint8_t current_mode = mode_snake;
	output_high_score(static_cast<mode>(current_mode), point_mask);
	
	uint8_t led_count = 0;
	uint8_t menu_letter_count = 0;
	uint16_t rand_seed = rand();
	
	while(!buttons::is_pressed(buttons::button_right))
	{
		timer::wait_for_interrupt();
		
		if(buttons::is_pressed(buttons::button_up))
		{
			srand(rand_seed);
			if(current_mode != max_mode - 1)
			{
				menu_letter_count = 0;
				++current_mode;
				output_menu_letter(current_mode, true);
				output_high_score(static_cast<mode>(current_mode), point_mask);
			}
		}
		else if(buttons::is_pressed(buttons::button_down))
		{
			srand(rand_seed);
			if(current_mode != 0)
			{
				menu_letter_count = 0;
				--current_mode;
				output_menu_letter(current_mode, true);
				output_high_score(static_cast<mode>(current_mode), point_mask);
			}
		}
		
		if(++led_count == target_led_count)
		{
			led_count = 0;
			output_high_score(static_cast<mode>(current_mode), point_mask);
			point_mask <<= 1;
			if(point_mask == (1 << number_display::max_digits))
				point_mask = 1;
		}
		
		if(++menu_letter_count == target_menu_letter_count)
		{
			menu_letter_count = 0;
			output_menu_letter(current_mode, false);
		}
		
		++rand_seed;
		
		redraw_if_needed();
	}
	
	ws2812_matrix::clear();
	ws2812_matrix::show();
	number_display::clear();
	
	srand(rand_seed);
	
	return static_cast<mode>(current_mode);
}
