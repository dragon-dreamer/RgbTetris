// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "move_helper.h"

#include "buttons.h"

move_direction move_helper::process_speed(accelerometer::speed_state* x_state,
	accelerometer::speed_state* y_state, bool accelerometer_enabled,
	button_vertical_mode button_mode)
{
	uint8_t direction = move_direction_none;
	if(accelerometer_enabled)
	{
		if(x_state)
		{
			switch(accelerometer::process_speed(*x_state,
				accelerometer::speed_x))
			{
				case accelerometer::action_move_left:
					direction |= move_direction_left;
					break;
				
				case accelerometer::action_move_right:
					direction |= move_direction_right;
					break;
				
				default:
					break;
			}
		}
		
		if(y_state)
		{
			switch(accelerometer::process_speed(*y_state,
				accelerometer::speed_y))
			{
				case accelerometer::action_move_fwd:
					direction |= move_direction_up;
					break;
				
				case accelerometer::action_move_back:
					direction |= move_direction_down;
					break;
				
				default:
					break;
			}
		}
	}
	else //button control
	{
		if(x_state)
		{
			if(buttons::is_pressed(buttons::button_left))
				direction |= move_direction_left;
			else if(buttons::is_pressed(buttons::button_right))
				direction |= move_direction_right;
		}
		
		if(y_state)
		{
			if(button_mode == mode_fwd_back)
			{
				if(buttons::is_pressed(buttons::button_fwd))
					direction |= move_direction_up;
				else if(buttons::is_pressed(buttons::button_back))
					direction |= move_direction_down;
			}
			else
			{
				if(buttons::is_pressed(buttons::button_up))
					direction |= move_direction_up;
				else if(buttons::is_pressed(buttons::button_down))
					direction |= move_direction_down;
			}
		}
	}
	
	return static_cast<move_direction>(direction);
}
