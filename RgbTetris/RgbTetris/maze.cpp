// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "maze.h"

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
enum wall_position : uint8_t
{
	wall_up = 1 << 0,
	wall_down = 1 << 1,
	wall_left = 1 << 2,
	wall_right = 1 << 3,
	wall_count = 4
};

struct cell
{
	void clear_wall(wall_position pos)
	{
		wall_state |= pos;
	}

	bool has_wall(wall_position pos) const
	{
		return !(wall_state & pos);
	}

	uint8_t wall_state : 4;
	uint8_t prev_cell_x : 6;
	uint8_t prev_cell_y : 6;
};

constexpr uint16_t max_maze_size = 1225; //35*35
using maze_data = cell[max_maze_size];
static_assert(sizeof(maze_data) == 2450, "Incorrect maze_data size");
static_assert(sizeof(maze_data) < RAMSIZE - 1500, "Incorrect maze_data size");

struct maze_dimension
{
	uint8_t size;
	uint8_t entrance_cell_coord;
	uint8_t exit_cell_coord;
	
	uint8_t cell_offset;
	uint8_t pixel_offset;
	uint8_t character_offset;
};

struct maze_info
{
	void init(uint8_t new_width, uint8_t new_height)
	{
		dim_x.size = new_width;
		dim_y.size = new_height;
		list_count = 0;
		memset(data, 0, sizeof(data));
	}

	cell* at(uint8_t x, uint8_t y)
	{
		if (x >= dim_x.size || y >= dim_y.size)
			return nullptr;

		return &data[y * dim_x.size + x];
	}

	uint16_t list_count; //For growing tree algorithm
	maze_dimension dim_x, dim_y;
	uint8_t cell_size;
	color::rgb start_color;
	color::rgb end_color;
	color::rgb exit_color;
	maze_data data;
};

void get_start_maze_point(uint8_t width, uint8_t height,
	uint8_t& start_x, uint8_t& start_y)
{
	if (rand() % 2)
	{
		start_x = (rand() % 2) ? 0 : width - 1;
		start_y = rand() % height;
	}
	else
	{
		start_y = (rand() % 2) ? 0 : height - 1;
		start_x = rand() % width;
	}
}

void get_unvisited_cells(maze_info& maze, uint8_t x, uint8_t y,
	uint8_t& unvisited_neighbors_count, util::coord unvisited_neighbors_coords[4],
	cell* init_cell)
{
	unvisited_neighbors_count = 0;
	//[-1; 0], [1; 0], [0; -1], [0; 1]
	for(int8_t i = -1; i < 2; i += 2)
	{
		cell* unvisited_cell = maze.at(static_cast<uint8_t>(x + i), y);
		if(unvisited_cell && !unvisited_cell->prev_cell_x && unvisited_cell != init_cell)
		{
			unvisited_neighbors_coords[unvisited_neighbors_count++]
				= { static_cast<uint8_t>(x + i), y };
		}
	}
	for(int8_t i = -1; i < 2; i += 2)
	{
		cell* unvisited_cell = maze.at(x, static_cast<uint8_t>(y + i));
		if(unvisited_cell && !unvisited_cell->prev_cell_x && unvisited_cell != init_cell)
		{
			unvisited_neighbors_coords[unvisited_neighbors_count++]
				= { x, static_cast<uint8_t>(y + i) };
		}
	}
}

void remove_unneeded_walls(uint8_t x, uint8_t y,
	const util::coord& next_cell_coords, cell* current_cell, cell* next_cell)
{
	if(next_cell_coords.y < y)
	{
		current_cell->clear_wall(wall_up);
		next_cell->clear_wall(wall_down);
	}
	else if(next_cell_coords.y > y)
	{
		current_cell->clear_wall(wall_down);
		next_cell->clear_wall(wall_up);
	}
	else if(next_cell_coords.x < x)
	{
		current_cell->clear_wall(wall_left);
		next_cell->clear_wall(wall_right);
	}
	else //next_cell_coords.x > x
	{
		current_cell->clear_wall(wall_right);
		next_cell->clear_wall(wall_left);
	}
}

