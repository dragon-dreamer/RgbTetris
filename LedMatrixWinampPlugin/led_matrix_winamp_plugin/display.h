// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <array>
#include <stdint.h>

#include "colors.h"

///RGB display matrix wrapper
class display
{
public:
	static const uint32_t display_width = 10;
	static const uint32_t display_height = 16;
	static const uint32_t bytes_per_led = 3;

	typedef std::array<std::array<color::rgb, display_width>, display_height> display_matrix;

public:
	///Creates empty display (filled with black color)
	display();

	///Returns raw display data
	const display::display_matrix& get_data() const;
	
	///Sets separate pixel color
	void set_pixel(uint8_t x, uint8_t y, const color::rgb& color);
	
	///Returns separate pixel color
	color::rgb get_pixel(uint8_t x, uint8_t y) const;
	
	///Fills matrix with black color
	void clear();

private:
	display_matrix matrix_;
};
