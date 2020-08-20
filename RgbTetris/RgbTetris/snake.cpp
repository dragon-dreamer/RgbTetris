// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "snake.h"

#include <stdlib.h>
#include <string.h>

#include <avr/pgmspace.h>

#include "accelerometer.h"
#include "buttons.h"
#include "colors.h"
#include "game.h"
#include "number_display.h"
#include "options.h"
#include "queue.h"
#include "timer.h"
#include "util.h"
#include "ws2812_matrix.h"

namespace
{
enum class direction : uint8_t
{
	fwd = buttons::button_fwd, //0
	back = buttons::button_back, //1
	left = buttons::button_left, //2
	right = buttons::button_right, //3
	max
};

static_assert(static_cast<uint8_t>(direction::fwd) == 0
	&& static_cast<uint8_t>(direction::back) == 1
	&& static_cast<uint8_t>(direction::left) == 2
	&& static_cast<uint8_t>(direction::right) == 3,
	"Incorrect direction enumeration");

constexpr uint8_t accelerometer_multiplier_coeff = 2;
constexpr uint8_t max_steps_to_food_bonus = 10;

const game::difficulty_map difficulties[] PROGMEM = {
	{ 0, 1, 105 },
	{ 5, 2, 99 },
	{ 20, 5, 90 },
	{ 80, 12, 83 },
	{ 200, 25, 74 },
	{ 500, 45, 69 },
	{ 900, 99, 63 },
	{ 3000, 130, 56 },
	{ 7000, 171, 47 },
	{ 10000, 211, 38 },
	{ 14000, 235, 33 },
	{ 17500, 255, 27 },
	{ 50000, 255, 20 }
};

using snake_coord = ws2812_matrix::smallest_coord;

using snake_queue = queue<util::round_to_power_of_2(ws2812_matrix::width * ws2812_matrix::height),
	snake_coord>;

constexpr uint16_t max_snake_length = ws2812_matrix::width * ws2812_matrix::height;

constexpr uint8_t food_color_change_count = 15;
constexpr uint8_t snake_color_change_count = 5;
constexpr uint8_t max_snake_wave_steps = 16;

bool move_head(snake_queue& snake_data, uint8_t x, uint8_t y, bool pop_tail)
{
	if(pop_tail)
	{
		snake_coord tail;
		if(snake_data.pop_front(tail))
			ws2812_matrix::clear_pixel(tail.x, tail.y);
	}
	
	return snake_data.push_back({ x, y });
}

bool create_food(const snake_queue& snake_data,
	color::rgb& food_color, uint8_t max_brightness, util::coord& food)
{
	if(snake_data.count() >= max_snake_length)
		return false;
	
	food.x = rand() % ws2812_matrix::width;
	food.y = rand() % ws2812_matrix::height;
	
	while(ws2812_matrix::is_on(food))
	{
		++food.x;
		if(food.x == ws2812_matrix::width)
		{
			food.x = 0;
			++food.y;
			if(food.y == ws2812_matrix::height)
			{
				food.y = 0;
			}
		}
	}
	
	game::get_random_color(food_color, max_brightness);
	return true;
}

void set_difficulty(uint32_t score, bool accelerometer_enabled,
	uint8_t& difficulty, uint8_t& multiplier)
{
	game::difficulty_map elem;
	for(uint8_t i = sizeof(difficulties) / sizeof(difficulties[0]); i; --i)
	{
		memcpy_P(&elem, &difficulties[i - 1], sizeof(elem));
		if(score >= elem.score)
		{
			difficulty = elem.difficulty;
			multiplier = elem.multiplier;
			if(accelerometer_enabled)
				multiplier += accelerometer_multiplier_coeff;
			
			break;
		}
	}
}

void snake_color_wave(const snake_queue& snake_data,
	color::rgb& prev_color, color::rgb& current_color, uint16_t& snake_wave_step_count)
{
	snake_coord coord {0, 0}, prev_coord {0, 0};
	color::rgb rgb;
	uint8_t count = static_cast<uint8_t>(snake_data.count());
	uint8_t step_count = util::max(max_snake_wave_steps, count);
	uint16_t doubled_step_count = static_cast<uint16_t>(step_count) * 2;
	
	snake_data.get(coord, count - 1);
	color::gradient(prev_color, current_color,
		step_count, static_cast<uint16_t>(snake_wave_step_count) % doubled_step_count, rgb);
	ws2812_matrix::set_pixel_color(coord.x, coord.y, rgb);
	
	snake_data.get(coord, 0);
	for(uint8_t i = 0; i != count - 1; ++i)
	{
		snake_data.get(prev_coord, i + 1);
		
		ws2812_matrix::get_pixel_color(prev_coord.x, prev_coord.y, rgb.r, rgb.g, rgb.b);
		ws2812_matrix::set_pixel_color(coord.x, coord.y, rgb);
		
		coord = prev_coord;
	}
	
	if(++snake_wave_step_count >= doubled_step_count)
		snake_wave_step_count = 0;
}

void restore_snake(const snake_queue& snake_data,
	color::rgb& prev_color, color::rgb& current_color, uint16_t& snake_wave_step_count)
{
	for(uint8_t i = 0, count = snake_data.count(); i != count; ++i)
		snake_color_wave(snake_data, prev_color, current_color, snake_wave_step_count);
}

void init(snake_queue& snake_data, direction& snake_direction)
{
	snake_data.clear();
	
	//Generate random initial direction
	snake_direction = static_cast<direction>(rand() % static_cast<uint8_t>(direction::max));
	
	//Set up random initial position
	uint8_t start_x, start_y;
	uint8_t* coord_to_grow = nullptr;
	switch(snake_direction)
	{
		case direction::left:
		case direction::right:
			coord_to_grow = &start_x;
			start_x = 1 + (rand() % (ws2812_matrix::width - 1 - snake::init_len));
			start_y = rand() % ws2812_matrix::height;
			break;
		
		case direction::fwd:
		case direction::back:
			coord_to_grow = &start_y;
			start_y = 1 + (rand() % (ws2812_matrix::height - 1 - snake::init_len));
			start_x = rand() % ws2812_matrix::width;
			break;
		
		default:
			break;
	}
	
	int8_t add = 1;
	if(snake_direction == direction::right || snake_direction == direction::back)
	{
		*coord_to_grow += snake::init_len;
		add = -1;
	}
	
	//Create snake
	for(uint8_t i = 0; i != snake::init_len; ++i, *coord_to_grow += add)
		move_head(snake_data, start_x, start_y, false);
}

bool get_head(const snake_queue& snake_data, uint8_t& x, uint8_t& y)
{
	snake_coord head;
	if(snake_data.get(head, snake_data.count() - 1))
	{
		x = head.x;
		y = head.y;
		return true;
	}
	
	return false;
}

void food_blink(const color::rgb& food_color, const util::coord& food)
{
	//Don't scale to brightness here (already scaled)
	if(ws2812_matrix::is_on(food))
		ws2812_matrix::clear_pixel(food);
	else
		ws2812_matrix::set_pixel_color(food, food_color);
}

uint32_t loop(snake_queue& snake_data, direction snake_direction)
{
	uint8_t action_counter = 0;
	direction new_direction = snake_direction;
	uint8_t food_color_change_counter = food_color_change_count - 1;
	uint8_t snake_color_change_counter = snake_color_change_count - 1;
	const bool accelerometer_enabled = options::is_accelerometer_enabled();
	uint32_t score = 0;
	uint8_t difficulty, multiplier;
	set_difficulty(score, accelerometer_enabled, difficulty, multiplier);
	util::coord food { 0, 0 };
	color::rgb snake_prev_color, snake_current_color;
	const uint8_t max_brightness = options::get_max_brightness();
	game::get_random_color(snake_current_color, max_brightness);
	game::get_random_color(snake_prev_color, max_brightness);
	
	uint16_t snake_wave_step_count = 0;
	restore_snake(snake_data, snake_prev_color,
		snake_current_color, snake_wave_step_count);
	
	color::rgb food_color;
	create_food(snake_data, food_color, max_brightness, food);
	
	uint8_t steps_to_food = max_steps_to_food_bonus;
	while(true)
	{
		timer::wait_for_interrupt();
		
		if(accelerometer_enabled)
		{
			switch(accelerometer::get_exclusive_direction())
			{
				case accelerometer::direction_left:
					new_direction = direction::left;
					break;
				
				case accelerometer::direction_right:
					new_direction = direction::right;
					break;
				
				case accelerometer::direction_up:
					new_direction = direction::fwd;
					break;
				
				case accelerometer::direction_down:
					new_direction = direction::back;
					break;
				
				default:
					break;
			}
		}
		else
		{
			for(uint8_t i = 0; i != buttons::button_directions_max; ++i)
			{
				if(buttons::is_pressed(static_cast<buttons::button_id>(i)))
				{
					new_direction = static_cast<direction>(i);
					break;
				}
			}
		}
		
		if(buttons::is_still_pressed(buttons::button_up)
			&& buttons::is_still_pressed(buttons::button_down))
		{
			if(!game::pause())
			{
				number_display::output_number(score);
				break;
			}
			
			//Force snake re-draw
			number_display::output_number(score);
			restore_snake(snake_data, snake_prev_color,
				snake_current_color, snake_wave_step_count);
			snake_color_change_counter = snake_color_change_count - 1;
		}
		
		if(++action_counter == difficulty)
		{
			action_counter = 0;
			
			if(new_direction != snake_direction)
			{
				//Opposite direction is not allowed
				if(!((new_direction == direction::left && snake_direction == direction::right)
					|| (new_direction == direction::right && snake_direction == direction::left)
					|| (new_direction == direction::fwd && snake_direction == direction::back)
					|| (new_direction == direction::back && snake_direction == direction::fwd)))
				{
					snake_direction = new_direction;
				}
				else
				{
					new_direction = snake_direction;
				}
			}
			
			uint8_t new_x = 0, new_y = 0;
			get_head(snake_data, new_x, new_y);
			
			bool coords_changed = false;
			switch(snake_direction)
			{
				case direction::left:
					if(new_x < ws2812_matrix::width - 1)
					{
						coords_changed = true;
						++new_x;
					}
					break;
				
				case direction::right:
					if(new_x)
					{
						coords_changed = true;
						--new_x;
					}
					break;
					
				case direction::fwd:
					if(new_y < ws2812_matrix::height - 1)
					{
						coords_changed = true;
						++new_y;
					}
					break;
					
				case direction::back:
					if(new_y)
					{
						coords_changed = true;
						--new_y;
					}
					break;
					
				default:
					break;
			}
			
			if(!coords_changed) //Wall collision
				break;
			
			bool has_food = new_x == food.x && new_y == food.y;
			if(ws2812_matrix::is_on_fast(new_x, new_y) && !has_food) //Snake collision
				break;
			
			move_head(snake_data, new_x, new_y, !has_food);
			
			if(has_food)
			{
				if(score < game::max_score)
				{
					score += multiplier + steps_to_food;
					if(score > game::max_score)
						score = game::max_score;
					
					number_display::output_number(score);
					set_difficulty(score, accelerometer_enabled, difficulty, multiplier);
				}
				
				if(snake_current_color != food_color)
				{
					snake_prev_color = snake_current_color;
					snake_current_color = food_color;
					snake_wave_step_count = snake_data.count();
				}
				
				//Redraw snake before creating food
				snake_color_wave(snake_data, snake_prev_color,
					snake_current_color, snake_wave_step_count);
				
				if(!create_food(snake_data, food_color, max_brightness, food)) //No more place
					break;
				
				steps_to_food = max_steps_to_food_bonus;
			}
			else
			{
				//Force snake re-draw
				snake_color_change_counter = snake_color_change_count - 1;
				if(steps_to_food)
					--steps_to_food;
			}
		}
		
		//First draw snake, then food, as food takes light pixels positions from display
		if(++snake_color_change_counter == snake_color_change_count)
		{
			snake_color_change_counter = 0;
			snake_color_wave(snake_data, snake_prev_color,
				snake_current_color, snake_wave_step_count);
			ws2812_matrix::show();
		}
		else if(++food_color_change_counter == food_color_change_count)
		{
			food_color_change_counter = 0;
			food_blink(food_color, food);
			ws2812_matrix::show();
		}
	}
	
	return score;
}
} //namespace

void snake::run()
{
	game::intro();
	
	uint32_t score;
	
	{
		snake_queue snake_data;
		direction snake_direction;
		init(snake_data, snake_direction);
		score = loop(snake_data, snake_direction);
	}
	
	game::end(score, game::game_snake);
}