void generate_maze_dfs(maze_info& maze)
{
	uint8_t x, y;
	get_start_maze_point(maze.dim_x.size, maze.dim_y.size, x, y);

	util::coord unvisited_neighbors_coords[4];
	uint8_t unvisited_neighbors_count;
	cell* init_cell = maze.at(x, y);
	maze.dim_x.exit_cell_coord = x;
	maze.dim_y.exit_cell_coord = y;
	uint16_t level = 0, max_level = 0;
	while(true)
	{
		cell* current_cell = maze.at(x, y);
		get_unvisited_cells(maze, x, y, unvisited_neighbors_count, unvisited_neighbors_coords, init_cell);
		if(!unvisited_neighbors_count)
		{
			//Backtrack
			if(!current_cell->prev_cell_x)
				break; //Finish generation

			if(level > max_level)
			{
				max_level = level;
				maze.dim_x.entrance_cell_coord = x;
				maze.dim_y.entrance_cell_coord = y;
			}

			x = current_cell->prev_cell_x - 1;
			y = current_cell->prev_cell_y - 1;
			--level;
			continue;
		}
		else
		{
			//Take random unvisited neighbor
			const util::coord& next_cell_coords = unvisited_neighbors_coords[rand() % unvisited_neighbors_count];
			cell* next_cell = maze.at(next_cell_coords.x, next_cell_coords.y);
			next_cell->prev_cell_x = x + 1;
			next_cell->prev_cell_y = y + 1;

			remove_unneeded_walls(x, y, next_cell_coords, current_cell, next_cell);

			x = next_cell_coords.x;
			y = next_cell_coords.y;
			++level;
		}
	}
}

void add_point_to_list(maze_info& maze, cell& value)
{
	++maze.list_count;
	value.prev_cell_x = 1;
}

bool get_point_from_list(maze_info& maze, uint8_t& x, uint8_t& y)
{
	if(!maze.list_count)
		return false;

	uint16_t cell_index = rand() % maze.list_count;
	for(x = 0; x != maze.dim_x.size; ++x)
	{
		for(y = 0; y != maze.dim_y.size; ++y)
		{
			cell* value = maze.at(x, y);
			if(value->prev_cell_x == 1)
			{
				if(!cell_index)
					return true;

				--cell_index;
			}
		}
	}

	return false;
}

void remove_point_from_list(maze_info& maze, cell& value)
{
	--maze.list_count;
	value.prev_cell_x = 63; //max value
}

void generate_maze_growing_tree(maze_info& maze)
{
	uint8_t x, y;
	get_start_maze_point(maze.dim_x.size, maze.dim_y.size, x, y);

	util::coord unvisited_neighbors_coords[4];
	uint8_t unvisited_neighbors_count;

	add_point_to_list(maze, *maze.at(x, y));
	while(true)
	{
		if(!get_point_from_list(maze, x, y))
			break;

		get_unvisited_cells(maze, x, y, unvisited_neighbors_count, unvisited_neighbors_coords, nullptr);

		cell* current_cell = maze.at(x, y);
		if(!unvisited_neighbors_count)
		{
			remove_point_from_list(maze, *current_cell);
			continue;
		}
		else
		{
			//Take random unvisited neighbor
			const util::coord& next_cell_coords = unvisited_neighbors_coords[rand() % unvisited_neighbors_count];
			cell* next_cell = maze.at(next_cell_coords.x, next_cell_coords.y);
			add_point_to_list(maze, *next_cell);
			remove_unneeded_walls(x, y, next_cell_coords, current_cell, next_cell);
		}
	}
}

