// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "buttons.h"

#include <avr/io.h>
#include <avr/sfr_defs.h>

#include "timer.h"

//Button connection definitions
#define BTNS_PORT PINC
#define BTN_FWD_PIN PC3
#define BTN_BACK_PIN PC4
#define BTN_LEFT_PIN PC2
#define BTN_RIGHT_PIN PC5
#define BTN_UP_PIN PC7
#define BTN_DOWN_PIN PC6

namespace
{
constexpr uint8_t button_pressed = 5;
constexpr uint8_t button_press_processed = UINT8_MAX - 2;
constexpr uint8_t button_still_pressed = UINT8_MAX - 1;

constexpr uint8_t button_pressed_repeat_counter = 50;
constexpr uint8_t button_pressed_repeat_initial_counter_value = 37;
constexpr uint8_t repeat_disabled = UINT8_MAX;

struct button_info
{
	uint8_t pin_mask;
	uint8_t button_state;
	uint8_t button_press_counter;
};

button_info info[buttons::btn_count] = {
	{ _BV(BTN_FWD_PIN), 0, repeat_disabled },
	{ _BV(BTN_BACK_PIN), 0, repeat_disabled },
	{ _BV(BTN_LEFT_PIN), 0, repeat_disabled },
	{ _BV(BTN_RIGHT_PIN), 0, repeat_disabled },
	{ _BV(BTN_UP_PIN), 0, repeat_disabled },
	{ _BV(BTN_DOWN_PIN), 0, repeat_disabled }
};

//This function is called on timer interrupt (see init())
void on_interrupt()
{
	const uint8_t button_values = BTNS_PORT;
	uint8_t state;
	for(uint8_t i = 0; i != sizeof(info) / sizeof(info[0]); ++i)
	{
		state = info[i].button_state;
		if((button_values & info[i].pin_mask) && (state != button_pressed))
		{
			state = 0;
			if(info[i].button_press_counter != repeat_disabled)
				info[i].button_press_counter = 0;
		}
		else
		{
			++state;
			
			if(state > button_press_processed)
			{
				if(info[i].button_press_counter != repeat_disabled
					&& ++info[i].button_press_counter == button_pressed_repeat_counter)
				{
					info[i].button_press_counter = button_pressed_repeat_initial_counter_value;
					state = button_pressed;
				}
				else
				{
					--state;
				}
			}
			else if(state > button_pressed)
			{
				state = button_pressed;
			}
		}
		
		info[i].button_state = state;
	}
}
} //namespace

void buttons::enable_repeat(button_id id, bool enable)
{
	info[id].button_press_counter = enable ? 0 : repeat_disabled;
}

void buttons::enable_repeat(uint8_t button_mask, bool enable)
{
	for(uint8_t i = 0; i != static_cast<uint8_t>(btn_count); ++i)
	{
		if(button_mask & (1 << i))
			enable_repeat(static_cast<button_id>(i), enable);
	}
}

bool buttons::is_pressed(button_id button)
{
	if(info[button].button_state == button_pressed)
	{
		info[button].button_state = button_still_pressed;
		return true;
	}
	
	return false;
}

bool buttons::is_still_pressed(button_id button)
{
	if(info[button].button_state == button_pressed)
		info[button].button_state = button_still_pressed;
	
	return info[button].button_state == button_still_pressed;
}

buttons::button_status buttons::get_button_status(button_id button)
{
	switch(info[button].button_state)
	{
		case button_pressed:
			info[button].button_state = button_still_pressed;
			return button_status_pressed;
		
		case button_still_pressed:
			return button_status_still_pressed;
		
		default:
			return button_status_not_pressed;
	}
}

void buttons::flush_pressed()
{
	for(uint8_t i = 0; i != buttons::btn_count; ++i)
		info[i].button_state = button_press_processed;
}

void buttons::init()
{
	timer::on_interrupt(on_interrupt);
}
