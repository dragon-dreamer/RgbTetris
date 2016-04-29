// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "game.h"
#include "static_class.h"

///Main menu. Allows to select game or mode, change options or reset high scores.
class mode_selector : static_class
{
public:
	enum mode : uint8_t
	{
		mode_snake = static_cast<uint8_t>(game::game_snake),
		mode_space_invaders = static_cast<uint8_t>(game::game_space_invaders),
		mode_tetris = static_cast<uint8_t>(game::game_tetris),
		mode_asteroids = static_cast<uint8_t>(game::game_asteroids),
		mode_maze = static_cast<uint8_t>(game::game_maze),
		mode_options,
		mode_uart,
		mode_debug,
		max_mode
	};
	
	enum option : uint8_t
	{
		option_accelerometer,
		option_brightness,
		option_reset_high_scores,
		max_option,
		option_back
	};
	
public:
	////Runs mode selection menu. Returns selected option.
	static mode select_mode();
	
	///Runs menu to change options or reset high scores. You
	///can enable or disable accelerometer here and set
	///brightness level using this menu.
	static void edit_options();
};