void draw_maze(maze_info& maze, uint16_t seconds_for_level, uint16_t original_seconds_for_level)
{
	color::rgb maze_color;
	color::gradient(maze.start_color, maze.end_color, original_seconds_for_level / 4,
		(original_seconds_for_level - seconds_for_level) / 4, maze_color);
	
	uint8_t horizontal_cells = 1 + (ws2812_matrix::width - 1) / (maze.cell_size - 1);
	uint8_t vertical_cells = 1 + (ws2812_matrix::height - 1) / (maze.cell_size - 1);
	
	for(uint8_t cell_x = 0; cell_x != horizontal_cells + 1; ++cell_x)
	{
		for(uint8_t cell_y = 0; cell_y != vertical_cells + 1; ++cell_y)
		{
			cell* current_cell = maze.at(cell_x + maze.dim_x.cell_offset, cell_y + maze.dim_y.cell_offset);
			if(!current_cell)
				continue;
			
			int8_t cell_pixel_pos_x = cell_x * (maze.cell_size - 1) - maze.dim_x.pixel_offset;
			int8_t cell_pixel_pos_y = cell_y * (maze.cell_size - 1) - maze.dim_y.pixel_offset;
			for(int8_t x = cell_pixel_pos_x; x != cell_pixel_pos_x + maze.cell_size; ++x)
			{
				if(x == cell_pixel_pos_x || x == cell_pixel_pos_x + maze.cell_size - 1) //corners
				{
					ws2812_matrix::set_pixel_color(x, cell_pixel_pos_y, maze_color);
					ws2812_matrix::set_pixel_color(x, cell_pixel_pos_y + maze.cell_size - 1, maze_color);
				}
				else
				{
					if(current_cell->has_wall(wall_down))
						ws2812_matrix::set_pixel_color(x, cell_pixel_pos_y + maze.cell_size - 1, maze_color);
					if(current_cell->has_wall(wall_up))
						ws2812_matrix::set_pixel_color(x, cell_pixel_pos_y, maze_color);
				}
			}
			
			for(int8_t y = cell_pixel_pos_y; y != cell_pixel_pos_y + maze.cell_size; ++y)
			{
				if(y == cell_pixel_pos_y || y == cell_pixel_pos_y + maze.cell_size - 1) //corners
				{
					ws2812_matrix::set_pixel_color(cell_pixel_pos_x, y, maze_color);
					ws2812_matrix::set_pixel_color(cell_pixel_pos_x + maze.cell_size - 1, y, maze_color);
				}
				else
				{
					if(current_cell->has_wall(wall_right))
						ws2812_matrix::set_pixel_color(cell_pixel_pos_x + maze.cell_size - 1, y, maze_color);
					if(current_cell->has_wall(wall_left))
						ws2812_matrix::set_pixel_color(cell_pixel_pos_x, y, maze_color);
				}
			}
		}
	}
}

int16_t get_finish_pixel_coord(const maze_dimension& dim, uint8_t cell_size)
{
	return dim.exit_cell_coord * (cell_size - 1) + 1 /* wall width */
		- dim.pixel_offset - dim.cell_offset * (cell_size - 1);
}

void set_corner_start_and_end(maze_info& maze)
{
	if (rand() % 2)
	{
		maze.dim_x.entrance_cell_coord = 0;
		maze.dim_y.entrance_cell_coord = 0;
		maze.dim_x.exit_cell_coord = static_cast<uint8_t>(maze.dim_x.size - 1);
		maze.dim_y.exit_cell_coord = static_cast<uint8_t>(maze.dim_y.size - 1);
	}
	else
	{
		maze.dim_x.entrance_cell_coord = static_cast<uint8_t>(maze.dim_x.size - 1);
		maze.dim_y.entrance_cell_coord = 0;
		maze.dim_x.exit_cell_coord = 0;
		maze.dim_y.exit_cell_coord = static_cast<uint8_t>(maze.dim_y.size - 1);
	}
	
	if(rand() % 2)
	{
		util::swap(maze.dim_x.entrance_cell_coord, maze.dim_x.exit_cell_coord);
		util::swap(maze.dim_y.entrance_cell_coord, maze.dim_y.exit_cell_coord);
	}
}

constexpr uint8_t character_center_x = (ws2812_matrix::width - 1) / 2;
constexpr uint8_t character_center_y = (ws2812_matrix::height - 1) / 2;

