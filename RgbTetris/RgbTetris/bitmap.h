// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

class bitmap : static_class
{
public:
	struct bitmap_header
	{
		uint8_t width : 4;
		uint8_t height : 4;
	};
	
	struct bitmap_flags
	{
		uint8_t transparent : 1;
	};
	
	struct bitmap_data
	{
		bitmap_header header;
		bitmap_flags flags;
		uint8_t* data;
	};
	
public:
	/** Displays monochrome bitmap at specified coords
	*
	*   format:
	*   - first uint8_t = [width_4bit|height_4bit]
	*   - next flags
	*   - color bytes follow. Each byte has 8 bits [1 = pixel is ON, 0 = pixel is OFF]
	*   @param data Image data in program memory
	*   @param x Display X coord
	*   @param y Display Y coord
	*   @param r Red color component
	*   @param g Green color component
	*   @param b Blue color component */
	static void display_bitmap_P(const uint8_t* data, uint8_t x, uint8_t y,
		uint8_t r, uint8_t g, uint8_t b);
	
	/** Displays color bitmap at specified coords
	*
	*   format:
	*   - first uint8_t = [width_4bit|height_4bit]
	*   - next flags
	*   - color bytes (rgb: 4 bits per color, 3 bytes per 2 leds)
	*   @param data Image data in program memory
	*   @param x Display X coord
	*   @param y Display Y coord
	*   @param brightness Color brightness */
	static void display_bitmap_P(const uint8_t* data, uint8_t x, uint8_t y, uint8_t brightness);
};
