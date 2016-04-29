// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "display.h"

display::display()
{
	clear();
}

const display::display_matrix& display::get_data() const
{
	return matrix_;
}

void display::set_pixel(uint8_t x, uint8_t y, const color::rgb& color)
{
	if(x >= display_width || y >= display_height)
		return;

	matrix_[y][x] = color;
}

color::rgb display::get_pixel(uint8_t x, uint8_t y) const
{
	if(x >= display_width || y >= display_height)
		return color::rgb();

	return matrix_[y][x];
}

void display::clear()
{
	memset(matrix_.data(), 0, sizeof(matrix_));
}