void scroll_character_into_view(maze_dimension& dim, uint8_t cell_size, uint8_t character_center,
	uint8_t display_size)
{
	uint16_t character_offset = static_cast<uint16_t>(dim.entrance_cell_coord) * (cell_size - 1) + 1 /* wall width */;
	
	int16_t needed_offset = character_offset - character_center;
	if(needed_offset > 0)
	{
		uint16_t max_pixel_offset = static_cast<uint16_t>(dim.size) * (cell_size - 1) + 1 - display_size;
		if(static_cast<uint16_t>(needed_offset) > max_pixel_offset)
			needed_offset = max_pixel_offset;
		
		dim.cell_offset = static_cast<uint8_t>(static_cast<uint16_t>(needed_offset) / (cell_size - 1));
		dim.pixel_offset = static_cast<uint8_t>(static_cast<uint16_t>(needed_offset) % (cell_size - 1));
		dim.character_offset = static_cast<uint8_t>(character_offset - needed_offset);
	}
	else
	{
		dim.cell_offset = dim.pixel_offset = 0;
		dim.character_offset = static_cast<uint8_t>(character_offset);
	}
}

void scroll_character_into_view(maze_info& maze)
{
	scroll_character_into_view(maze.dim_x, maze.cell_size, character_center_x, ws2812_matrix::width);
	scroll_character_into_view(maze.dim_y, maze.cell_size, character_center_y, ws2812_matrix::height);
}

void scroll_maze(maze_dimension& dim, int8_t scroll,
	uint8_t display_size, uint8_t character_center,
	uint8_t cell_size)
{
	if(scroll > 0)
	{
		if((dim.size - dim.cell_offset) * (cell_size - 1) - dim.pixel_offset != display_size - 1)
		{
			if(dim.character_offset < character_center)
			{
				++dim.character_offset;
			}
			else if(dim.pixel_offset != cell_size - 2)
			{
				++dim.pixel_offset;
			}
			else
			{
				++dim.cell_offset;
				dim.pixel_offset = 0;
			}
		}
		else
		{
			++dim.character_offset;
		}
	}
	else if(scroll < 0)
	{
		if(dim.character_offset > character_center)
		{
			--dim.character_offset;
		}
		else if(dim.pixel_offset)
		{
			--dim.pixel_offset;
		}
		else if(dim.cell_offset)
		{
			--dim.cell_offset;
			dim.pixel_offset = cell_size - 2;
		}
		else
		{
			--dim.character_offset;
		}
	}
}

bool scroll_maze(maze_info& maze, int8_t scroll_x, int8_t scroll_y)
{
	bool ret = false;
	
	if(scroll_y && !ws2812_matrix::is_on(maze.dim_x.character_offset, maze.dim_y.character_offset + scroll_y))
	{
		ret = true;
		scroll_maze(maze.dim_y, scroll_y, ws2812_matrix::height, character_center_y, maze.cell_size);
	}
	
	if(scroll_x && !ws2812_matrix::is_on(maze.dim_x.character_offset + scroll_x, maze.dim_y.character_offset))
	{
		ret = true;
		scroll_maze(maze.dim_x, scroll_x, ws2812_matrix::width, character_center_x, maze.cell_size);
	}
	
	return ret;
}

void draw_exit_point(int16_t exit_x, int16_t exit_y, const color::rgb& rgb)
{
	if(exit_x >= 0 && exit_y >= 0 && exit_x < ws2812_matrix::width && exit_y < ws2812_matrix::height)
		ws2812_matrix::set_pixel_color(static_cast<uint8_t>(exit_x), static_cast<uint8_t>(exit_y), rgb);
}

void add_random_passages(maze_info& maze, uint8_t probability) //0 <= probability <= 128
{
	for(uint8_t x = 1; x != maze.dim_x.size - 1; ++x)
	{
		for(uint8_t y = 1; y != maze.dim_y.size - 1; ++y)
		{
			if(rand() % 128 < probability)
			{
				wall_position wall = static_cast<wall_position>(1 << (rand() % wall_count));
				maze.at(x, y)->clear_wall(wall);
				switch(wall)
				{
				case wall_down:
					maze.at(x, y + 1)->clear_wall(wall_up);
					break;
				
				case wall_up:
					maze.at(x, y - 1)->clear_wall(wall_down);
					break;
				
				case wall_left:
					maze.at(x - 1, y)->clear_wall(wall_right);
					break;
				
				case wall_right:
					maze.at(x + 1, y)->clear_wall(wall_left);
					break;
				}
			}
		}
	}
}

