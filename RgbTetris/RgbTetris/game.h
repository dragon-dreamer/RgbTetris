// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "colors.h"
#include "number_display.h"
#include "static_class.h"

///Game utilities and helpers
class game : static_class
{
public:
	static constexpr uint32_t max_score = number_display::max_number;
	
	//Game list
	enum game_id : uint8_t
	{
		game_snake,
		game_space_invaders,
		game_tetris,
		game_asteroids,
		game_maze,
		max_game_id
	};

public:
	///Helper structure for level configuration used in several games
	struct difficulty_map
	{
		uint32_t score;
		uint8_t multiplier;
		uint8_t difficulty;
	};

public:
	///Displays game intro, running "3... 2... 1..." text.
	static void intro();
	///Ends game, displays "GAME OVER" or "NEW HIGH SCORE" text,
	///saves new high score.
	static void end(uint32_t score, game_id id);
	
	///Pauses game, displays pause animation and blinking
	///"PAUSE" text on number display.
	///Returns true on game continue, false on game exit.
	///Game can be continued by pressing "right" or exit
	///by pressing "left".
	///All games can be paused by pressing "up" and "down" buttons
	///simultaneously.
	static bool pause();
	///Pauses game like pause(), but also backups full screen data
	///and restores it if game continues.
	static bool pause_with_screen_backup();
	
	///Generates random color from fixed color list,
	///then scales it to specified brightness.
	static void get_random_color(color::rgb& value, uint8_t max_brightness);
};
