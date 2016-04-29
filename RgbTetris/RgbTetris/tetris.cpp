// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "tetris.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <avr/pgmspace.h>

#include "accelerometer.h"
#include "buttons.h"
#include "colors.h"
#include "game.h"
#include "move_helper.h"
#include "number_display.h"
#include "options.h"
#include "timer.h"
#include "util.h"
#include "ws2812_matrix.h"

namespace
{
struct tetris_difficulty_map
{
	uint32_t score;
	uint8_t multiplier;
	uint8_t difficulty;
	uint8_t max_block_id;
};

const tetris_difficulty_map difficulties[] PROGMEM = {
	{ 0, 1, 118, 6 },
	{ 5, 2, 112, 6 },
	{ 20, 3, 105, 6 },
	{ 80, 4, 98, 7 },
	{ 200, 5, 89, 7 },
	{ 500, 7, 80, 8 },
	{ 900, 10, 72, 8 },
	{ 3000, 12, 63, 9 },
	{ 5000, 15, 54, 9 },
	{ 10000, 17, 45, 10 },
	{ 15000, 20, 36, 11 },
	{ 25000, 25, 27, 13 },
	{ 50000, 30, 16, 13 }
};

constexpr uint8_t invalid_block_pos = 0xff;
constexpr uint8_t combo_multiplier = 3;
constexpr uint8_t fast_fall_game_difficulty = 7;

struct block
{
	static constexpr uint8_t max_columns = 4;
	static constexpr uint8_t max_rows = 4;
	
	uint8_t width : 4;
	uint8_t height : 4;
	
	union
	{
		struct
		{
			uint8_t row1 : 4;
			uint8_t row2 : 4;
			uint8_t row3 : 4;
			uint8_t row4 : 4;
		};
		
		uint16_t data;
	};
	
	bool at(uint8_t x, uint8_t y) const
	{
		return static_cast<bool>(data & (1 << (y * max_columns + x)));
	}
	