enum algo_flags : uint8_t
{
	algo_dfs_long_path,
	algo_dfs_corners,
	algo_growing_tree_corners
};

struct level
{
	uint8_t width : 6;
	uint8_t height : 6;
	uint8_t score_multiplier : 4;
	uint8_t allowed_time : 6; // * 10 sec
	uint8_t cell_size : 2; // + 3
	uint8_t flags : 3;
	uint8_t random_passage_probability : 5; //[0 - 31] * 4
};

const level levels[] PROGMEM = {
	{ 3, 5, 1, 2, 1, algo_dfs_long_path, 0 },
	{ 3, 5, 2, 3, 2, algo_dfs_long_path, 0 },
	{ 5, 8, 3, 3, 0, algo_dfs_long_path, 0 },
	{ 12, 5, 3, 3, 1, algo_growing_tree_corners, 0 },
	{ 5, 16, 4, 3, 0, algo_growing_tree_corners, 0 },
	{ 10, 10, 4, 2, 0, algo_dfs_corners, 16 },
	{ 12, 12, 5, 4, 1, algo_dfs_long_path, 15 },
	{ 11, 11, 5, 5, 0, algo_dfs_long_path, 3 },
	{ 35, 35, 6, 6, 0, algo_dfs_corners, 27 },
	{ 20, 20, 6, 9, 1, algo_growing_tree_corners, 6 },
	{ 10, 15, 6, 8, 0, algo_dfs_long_path, 0 },
	{ 15, 15, 7, 5, 0, algo_growing_tree_corners, 0 },
	{ 62, 3, 7, 11, 3, algo_dfs_long_path, 0 },
	{ 15, 15, 7, 10, 1, algo_dfs_corners, 0 },
	{ 30, 8, 8, 11, 0, algo_dfs_long_path, 1 },
	{ 4, 62, 8, 4, 1, algo_growing_tree_corners, 5 },
	{ 5, 45, 9, 7, 0, algo_dfs_corners, 1 },
	{ 20, 20, 9, 14, 0, algo_dfs_long_path, 0 },
	{ 16, 16, 9, 16, 1, algo_dfs_long_path, 0 },
	{ 11, 11, 9, 9, 3, algo_dfs_long_path, 0 },
	{ 27, 18, 10, 15, 0, algo_dfs_corners, 0 },
	{ 16, 30, 10, 25, 1, algo_dfs_long_path, 0 },
	{ 24, 24, 11, 28, 0, algo_dfs_long_path, 0 },
	{ 25, 30, 12, 32, 0, algo_dfs_long_path, 1 },
	{ 32, 32, 14, 45, 0, algo_dfs_long_path, 0 },
	{ 35, 35, 15, 63, 1, algo_dfs_long_path, 0 }
};

