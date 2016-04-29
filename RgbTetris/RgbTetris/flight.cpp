// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "flight.h"

#include <stdlib.h>
#include <string.h>

#include <avr/pgmspace.h>

#include "accelerometer.h"
#include "buttons.h"
#include "game.h"
#include "colors.h"
#include "move_helper.h"
#include "number_display.h"
#include "options.h"
#include "timer.h"
#include "ws2812_matrix.h"
#include "util.h"

namespace
{
constexpr uint8_t ship_width = 3;
constexpr uint8_t ship_height = 6; //2 + keep 4 free rows
constexpr uint8_t max_probability = 128;
constexpr uint8_t asteroid_spawn_probability = 12;
constexpr uint8_t gradient_step_count = 100;

struct level
{
	uint32_t score;
	uint8_t min_wall_width : 4;
	uint8_t max_wall_width : 4;
	uint8_t difficulty;
	uint8_t score_multiplier;
	uint8_t wall_move_probability;
};

const level levels[] PROGMEM = {
	{ 0,     2, 3, 56, 1, 8 },
	{ 50,    2, 4, 48, 2, 8 },
	{ 200,   2, 5, 40, 3, 7 },
	{ 500,   3, 5, 32, 4, 7 },
	{ 1000,  3, 6, 28, 5, 6 },
	{ 2000,  4, 6, 24, 7, 6 },
	{ 5000,  4, 7, 20, 10, 5 },
	{ 10000, 5, 7, 16, 15, 5 },
	{ 15000, 6, 7, 10, 25, 4 },
	{ 25000, 7, 7, 6, 35, 4 }
};

void load_level(level& lvl, uint32_t score, uint8_t& target_difficulty_counter)
{
	for(uint8_t i = sizeof(levels) / sizeof(levels[0]); i; --i)
	{
		memcpy_P(&lvl, &levels[i - 1], sizeof(lvl));
		if(score >= lvl.score)
		{
			target_difficulty_counter = lvl.difficulty;
			break;
		}
	}
}

//Ship picture:
/* 010 *
*  111 */
bool ship_intersects(uint8_t x, uint8_t y)
{
	return ws2812_matrix::is_on(x, y)
		|| ws2812_matrix::is_on(x + 1, y)
		|| ws2812_matrix::is_on(x + 2, y)
		|| ws2812_matrix::is_on(x + 1, y + 1);
}

void draw_ship(uint8_t x, uint8_t y,
	const color::rgb& front_color, const color::rgb& back_color)
{
	ws2812_matrix::set_pixel_color(x, y, back_color);
	ws2812_matrix::set_pixel_color(x + 1, y, back_color);
	ws2812_matrix::set_pixel_color(x + 2, y, back_color);
	ws2812_matrix::set_pixel_color(x + 1, y + 1, front_color);
}

void clear_ship(uint8_t x, uint8_t y)
{
	color::rgb black { 0, 0, 0 };
	draw_ship(x, y, black, black);
}

void spawn_asteroid(uint8_t left_wall_size, uint8_t right_wall_size, const color::rgb& rgb)
{
	if(rand() % max_probability > asteroid_spawn_probability)
		return;
	
	uint8_t available_x_coords[ws2812_matrix::width - 2];
	uint8_t coord_count = 0;
	for(uint8_t x = left_wall_size + 1; x != ws2812_matrix::width - right_wall_size - 1; ++x)
	{
		bool ok = true;
		for(uint8_t y = ws2812_matrix::height - 1; y != ws2812_matrix::height - 5; --y)
		{
			if(ws2812_matrix::is_on(x, y) || ws2812_matrix::is_on(x - 1, y) || ws2812_matrix::is_on(x + 1, y))
			{
				ok = false;
				break;
			}
		}
		
		if(ok)
		{
			available_x_coords[coord_count] = x;
			++coord_count;
		}
	}
	
	if(coord_count)
	{
		ws2812_matrix::set_pixel_color(available_x_coords[rand() % coord_count],
			ws2812_matrix::height - 1, rgb);
	}
}

constexpr uint8_t max_bullets = 4;
constexpr uint8_t target_bullet_move_counter = 10;
constexpr uint8_t target_bullet_counter = 30;
struct bullet
{
	util::coord coords;
	bool active;
};

struct bullet_info
{
	bullet_info()
		: bullet_counter(0),
		bullet_count(0)
	{
		memset(bullets, 0, sizeof(bullets));
	}
	
