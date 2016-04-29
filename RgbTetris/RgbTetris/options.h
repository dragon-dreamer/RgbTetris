// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "game.h"
#include "static_class.h"

///Options and high scores manager. Deals with EEPROM.
class options : static_class
{
public:
	//Default option values and some limits
	static constexpr uint8_t default_max_brightness = 25;
	///Be careful with high brightness, as fully lit LEDs can
	///consume up to 10A current (50W)! You should have
	///a decent power supply.
	static constexpr uint8_t max_available_brightness = 200;
	static constexpr uint8_t min_available_brightness = 15;
	static constexpr bool default_accelerometer_state = false;

public:
	static uint8_t get_max_brightness();
	static void set_max_brightness(uint8_t value);
	
	static bool is_accelerometer_enabled();
	static void set_accelerometer_enabled(bool value);
	
	static void reset_high_scores();
	static void reset_high_score(game::game_id id);
	static uint32_t get_high_score(game::game_id id);
	static void set_high_score(game::game_id id, uint32_t score);
};
