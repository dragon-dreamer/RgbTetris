// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "accelerometer.h"
#include "move_direction.h"
#include "static_class.h"

///Helper class to calculate movement for accelerometer and buttons universally.
class move_helper : static_class
{
public:
	///Which buttons should be treated as up and down buttons.
	enum button_vertical_mode : uint8_t
	{
		mode_up_down,
		mode_fwd_back
	};
	
public:
	///Calculates and returns move direction each call. Universally processes
	///buttons and accelerometer.
	static move_direction process_speed(accelerometer::speed_state* x_state,
		accelerometer::speed_state* y_state, bool accelerometer_enabled,
		button_vertical_mode button_mode = mode_fwd_back);
};