	uint8_t bullet_counter;
	uint8_t bullet_count;
	bullet bullets[max_bullets];
};

bool add_bullet(uint8_t x, uint8_t y, bullet_info& info)
{
	if(info.bullet_counter || info.bullet_count == max_bullets)
		return false;
	
	//Check if wall is ahead
	if(ws2812_matrix::is_on(x, y) && (ws2812_matrix::is_on(x + 1, y) || ws2812_matrix::is_on(x - 1, y)))
		return false;
	
	for(uint8_t i = 0; i != max_bullets; ++i)
	{
		auto& bullet = info.bullets[i];
		if(bullet.active)
			continue;
			
		bullet.coords.x = x;
		bullet.coords.y = y;
		bullet.active = true;
		break;
	}
	
	++info.bullet_count;
	info.bullet_counter = target_bullet_counter;
	return true;
}

void draw_bullets(const bullet_info& info, const color::rgb& rgb)
{
	for(uint8_t i = 0; i != max_bullets; ++i)
	{
		auto& bullet = info.bullets[i];
		if(!bullet.active)
			continue;
		
		ws2812_matrix::set_pixel_color(bullet.coords, rgb);
	}
}

bool calculate_bullets(bullet_info& info, uint32_t& score, uint8_t multiplier)
{
	bool score_changed = false;
	for(uint8_t i = 0; i != max_bullets; ++i)
	{
		auto& bullet = info.bullets[i];
		if(!bullet.active)
			continue;
		
		//Check if asteroid or wall is ahead
		if(ws2812_matrix::is_on(bullet.coords))
		{
			bullet.active = false;
			--info.bullet_count;
			if(!ws2812_matrix::is_on(bullet.coords.x + 1, bullet.coords.y)
				&& !ws2812_matrix::is_on(bullet.coords.x - 1, bullet.coords.y))
			{
				//Asteroid hit
				ws2812_matrix::clear_pixel(bullet.coords);
				score += multiplier;
				score_changed = true;
			}
		}
	}
	
	return score_changed;
}

void move_bullets(bullet_info& info)
{
	for(uint8_t i = 0; i != max_bullets; ++i)
	{
		auto& bullet = info.bullets[i];
		if(!bullet.active)
			continue;
		
		if(++bullet.coords.y == ws2812_matrix::height)
		{
			bullet.active = false;
			--info.bullet_count;
		}
	}
}

uint32_t loop()
{
	level current_level { 0, 0, 0, 0, 0, 0 };
	uint32_t temp_score = 0;
	uint32_t result_score = 0;
	uint32_t prev_result_score;
	uint8_t target_difficulty_counter = 0;
	load_level(current_level, result_score, target_difficulty_counter);
	
	uint8_t left_wall_size = 1;
	uint8_t right_wall_size = 1;
	int8_t next_left_wall_size_change = 0, next_right_wall_size_change = 0;
	uint8_t difficulty_counter = 0;
	bool need_redraw = true;
	
	color::rgb current_color, prev_color, rgb;
	uint8_t max_brightness = options::get_max_brightness();
	bool accelerometer_enabled = options::is_accelerometer_enabled();
	game::get_random_color(current_color, max_brightness);
	game::get_random_color(prev_color, max_brightness);
	uint8_t current_gradient_step = 0;
	accelerometer::speed_state x_speed_state(7), y_speed_state(7);
	bullet_info bullets;
	bool need_check_bullets = false;
	uint8_t bullet_move_counter = 0;
	
	for(uint8_t i = 0; i != ws2812_matrix::height; ++i)
	{
		ws2812_matrix::set_pixel_color(0, i, prev_color);
		ws2812_matrix::set_pixel_color(ws2812_matrix::width - 1, i, prev_color);
	}
	
	uint8_t ship_x = (ws2812_matrix::width - ship_width) / 2;
	uint8_t ship_y = 0;
	uint8_t new_x = ship_x, new_y = ship_y;
	uint8_t move_dir;
	
	constexpr color::rgb ship_front_color { 0xff, 0, 0 };
	constexpr color::rgb ship_back_color { 0xff, 0xff, 0 };
	constexpr color::rgb bullet_color { 0, 0, 0xff };
	constexpr color::rgb asteroid_color { 0xff, 0, 0 };
		
	color::rgb front_color(ship_front_color);
	color::rgb back_color(ship_back_color);
	color::scale_to_brightness(front_color, max_brightness);
	color::scale_to_brightness(back_color, max_brightness);
	draw_ship(ship_x, ship_y, front_color, back_color);
	
	color::rgb current_bullet_color(bullet_color);
	color::rgb current_asteroid_color(asteroid_color);
	color::scale_to_brightness(current_bullet_color, max_brightness);
	color::scale_to_brightness(current_asteroid_color, max_brightness);
	
	while(true)
	{
		timer::wait_for_interrupt();
		
		draw_bullets(bullets, { 0, 0, 0 });
		if(++difficulty_counter == target_difficulty_counter)
		{
			difficulty_counter = 0;
			need_check_bullets = true;
			
			clear_ship(ship_x, ship_y);
			ws2812_matrix::shift_down();
			if(ship_intersects(ship_x, ship_y))
				return result_score; //Game over
				
			color::gradient(prev_color, current_color,
				gradient_step_count, current_gradient_step, rgb);
			
			if(++current_gradient_step == gradient_step_count)
			{
				current_gradient_step = 0;
				prev_color = current_color;
				game::get_random_color(current_color, max_brightness);
			}
			
			for(uint8_t x = 0; x != ws2812_matrix::width; ++x)
			{
				if(x < left_wall_size || x >= ws2812_matrix::width - right_wall_size)
					ws2812_matrix::set_pixel_color(x, ws2812_matrix::height - 1, rgb);
			}
			
			spawn_asteroid(left_wall_size, right_wall_size, current_asteroid_color);
			draw_ship(ship_x, ship_y, front_color, back_color);
			
			temp_score += current_level.score_multiplier;
			prev_result_score = result_score;
			result_score = temp_score / 8;
			if(prev_result_score != result_score)
			{
				number_display::output_number(result_score);
				load_level(current_level, result_score, target_difficulty_counter);
			}
			
			if(next_left_wall_size_change)
			{
				left_wall_size += next_left_wall_size_change;
				next_left_wall_size_change = 0;
			}
			else if(next_right_wall_size_change)
			{
				right_wall_size += next_right_wall_size_change;
				next_right_wall_size_change = 0;
			}
			else
			{
				switch(rand() % current_level.wall_move_probability)
				{
					case 0:
						if(left_wall_size + right_wall_size >= current_level.max_wall_width)
						{
							if(right_wall_size == 1)
								break;
						
							--right_wall_size;
							next_left_wall_size_change = 1;
						}
						else
						{
							++left_wall_size;
						}
						break;
					
					case 1:
						if(left_wall_size > 1)
						{
							--left_wall_size;
						
							if(left_wall_size + right_wall_size < current_level.min_wall_width)
								next_right_wall_size_change = 1;
						}
						break;
					
					case 2:
						if(left_wall_size + right_wall_size >= current_level.max_wall_width)
						{
							if(left_wall_size == 1)
								break;
						
							--left_wall_size;
							next_right_wall_size_change = 1;
						}
						else
						{
							++right_wall_size;
						}
						break;
					
					case 3:
						if(right_wall_size > 1)
						{
							--right_wall_size;
						
							if(left_wall_size + right_wall_size < current_level.min_wall_width)
								next_left_wall_size_change = 1;
						}
						break;
				
					default:
						break;
				}
			}
		}
		
		clear_ship(ship_x, ship_y);
		
		move_dir = move_helper::process_speed(&x_speed_state, &y_speed_state, accelerometer_enabled);
		if(move_dir & move_direction_left)
		{
			if(ship_x < ws2812_matrix::width - ship_width && !ship_intersects(new_x + 1, new_y))
				++new_x;
		}	
		else if(move_dir & move_direction_right)
		{
			if(ship_x && !ship_intersects(new_x - 1, new_y))
				--new_x;
		}
			
		if(move_dir & move_direction_up)
		{
			if(ship_y < ws2812_matrix::height - ship_height && !ship_intersects(new_x, new_y + 1))
				++new_y;
		}
		else if(move_dir & move_direction_down)
		{
			if(ship_y && !ship_intersects(new_x, new_y - 1))
				--new_y;
		}
		
		if(new_x != ship_x || new_y != ship_y)
		{
			need_redraw = true;
			ship_x = new_x;
			ship_y = new_y;
		}
		
		draw_ship(ship_x, ship_y, front_color, back_color);
		
		if(bullets.bullet_counter)
			--bullets.bullet_counter;
		
		auto button_up_state = buttons::get_button_status(buttons::button_up);
		auto button_down_state = buttons::get_button_status(buttons::button_down);
		if(button_up_state == buttons::button_status_pressed) //Shoot
		{
			if(add_bullet(ship_x + 1, ship_y + 2, bullets))
				need_check_bullets = true;
		}
		else if(button_up_state == buttons::button_status_still_pressed
			&& button_down_state == buttons::button_status_still_pressed)
		{
			if(!game::pause_with_screen_backup())
			{
				number_display::output_number(result_score);
				return result_score;
			}
					
			number_display::output_number(result_score);
			need_redraw = true;
		}
		
		if(!need_check_bullets && ++bullet_move_counter == target_bullet_move_counter)
		{
			bullet_move_counter = 0;
			move_bullets(bullets);
			need_check_bullets = true;
		}
		
		if(need_check_bullets)
		{
			need_check_bullets = false;
			need_redraw = true;
			if(calculate_bullets(bullets, result_score, current_level.score_multiplier))
			{
				number_display::output_number(result_score);
				temp_score = result_score * 8;
			}
		}
		
		if(need_redraw)
		{
			draw_bullets(bullets, current_bullet_color);
			
			need_redraw = false;
			ws2812_matrix::show();
		}
	}
	
	return result_score;
}
} //namespace

void flight::run()
{
	game::intro();
	buttons::enable_repeat(buttons::mask_fwd | buttons::mask_back | buttons::mask_right
		| buttons::mask_left | buttons::mask_up, true);
	
	uint32_t score = loop();
	
	buttons::enable_repeat(buttons::mask_fwd | buttons::mask_back | buttons::mask_right
		| buttons::mask_left | buttons::mask_up, false);
	game::end(score, game::game_asteroids);
}