	void set(uint8_t x, uint8_t y, bool value)
	{
		data ^= (-static_cast<int8_t>(value) ^ data) & (1 << (y * max_columns + x));
	}
};

const block blocks[] PROGMEM = {
	{ 4, 1, 0b1111, 0, 0, 0 }, //line 4x1
		
	{ 3, 2, 0b0011, 0b0110, 0, 0 }, //"Z" left
	{ 3, 2, 0b0110, 0b0011, 0, 0 }, //"Z" right
	{ 3, 2, 0b0010, 0b0111, 0, 0 }, //"T"
	{ 3, 2, 0b0001, 0b0111, 0, 0 }, //"L" left
	{ 3, 2, 0b0100, 0b0111, 0, 0 }, //"L" right
	{ 2, 2, 0b0011, 0b0011, 0, 0 }, //cube
	
	//7
	{ 1, 1, 0b0001, 0, 0, 0 }, //"."
	
	//8
	{ 3, 2, 0b0111, 0b0111, 0, 0 }, //block 3x2
	
	//9
	{ 3, 3, 0b0111, 0b0010, 0b0010, 0 }, //Huge "T"
	
	//10
	{ 3, 3, 0b0111, 0b0111, 0b0111, 0 }, //Huge cube
	
	//11
	{ 3, 1, 0b0111, 0, 0, 0 }, //line 3x1
	
	//12
	{ 3, 3, 0b0011, 0b0010, 0b0110, 0 }, //Huge "Z" left
	{ 3, 3, 0b0110, 0b0010, 0b0011, 0 } //Huge "Z" right
};

tetris_difficulty_map get_difficulty(uint32_t score)
{
	tetris_difficulty_map elem { 0, 0, 0, 0 };
	for(uint8_t i = sizeof(difficulties) / sizeof(difficulties[0]); i; --i)
	{
		memcpy_P(&elem, &difficulties[i - 1], sizeof(elem));
		if(score >= elem.score)
			return elem;
	}
	
	return elem;
}

block rotate_block(const block& b)
{
	block ret { b.height, b.width, 0, 0, 0, 0 };
	
	for(uint8_t i = 0; i != b.width; ++i)
	{
		for(uint8_t j = 0; j != b.height; ++j)
		{
			//ret.set(j, i, b.at(b.width - i - 1, j)); //ccw
			ret.set(b.height - 1 - j, i, b.at(i, j)); //cw
		}
	}
	
	return ret;
}

block create_block(uint8_t max_block_id)
{
	uint8_t block_id = rand() % (max_block_id + 1);
	block ret;
	memcpy_P(&ret, &blocks[block_id], sizeof(ret));
	for(uint8_t i = 0, rotations = rand() % 4; i != rotations; ++i)
		ret = rotate_block(ret);
	
	return ret;
}

bool intersects(const block& b, uint8_t x, uint8_t y)
{
	for(uint8_t i = 0; i != b.height; ++i)
	{
		for(uint8_t j = 0; j != b.width; ++j)
		{
			if(b.at(j, i) && ws2812_matrix::is_on(x + j, y - i))
				return true;
		}
	}
	
	return false;
}

void hide_block(uint8_t x, uint8_t y, const block& fig)
{
	for(uint8_t i = 0; i != fig.height; ++i)
	{
		for(uint8_t j = 0; j != fig.width; ++j)
		{
			if(fig.at(j, i))
				ws2812_matrix::clear_pixel(x + j, y - i);
		}
	}
}

void show_block(uint8_t x, uint8_t y, const block& fig, const color::rgb& rgb)
{
	for(uint8_t i = 0; i != fig.height; ++i)
	{
		for(uint8_t j = 0; j != fig.width; ++j)
		{
			if(fig.at(j, i))
				ws2812_matrix::set_pixel_color(x + j, y - i, rgb);
		}
	}
}

bool get_new_block_pos(const block& b, uint8_t& x)
{
	x = (ws2812_matrix::width - b.width) / 2;
	if(!intersects(b, x, ws2812_matrix::height - 1))
		return true;
	
	//Try to fit block somewhere else
	for(uint8_t new_x = 0; new_x != ws2812_matrix::width - b.width + 1; ++new_x)
	{
		if(!intersects(b, new_x, ws2812_matrix::height - 1))
		{
			x = new_x;
			return true;
		}
	}
	
	return false;
}

void init_difficulty(uint32_t score, uint8_t& multiplier,
	uint8_t& game_difficulty, uint8_t& max_block_id)
{
	tetris_difficulty_map difficulty = get_difficulty(score);
	game_difficulty = difficulty.difficulty;
	multiplier = difficulty.multiplier;
	max_block_id = difficulty.max_block_id;
}

void create_bright_color(color::rgb& rgb)
{
	rgb.r = (rand() % 2) * 0xff;
	rgb.g = (rand() % 2) * 0xff;
	rgb.b = (rand() % 2) * 0xff;
	if(!rgb.r && !rgb.g && !rgb.b)
		rgb.r = 0xff;
}

void remove_rows_flash(uint8_t full_rows[ws2812_matrix::height], uint8_t row_count,
	uint8_t max_brightness)
{
	color::rgb rgb;
	create_bright_color(rgb);
	color::scale_to_brightness(rgb, max_brightness);
	
	color::rgb current;
	for(uint8_t step = 0; step != 6; ++step)
	{
		if(step % 2)
			current = { 0, 0, 0 };
		else
			current = rgb;
		
		for(uint8_t x = 0; x != ws2812_matrix::width; ++x)
		{
			for(int8_t i = row_count - 1; i >= 0; --i)
				ws2812_matrix::set_pixel_color(x, full_rows[i], current);
		}
		
		ws2812_matrix::show();
		util::delay(500);
	}
}

void remove_rows_color_light(uint8_t full_rows[ws2812_matrix::height], uint8_t row_count,
	uint8_t max_brightness)
{
	color::rgb rgb;
	create_bright_color(rgb);
	
	color::rgb scaled(rgb);
	color::scale_to_brightness(scaled, max_brightness);
	for(uint8_t x = 0; x != ws2812_matrix::width; ++x)
	{
		for(int8_t i = row_count - 1; i >= 0; --i)
			ws2812_matrix::set_pixel_color(x, full_rows[i], scaled);
		
		ws2812_matrix::show();
		util::delay(150);
	}
	
	for(uint8_t c = 0x33; c; --c)
	{
		if(rgb.r)
			rgb.r -= 5;
		if(rgb.g)
			rgb.g -= 5;
		if(rgb.b)
			rgb.b -= 5;
		
		scaled = rgb;
		color::scale_to_brightness(scaled, max_brightness);
		
		for(int8_t i = row_count - 1; i >= 0; --i)
		{
			for(uint8_t x = 0; x != ws2812_matrix::width; ++x)
				ws2812_matrix::set_pixel_color(x, full_rows[i], scaled);
		}
		
		ws2812_matrix::show();
	}
}

void check_filled_rows(uint32_t& score, uint8_t& multiplier,
	uint8_t& game_difficulty, uint8_t& max_block_id, uint8_t max_brightness)
{
	uint8_t full_rows[ws2812_matrix::height];
	uint8_t current_row_id = 0;
	for(uint8_t y = 0; y != ws2812_matrix::height; ++y)
	{
		bool is_full = true;
		for(uint8_t x = 0; x != ws2812_matrix::width; ++x)
		{
			if(!ws2812_matrix::is_on_fast(x, y))
			{
				is_full = false;
				break;
			}
		}
		
		if(is_full)
		{
			full_rows[current_row_id] = y;
			++current_row_id;
		}
	}
	
	if(current_row_id)
	{
		if(rand() % 2)
			remove_rows_flash(full_rows, current_row_id, max_brightness);
		else
			remove_rows_color_light(full_rows, current_row_id, max_brightness);
		
		for(uint8_t i = 0; i != current_row_id; ++i)
		{
			for(uint8_t y = full_rows[i] + 1 - i; y <= ws2812_matrix::height; ++y)
			{
				for(uint8_t x = 0; x != ws2812_matrix::width; ++x)
					ws2812_matrix::copy_color(x, y, x, y - 1);
			}
		}
		
		ws2812_matrix::show();
		
		score += multiplier * current_row_id;
		if(current_row_id > 1) //Combo
			score += (multiplier * current_row_id) / combo_multiplier;
		
		if(score >= game::max_score)
			score = game::max_score;
		
		number_display::output_number(score);
	}
	
	init_difficulty(score, multiplier, game_difficulty, max_block_id);
}

bool loop(uint32_t& score)
{
	score = 0;
	const uint8_t max_brightness = options::get_max_brightness();
	const bool accelerometer_enabled = options::is_accelerometer_enabled();
	
	block fig;
	uint8_t block_x = invalid_block_pos, block_y = invalid_block_pos;
	color::rgb block_color;
	uint8_t new_x;
	bool need_redraw = false;
	
	uint8_t game_counter = 0;
	uint8_t game_difficulty;
	uint8_t multiplier;
	uint8_t max_block_id;
	accelerometer::speed_state speed_state(5);
	accelerometer::direction last_direction = accelerometer::direction_none;
	
	init_difficulty(score, multiplier, game_difficulty, max_block_id);
	
	while(true)
	{
		timer::wait_for_interrupt();
		
		if(block_x == invalid_block_pos)
		{
			fig = create_block(max_block_id);
			game::get_random_color(block_color, max_brightness);
			
			block_y = ws2812_matrix::height - 1;
			bool have_space = get_new_block_pos(fig, block_x);
			show_block(block_x, block_y, fig, block_color);
			ws2812_matrix::show();
			if(!have_space)
				break; //No more space
		}
		
		if(++game_counter == game_difficulty)
		{
			game_counter = 0;
			hide_block(block_x, block_y, fig);
			if(block_y == fig.height - 1 || intersects(fig, block_x, block_y - 1))
			{
				show_block(block_x, block_y, fig, block_color);
				block_y = invalid_block_pos;
				block_x = invalid_block_pos; //Force new block creation
				//Re-initializes difficulty in case of fast drop
				check_filled_rows(score, multiplier, game_difficulty, max_block_id,
					max_brightness);
				continue;
			}
			
			--block_y;
			need_redraw = true;
		}
		
		auto button_up_status = buttons::get_button_status(buttons::button_up);
		auto button_down_status = buttons::get_button_status(buttons::button_down);
		if(button_up_status != buttons::button_status_not_pressed
			&& button_down_status != buttons::button_status_not_pressed)
		{
			button_up_status = buttons::button_status_not_pressed;
			button_down_status = buttons::button_status_not_pressed;
			
			hide_block(block_x, block_y, fig);
			if(!game::pause_with_screen_backup())
			{
				number_display::output_number(score);
				return true;
			}
			
			show_block(block_x, block_y, fig, block_color);
			number_display::output_number(score);
			need_redraw = true;
		}
		
		new_x = block_x;
		
		if(button_up_status == buttons::button_status_pressed)
		{
			hide_block(block_x, block_y, fig);
			const block new_fig = rotate_block(fig);
			uint8_t move_left = 0;
			
			bool rotated = true;
			if(block_y < new_fig.height - 1)
			{
				rotated = false;
			}
			else
			{
				if(new_fig.width + block_x > ws2812_matrix::width)
					move_left = block_x + new_fig.width - ws2812_matrix::width;
				
				while(intersects(new_fig, block_x - move_left, block_y))
				{
					++move_left;
					if(move_left == new_fig.width || block_x < move_left)
					{
						rotated = false;
						break;
					}
				}
			}
			
			if(rotated)
			{
				block_x -= move_left;
				new_x = block_x;
				fig = new_fig;
				need_redraw = true;
			}
			else
			{
				show_block(block_x, block_y, fig, block_color);
			}
		}
		
		switch(move_helper::process_speed(&speed_state, nullptr, accelerometer_enabled))
		{
			case move_direction_left:
				if(new_x < ws2812_matrix::width - fig.width)
					++new_x;
				break;
				
			case move_direction_right:
				if(new_x)
					--new_x;
				break;
				
			default:
				break;
		}
		
		if(accelerometer_enabled)
		{
			if(game_difficulty != fast_fall_game_difficulty)
			{
				auto accel_direction = accelerometer::get_exclusive_direction();
				
				if(accel_direction == accelerometer::direction_down
					&& last_direction == accelerometer::direction_none)
				{
					game_difficulty = fast_fall_game_difficulty;
					game_counter = game_difficulty - 1;
				}
				
				last_direction = accel_direction;
			}
		}
		else
		{
			if(button_down_status == buttons::button_status_pressed
				&& game_difficulty != fast_fall_game_difficulty)
			{
				game_difficulty = fast_fall_game_difficulty;
				game_counter = game_difficulty - 1;
			}
		}
		
		if(new_x != block_x)
		{
			hide_block(block_x, block_y, fig);
			if(!intersects(fig, new_x, block_y))
			{
				block_x = new_x;
				need_redraw = true;
			}
			else
			{
				show_block(block_x, block_y, fig, block_color);
			}
		}
		
		if(need_redraw)
		{
			show_block(block_x, block_y, fig, block_color);
			ws2812_matrix::show();
			need_redraw = false;
		}
	}
	
	return false;
}
} //namespace

void tetris::run()
{
	game::intro();
	buttons::enable_repeat(buttons::mask_right | buttons::mask_left, true);
	
	uint32_t score = 0;
	bool interrupted = loop(score);
	
	buttons::enable_repeat(buttons::mask_right | buttons::mask_left, false);
	
	if(!interrupted)
		util::delay(5000);
	game::end(score, game::game_tetris);
}
