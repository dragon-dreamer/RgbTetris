// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "colors.h"

#include <stdlib.h>

uint32_t color::get_random_color(uint8_t min_component_value, uint8_t max_component_value)
{
	max_component_value -= min_component_value;
	return make_rgb(min_component_value + rand() % max_component_value,
		min_component_value + rand() % max_component_value,
		min_component_value + rand() % max_component_value);
}

void color::rgb_color_wheel(uint8_t wheel_pos, uint8_t& r, uint8_t& g, uint8_t& b)
{
	if(wheel_pos < 85)
	{
		g = 0;
		b = wheel_pos * 3;
		r = 255 - b;
		return;
	}

	if(wheel_pos < 170)
	{
		wheel_pos -= 85;
		r = 0;
		g = wheel_pos * 3;
		b = 255 - g;
		return;
	}

	wheel_pos -= 170;
	b = 0;
	r = wheel_pos * 3;
	g = 255 - r;
}

void color::rgb_color_wheel(uint8_t wheel_pos, rgb& value)
{
	rgb_color_wheel(wheel_pos, value.r, value.g, value.b);
}

void color::scale_to_brightness(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t brightness)
{
	r = (static_cast<uint16_t>(r)* brightness) >> 8;
	g = (static_cast<uint16_t>(g)* brightness) >> 8;
	b = (static_cast<uint16_t>(b)* brightness) >> 8;
}

void color::scale_to_brightness(rgb& value, uint8_t brightness)
{
	scale_to_brightness(value.r, value.g, value.b, brightness);
}

uint32_t color::scale_to_brightness_rgb(uint32_t color, uint8_t brightness)
{
	uint8_t r, g, b;
	from_rgb(color, r, g, b);
	scale_to_brightness(r, g, b, brightness);
	return make_rgb(r, g, b);
}

void color::gradient(uint8_t r_from, uint8_t g_from, uint8_t b_from,
	uint8_t r_to, uint8_t g_to, uint8_t b_to,
	uint32_t step_count, uint32_t current_step,
	uint8_t& r, uint8_t& g, uint8_t& b)
{
	if(current_step > step_count)
		current_step = 2 * step_count - current_step;

	const double pos = static_cast<float>(current_step) / step_count;
	r = static_cast<uint8_t>(r_from + (r_to - r_from) * pos);
	g = static_cast<uint8_t>(g_from + (g_to - g_from) * pos);
	b = static_cast<uint8_t>(b_from + (b_to - b_from) * pos);
}

void color::gradient(const rgb& from, const rgb& to,
	uint32_t step_count, uint32_t current_step,
	rgb& value)
{
	gradient(from.r, from.g, from.b, to.r, to.g, to.b, step_count, current_step,
		value.r, value.g, value.b);
}
