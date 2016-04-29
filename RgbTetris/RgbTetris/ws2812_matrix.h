// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "colors.h"
#include "static_class.h"
#include "util.h"

///LED display class
class ws2812_matrix : static_class
{
public:
	static constexpr uint8_t width = 10;
	static constexpr uint8_t height = 16;
	static constexpr uint8_t bytes_per_led = 3;
	static constexpr uint16_t byte_count = width * height * bytes_per_led;
	
public:
	static const uint8_t r_offset = 1;
	static const uint8_t g_offset = 0;
	static const uint8_t b_offset = 2;

public:
	enum class axis_id : uint8_t
	{
		x, y
	};
	
public:
	static void init();

	static void set_pixel_color(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
	static void set_pixel_color(uint8_t x, uint8_t y, const color::rgb& rgb);
	static void set_pixel_color(const util::coord& coord, const color::rgb& rgb);
	static void clear_pixel(uint8_t x, uint8_t y);
	static void clear_pixel(const util::coord& coord);
	static void get_pixel_color(uint8_t x, uint8_t y, uint8_t& r, uint8_t& g, uint8_t& b);
	static void copy_color(uint8_t from_x, uint8_t from_y, uint8_t to_x, uint8_t to_y);
	static void set_pixel_color(uint8_t x, uint8_t y, uint32_t color);
	static void clear();
	static void clear_horizontal_lines(uint8_t y_from, uint8_t y_to);
	
	static uint32_t get_pixel_color(uint8_t x, uint8_t y);
	static uint8_t* get_pixels();
	
	//No bound checks
	static void set_pixel_color_fast(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
	static void get_pixel_color_fast(uint8_t x, uint8_t y, uint8_t& r, uint8_t& g, uint8_t& b);
	static void set_pixel_color_fast(uint8_t x, uint8_t y, uint32_t color);
	static void set_pixel_color_fast(const util::coord& coords, const color::rgb& rgb);
	static uint32_t get_pixel_color_fast(uint8_t x, uint8_t y);
	
	static bool is_on(uint8_t x, uint8_t y);
	static bool is_on(const util::coord& coord);
	static bool is_on_fast(uint8_t x, uint8_t y);
	
	static void show();

public:
	static void shift_left();
	static void shift_right();
	static void shift_up();
	static void shift_down();
	
	static void shift_left(uint8_t y_from, uint8_t y_to);
	static void shift_right(uint8_t y_from, uint8_t y_to);
	static void shift_up(uint8_t x_from, uint8_t x_to);
	static void shift_down(uint8_t x_from, uint8_t x_to);
};
