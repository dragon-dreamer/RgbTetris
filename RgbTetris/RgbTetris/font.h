// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "colors.h"
#include "static_class.h"
#include "ws2812_matrix.h"

class font : static_class
{
public:
	static constexpr uint8_t symbol_width = 5;
	static constexpr uint8_t symbol_height = 8;
	
public:
	///Outputs symbol column (part = 0 to symbol_width - 1),
	///where zero_coord is symbol offset from display border, and text_shift is letter shift
	static void output_symbol_part(char symbol, uint8_t part, ws2812_matrix::axis_id axis,
		uint8_t zero_coord, uint8_t text_shift, const color::rgb& letter_color);
};
