// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "effect.h"

#include <avr/pgmspace.h>

#include "colors.h"
#include "font.h"
#include "options.h"
#include "util.h"
#include "ws2812_matrix.h"

namespace
{
void marquee_internal(char symbol, effect::direction dir, uint8_t text_shift,
	uint16_t delay, bool last)
{
	uint8_t max_brightness = options::get_max_brightness();
	ws2812_matrix::axis_id axis;
	uint8_t init_letter_coord;
	using scroll_func = void (*)();
	scroll_func scroll;
	bool invert_letter = false;
	switch(dir)
	{
		case effect::direction::back:
			axis = ws2812_matrix::axis_id::y;
			init_letter_coord = ws2812_matrix::height - 1;
			scroll = &ws2812_matrix::shift_down;
			invert_letter = true;
			break;
		
		case effect::direction::fwd:
			axis = ws2812_matrix::axis_id::y;
			init_letter_coord = 0;
			scroll = &ws2812_matrix::shift_up;
			break;
		
		case effect::direction::left:
			axis = ws2812_matrix::axis_id::x;
			init_letter_coord = 0;
			scroll = &ws2812_matrix::shift_left;
			invert_letter = true;
			break;
		
		default: //right
			axis = ws2812_matrix::axis_id::x;
			init_letter_coord = ws2812_matrix::width - 1;
			scroll = &ws2812_matrix::shift_right;
			break;
	}
	
	auto current_letter_color = color::get_random_color(5, max_brightness);
		
	for(uint8_t i = 0; i != font::symbol_width; ++i)
	{
		scroll();
		font::output_symbol_part(symbol, invert_letter ? font::symbol_width - i - 1 : i,
			axis, init_letter_coord, text_shift, current_letter_color);
		ws2812_matrix::show();
		util::delay(delay);
	}
		
	scroll();
	ws2812_matrix::show();
	util::delay(delay);
	
	if(last)
	{
		uint8_t target_coord = axis == ws2812_matrix::axis_id::y
			? ws2812_matrix::height : ws2812_matrix::width;
		for(uint8_t i = 0; i != target_coord; ++i)
		{
			scroll();
			ws2812_matrix::show();
			util::delay(delay);
		}
	}
}
} //namespace

void effect::marquee_P(const char* text, direction dir, uint8_t text_shift,
	uint16_t delay)
{
	char symbol;
	while(symbol = pgm_read_byte(text++))
		marquee_internal(symbol, dir, text_shift, delay, !pgm_read_byte(text));
}

void effect::marquee(const char* text, direction dir, uint8_t text_shift,
	uint16_t delay)
{
	while(*text)
	{
		const char symbol = *text++;
		marquee_internal(symbol, dir, text_shift, delay, !*text);
	}
}
