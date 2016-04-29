// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "bitmap.h"

#include <avr/pgmspace.h>

#include "ws2812_matrix.h"

namespace
{
const uint8_t* read_format(const uint8_t* data, uint8_t& width, uint8_t& height, bool& opaque)
{
	uint8_t header = pgm_read_byte(data++);
	uint8_t flags = pgm_read_byte(data++);
	opaque = !reinterpret_cast<bitmap::bitmap_flags*>(&flags)->transparent;
	width = reinterpret_cast<bitmap::bitmap_header*>(&header)->width;
	height = reinterpret_cast<bitmap::bitmap_header*>(&header)->height;
	return data;
}
} //namespace

void bitmap::display_bitmap_P(const uint8_t* data, uint8_t pos_x, uint8_t pos_y,
	uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t width, height;
	bool opaque;
	data = read_format(data, width, height, opaque);
	uint8_t color_byte = pgm_read_byte(data++);
	uint8_t current_bit = 0;
	for(uint8_t x = 0; x != width; ++x)
	{
		for(uint8_t y = 0; y != height; ++y)
		{
			if(color_byte & (1 << current_bit))
				ws2812_matrix::set_pixel_color(pos_x + x, pos_y + y, r, g, b);
			else if(opaque)
				ws2812_matrix::clear_pixel(pos_x + x, pos_y + y);
			
			if(++current_bit == 8)
			{
				color_byte = pgm_read_byte(data++);
				current_bit = 0;
			}
		}
	}
}

void bitmap::display_bitmap_P(const uint8_t* data, uint8_t pos_x, uint8_t pos_y, uint8_t brightness)
{
	uint8_t width, height;
	bool opaque;
	data = read_format(data, width, height, opaque);
	uint8_t color_byte[3];
	uint8_t r, g, b;
	bool odd_led = true;
	for(uint8_t x = 0; x != width; ++x)
	{
		for(uint8_t y = 0; y != height; ++y)
		{
			if(odd_led)
			{
				color_byte[0] = pgm_read_byte(data++);
				color_byte[1] = pgm_read_byte(data++);
				color_byte[2] = pgm_read_byte(data++);
				r = color_byte[0] >> 4;
				g = color_byte[0] & 0x0f;
				b = color_byte[1] >> 4;
			}
			else
			{
				r = color_byte[1] & 0x0f;
				g = color_byte[2] >> 4;
				b = color_byte[2] & 0x0f;
			}
			
			odd_led = !odd_led;
			
			if(r || g || b)
			{
				r = (static_cast<uint16_t>(r) * 17 * brightness) >> 8;
				g = (static_cast<uint16_t>(g) * 17 * brightness) >> 8;
				b = (static_cast<uint16_t>(b) * 17 * brightness) >> 8;
				
				ws2812_matrix::set_pixel_color(pos_x + x, pos_y + y, r, g, b);
			}
			else if(opaque)
			{
				ws2812_matrix::clear_pixel(pos_x + x, pos_y + y);
			}
		}
	}
}