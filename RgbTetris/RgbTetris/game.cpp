// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "game.h"

#include <string.h>
#include <stdlib.h>

#include <avr/pgmspace.h>

#include "buttons.h"
#include "colors.h"
#include "effect.h"
#include "number_display.h"
#include "options.h"
#include "queue.h"
#include "util.h"
#include "ws2812_matrix.h"

void game::intro()
{
	ws2812_matrix::clear();
	number_display::output_number(0);
	
	effect::marquee_P(PSTR("3"), effect::direction::left, 4, 200);
	util::delay(1000);
	effect::marquee_P(PSTR("2"), effect::direction::right, 4, 200);
	util::delay(1000);
	effect::marquee_P(PSTR("1"), effect::direction::left, 4, 200);
	ws2812_matrix::clear();
	buttons::flush_pressed();
}

void game::end(uint32_t score, game_id id)
{
	ws2812_matrix::clear();
	
	effect::marquee_P(PSTR("GAME OVER!"), effect::direction::left, 4, 250);
	util::delay(1000);
	
	char buf[24];
	
	if(score > options::get_high_score(id))
	{
		options::set_high_score(id, score);
		strcpy_P(buf, PSTR("NEW HIGH SCORE: "));
	}
	else
	{
		strcpy_P(buf, PSTR("SCORE: "));
	}
	
	{
		char score_buf[8];
		ltoa(score, score_buf, 10);
		strcat(buf, score_buf);
	}
	
	effect::marquee(buf, effect::direction::left, 4, 200);
	ws2812_matrix::clear();
	buttons::flush_pressed();
}

namespace
{
constexpr uint8_t max_pause_counter = 20;
bool interruptible_demo()
{
	int16_t x = (rand() % ws2812_matrix::width) * 16;
	int16_t y = x;
	int8_t speed_x = -8 - rand() % 9;
	int8_t speed_y = speed_x;
	uint8_t wheel = 0;
	queue<16, util::packed_coord> pixel_queue;
	uint8_t pixel_x, pixel_y;
	color::rgb rgb;
	uint8_t max_brightness = options::get_max_brightness();
	
	uint8_t pause_counter = 0;
	const uint8_t pause_text[] = {
		0b11100101, //P
		0b11101101, //A
		0b01111001, //U
		0b10111100, //S
		0b11110100  //E
	};
	
	while(true)
	{
		x += speed_x;
		if(x >= (ws2812_matrix::width - 1) * 16)
		{
			x = (ws2812_matrix::width - 1) * 16;
			speed_x = -8 - rand() % 9;
		}
		else if(x <= 0)
		{
			x = 0;
			speed_x = 8 + rand() % 9;
		}
		
		y += speed_y;
		if(y >= (ws2812_matrix::height - 1) * 16)
		{
			y = (ws2812_matrix::height - 1) * 16;
			speed_y = -8 - rand() % 9;
		}
		else if(y <= 0)
		{
			y = 0;
			speed_y = 8 + rand() % 9;
		}
		
		color::rgb_color_wheel(++wheel, rgb);
		color::scale_to_brightness(rgb, max_brightness);
		
		pixel_x = static_cast<uint8_t>(x / 16);
		pixel_y = static_cast<uint8_t>(y / 16);
		ws2812_matrix::set_pixel_color(pixel_x, pixel_y, rgb);
		
		if(pixel_queue.full())
		{
			util::packed_coord coord;
			if(pixel_queue.pop_front(coord))
			{
				util::packed_coord next_coord;
						
				bool clear = true;
				uint8_t i = 0;
				while(pixel_queue.get(next_coord, i++))
				{
					if(next_coord == coord)
					{
						clear = false;
						break;
					}
				}
				
				if(clear)
					ws2812_matrix::clear_pixel(coord.x, coord.y);
			}
		}
		
		pixel_queue.push_back({ pixel_x, pixel_y });
		
		ws2812_matrix::show();
		if(++pause_counter == max_pause_counter)
		{
			number_display::output_data(pause_text);
		}
		else if(pause_counter == max_pause_counter * 2)
		{
			pause_counter = 0;
			number_display::output_number(0, 0, true);
		}
		
		if(buttons::is_pressed(buttons::button_right))
			return true;
		else if(buttons::is_pressed(buttons::button_left))
			return false;
		
		util::delay(100);
	}
}

const color::rgb game_colors[] PROGMEM = {
	color::from_rgb(color::red), color::from_rgb(color::lime), color::from_rgb(color::blue),
	color::from_rgb(color::yellow), color::from_rgb(color::aqua), { 0xff, 0, 0x80 },
	color::from_rgb(color::white), color::from_rgb(color::darkblue), color::from_rgb(color::darkgreen),
	color::from_rgb(color::teal), color::from_rgb(color::springgreen), color::from_rgb(color::midnightblue),
	color::from_rgb(color::lightseagreen), color::from_rgb(color::dodgerblue),
	color::from_rgb(color::forestgreen), color::from_rgb(color::turquoise),
	color::from_rgb(color::steelblue), color::from_rgb(color::indigo), color::from_rgb(color::olivedrab),
	color::from_rgb(color::darkolivegreen), color::from_rgb(color::mediumaquamarine),
	color::from_rgb(color::lawngreen), color::from_rgb(color::purple), color::from_rgb(color::blueviolet),
	color::from_rgb(color::darkred), color::from_rgb(color::darkorange),
	color::from_rgb(color::orange), color::from_rgb(color::yellowgreen),
	color::from_rgb(color::saddlebrown), color::from_rgb(color::lightgreen), color::from_rgb(color::sienna),
	color::from_rgb(color::brown), color::from_rgb(color::greenyellow), color::from_rgb(color::firebrick),
	color::from_rgb(color::darkgoldenrod), color::from_rgb(color::peru), color::from_rgb(color::chocolate),
	color::from_rgb(color::goldenrod), color::from_rgb(color::crimson), color::from_rgb(color::palevioletred),
	color::from_rgb(color::sandybrown), color::from_rgb(color::wheat), color::from_rgb(color::deeppink),
	color::from_rgb(color::tomato), color::from_rgb(color::pink), color::from_rgb(color::violet),
	color::from_rgb(color::salmon), color::from_rgb(color::gold)
};

} //namespace

bool game::pause()
{
	ws2812_matrix::clear();
	buttons::flush_pressed();
	bool ret = interruptible_demo();
	ws2812_matrix::clear();
	buttons::flush_pressed();
	return ret;
}

bool game::pause_with_screen_backup()
{
	uint8_t pixels[ws2812_matrix::height][ws2812_matrix::width][ws2812_matrix::bytes_per_led];
	memcpy(pixels, ws2812_matrix::get_pixels(), sizeof(pixels));
	bool ret = pause();
	if(ret)
		memcpy(ws2812_matrix::get_pixels(), pixels, sizeof(pixels));
	return ret;
}

void game::get_random_color(color::rgb& value, uint8_t max_brightness)
{
	uint8_t color_index = rand() % (sizeof(game_colors) / sizeof(game_colors[0]));
	memcpy_P(&value, &game_colors[color_index], sizeof(value));
	color::scale_to_brightness(value, max_brightness);
}
