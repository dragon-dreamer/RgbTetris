// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

class buttons : static_class
{
public:
	///Button identifiers
	enum button_id : uint8_t
	{
		button_fwd = 0,
		button_back = 1,
		button_left = 2,
		button_right = 3,
		button_directions_max = 4,
		button_up = 4,
		button_down = 5,
		button_extended_directions_max = 5,
		btn_count
	};
	
	///Button statuses
	enum button_status : uint8_t
	{
		button_status_pressed,
		button_status_still_pressed,
		button_status_not_pressed
	};
	
	///Mask enumeration used in enable_repeat function
	///to specify several buttons in a single call
	enum mask : uint8_t
	{
		mask_fwd = 1 << button_fwd,
		mask_back = 1 << button_back,
		mask_left = 1 << button_left,
		mask_right = 1 << button_right,
		mask_up = 1 << button_up,
		mask_down = 1 << button_down
	};
	
public:
	static void init();
	
	///Enables or disables press event repeat for specified button
	static void enable_repeat(button_id id, bool enable);
	
	///Returns true if button was pressed.
	///Next call this function will return false until button
	///is released and pressed again.
	static bool is_pressed(button_id id);
	
	///Returns true if button was pressed and still pressed.
	///Next call this function will return true until button
	///is released.
	static bool is_still_pressed(button_id id);
	
	///Returns button press status
	static button_status get_button_status(button_id id);
	
	///Reset all pressed buttons, as if they were released
	static void flush_pressed();
	
	///Enables or disables press event repeat for specified button(s)
	static void enable_repeat(uint8_t button_mask, bool enable);
};
