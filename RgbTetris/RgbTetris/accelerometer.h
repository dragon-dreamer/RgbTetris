// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

///High-level wrapper for adxl345 API
class accelerometer : static_class
{
public:
	///This structure is used to generate speed_action events
	///depending on slope and speed_measurement_multiplier value.
	///The greater speed_measurement_multiplier is, the faster these
	///events are generated.
	struct speed_state
	{
		explicit speed_state(uint8_t speed_measurement_multiplier)
			: speed_counter(0),
			positive_speed(false),
			speed_measurement_multiplier(speed_measurement_multiplier)
		{
		}
		
		int16_t speed_counter;
		bool positive_speed;
		uint8_t speed_measurement_multiplier;
	};
	
	///Movement action
	enum speed_action : uint8_t
	{
		action_move_left = 0,
		action_move_right = 1,
		action_move_back = 0,
		action_move_fwd = 1,
		action_stay
	};
	
	///Slope direction
	enum direction : uint8_t
	{
		direction_none = 0,
		direction_up = 1 << 0,
		direction_left = 1 << 1,
		direction_right = 1 << 2,
		direction_down = 1 << 3
	};
	
	enum speed_type : uint8_t
	{
		speed_x,
		speed_y
	};
	
	static constexpr int8_t max_speed = INT8_MAX;
	static constexpr int8_t min_speed = INT8_MIN + 1;
	
public:
	///Initializes accelerometer wrapper
	static void init();
	
	///Returns cached values of x, y and z axes.
	///These values are refreshed with ~61 Hz frequency.
	static void get_values(int16_t& x, int16_t& y, int16_t& z);
	
	///Returns accelerometer slope direction
	///(direction which has the biggest slope value)
	static direction get_exclusive_direction();
	
	///Returns X axis speed value
	static int8_t get_x_speed();
	///Returns Y axis speed value
	static int8_t get_y_speed();
	
	/** Returns movement action event
	*   @param state Helper structure for event frequency calculation
	*   @param speed X or Y speed direction
	*   @returns Accelerometer movement event */
	static speed_action process_speed(speed_state& state, speed_type speed);
};