constexpr uint8_t max_allowed_time_drop_hard_mode = 25;
constexpr uint8_t max_random_passage_probability_hard_mode = 3;
constexpr uint8_t hard_mode_max_cell_size = 2;
constexpr uint8_t last_level_id = sizeof(levels) / sizeof(levels[0]) - 1;
void load_level(uint8_t& level_id, maze_info& maze, uint8_t& score_multiplier, uint8_t max_brightness,
	uint16_t& seconds_for_level, int16_t& exit_x, int16_t& exit_y)
{
	bool hardmode = false;
	if(level_id > last_level_id)
		hardmode = true;
	
	level info;
	memcpy_P(&info, &levels[hardmode ? last_level_id : level_id], sizeof(info));
	
	//Load last level with some modifications in case if no more levels left
	if(hardmode)
	{
		info.random_passage_probability = rand() % (max_random_passage_probability_hard_mode + 1);
		info.cell_size = rand() % (hard_mode_max_cell_size + 1);
		info.allowed_time -= util::min<uint8_t>(level_id / 8, max_allowed_time_drop_hard_mode);
	}
	
	maze.cell_size = info.cell_size + 3;
	score_multiplier = info.score_multiplier;
	
	maze.init(info.width, info.height);
	switch(info.flags)
	{
		case algo_dfs_long_path:
			generate_maze_dfs(maze);
			break;
		
		case algo_dfs_corners:
			generate_maze_dfs(maze);
			set_corner_start_and_end(maze);
			break;
			
		case algo_growing_tree_corners:
			generate_maze_growing_tree(maze);
			set_corner_start_and_end(maze);
			break;
		
		default:
			break;
	}
	
	if(info.random_passage_probability)
		add_random_passages(maze, info.random_passage_probability * 4);	
	
	game::get_random_color(maze.start_color, max_brightness);
	game::get_random_color(maze.end_color, max_brightness);
	game::get_random_color(maze.exit_color, max_brightness);
	
	seconds_for_level = info.allowed_time * 10;
	number_display::output_number(seconds_for_level);
	
	scroll_character_into_view(maze);
	
	exit_x = get_finish_pixel_coord(maze.dim_x, maze.cell_size);
	exit_y = get_finish_pixel_coord(maze.dim_y, maze.cell_size);
	if(level_id != UINT8_MAX)
		++level_id;
}

//These colors end up in RAM (use 12 bytes)
constexpr color::rgb cold_color { 0, 0, 0xff };
constexpr color::rgb second_cold_color { 0, 0xff, 0 };
constexpr color::rgb warm_color { 0xff, 0, 0 };
constexpr color::rgb second_warm_color { 0xff, 0xff, 0 };
void get_character_color(maze_info& maze, color::rgb& character_color,
	color::rgb& second_character_color, uint8_t max_brightness)
{
	
	int8_t x_character_cell = static_cast<int8_t>(
		(maze.dim_x.character_offset + maze.dim_x.pixel_offset) / (maze.cell_size - 1));
	int8_t y_character_cell = static_cast<int8_t>(
		(maze.dim_y.character_offset + maze.dim_y.pixel_offset) / (maze.cell_size - 1));
	x_character_cell += maze.dim_x.cell_offset;
	y_character_cell += maze.dim_y.cell_offset;
	x_character_cell -= maze.dim_x.exit_cell_coord;
	y_character_cell -= maze.dim_y.exit_cell_coord;
	
	uint8_t distance_to_exit = static_cast<uint8_t>(
		util::isqrt(x_character_cell * x_character_cell + y_character_cell * y_character_cell));
	uint8_t max_distance_to_exit = static_cast<uint8_t>(
		util::isqrt(maze.dim_x.size * maze.dim_x.size + maze.dim_y.size * maze.dim_y.size));
	if(distance_to_exit > max_distance_to_exit)
		distance_to_exit = max_distance_to_exit;
	
	color::gradient(cold_color, warm_color, max_distance_to_exit,
		max_distance_to_exit - distance_to_exit, character_color);
	color::scale_to_brightness(character_color, max_brightness);
	color::gradient(second_cold_color, second_warm_color, max_distance_to_exit,
		max_distance_to_exit - distance_to_exit, second_character_color);
	color::scale_to_brightness(second_character_color, max_brightness);
}

constexpr uint8_t target_exit_draw_counter = 21;
constexpr uint8_t target_character_draw_counter = 4;
constexpr uint8_t target_character_color_gradient_counter = 20;
uint32_t loop()
{
	uint32_t score = 0;
	
	uint8_t max_brightness = options::get_max_brightness();
	
	uint8_t level_id = 0;
	maze_info maze;
	uint8_t score_multiplier = 0;
	uint16_t seconds_for_level = 0, original_seconds_for_level = 0;
	int16_t exit_x = 0, exit_y = 0;
	load_level(level_id, maze, score_multiplier, max_brightness, original_seconds_for_level, exit_x, exit_y);
	seconds_for_level = original_seconds_for_level;
	
	bool refresh = true;
	int8_t scroll_x = 0, scroll_y = 0;
	uint8_t ticks_per_second = static_cast<uint8_t>(timer::frequency);
	uint8_t exit_draw_counter = 0, character_draw_counter = target_character_draw_counter - 1;
	uint8_t character_color_gradient_counter = 0;
	color::rgb character_color, second_character_color,
		result_character_color;
	const color::rgb black { 0, 0, 0 };
	bool exit_point_visible = true;
	
	bool accelerometer_enabled = options::is_accelerometer_enabled();
	accelerometer::speed_state x_speed_state(9), y_speed_state(9);
	get_character_color(maze, character_color, second_character_color, max_brightness);
	while(true)
	{
		timer::wait_for_interrupt();
		
		uint8_t move_dir = move_helper::process_speed(&x_speed_state, &y_speed_state,
			accelerometer_enabled, move_helper::mode_up_down);
		if(move_dir & move_direction_left)
			scroll_x = 1;
		else if(move_dir & move_direction_right)
			scroll_x = -1;
			
		if(move_dir & move_direction_up)
			scroll_y = 1;
		else if(move_dir & move_direction_down)
			scroll_y = -1;
		
		if(buttons::get_button_status(buttons::button_up) == buttons::button_status_still_pressed
			&& buttons::get_button_status(buttons::button_down) == buttons::button_status_still_pressed)
		{
			if(!game::pause())
			{
				number_display::output_number(score);
				return score;
			}
			
			number_display::output_number(seconds_for_level);
			refresh = true;
		}
		
		if(!--ticks_per_second)
		{
			ticks_per_second = static_cast<uint8_t>(timer::frequency);
			if(!--seconds_for_level)
				break; //No more time, game over
			
			number_display::output_number(seconds_for_level);
			refresh = true;
		}
		
		if(scroll_x || scroll_y)
		{
			draw_exit_point(exit_x, exit_y, black);
			
			if(scroll_maze(maze, scroll_x, scroll_y))
			{
				refresh = true;
				exit_x = get_finish_pixel_coord(maze.dim_x, maze.cell_size);
				exit_y = get_finish_pixel_coord(maze.dim_y, maze.cell_size);
				if(exit_x == maze.dim_x.character_offset && exit_y == maze.dim_y.character_offset)
				{
					score += seconds_for_level * score_multiplier;
					load_level(level_id, maze, score_multiplier, max_brightness, original_seconds_for_level, exit_x, exit_y);
					seconds_for_level = original_seconds_for_level;
					ticks_per_second = static_cast<uint8_t>(timer::frequency);
				}
				get_character_color(maze, character_color, second_character_color, max_brightness);
				character_draw_counter = target_character_draw_counter - 1;
			}
			
			draw_exit_point(exit_x, exit_y, exit_point_visible ? maze.exit_color : black);
			scroll_x = scroll_y = 0;
		}
		
		if(++exit_draw_counter == target_exit_draw_counter)
		{
			exit_draw_counter = 0;
			exit_point_visible = !exit_point_visible;
			refresh = true;
		}
		
		if(++character_draw_counter == target_character_draw_counter)
		{
			character_draw_counter = 0;
			color::gradient(character_color, second_character_color,
				target_character_color_gradient_counter / 2,
				character_color_gradient_counter, result_character_color);
			if(++character_color_gradient_counter == target_character_color_gradient_counter)
				character_color_gradient_counter = 0;
			
			refresh = true;
		}
		
		if(refresh)
		{
			refresh = false;
			ws2812_matrix::clear();
			draw_maze(maze, seconds_for_level, original_seconds_for_level);
			ws2812_matrix::set_pixel_color(maze.dim_x.character_offset, maze.dim_y.character_offset,
				result_character_color);
			draw_exit_point(exit_x, exit_y, exit_point_visible ? maze.exit_color : black);
			ws2812_matrix::show();
		}
	}
	
	return score;
}
} //namespace

void maze::run()
{
	game::intro();
	buttons::enable_repeat(buttons::mask_up | buttons::mask_down
		| buttons::mask_right | buttons::mask_left, true);
	
	uint32_t score = loop();
	
	buttons::enable_repeat(buttons::mask_up | buttons::mask_down
		| buttons::mask_right | buttons::mask_left, false);
	
	number_display::clear();
	game::end(score, game::game_maze);
}
