// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "accelerometer.h"

#include <stdlib.h>

#include <avr/io.h>

#include "adxl345.h"

namespace
{
int16_t cached_x = 0, cached_y = 0, cached_z = 0;
bool swap_up_down_thresholds = false;

//Full 255 TCNT2 value.
//As timer overflows in 0.01632 sec (16 MHz, prescaler = 1024),
//This is approx. equal to 0.01632 sec = 61 Hz
constexpr uint8_t timer_threshold = 254;

constexpr int16_t direction_threshold_x = 90;
constexpr int16_t direction_threshold_up = 35;
constexpr int16_t direction_threshold_down = 100;
constexpr int8_t speed_action_addition_coeff = 1;
constexpr int16_t speed_threshold = 45;

void get_values_internal()
{
	if(TCNT2 > timer_threshold || bit_is_set(TIFR2, TOV2))
	{
		//Too much time elapsed, renew accelerometer values
		//This operation takes approximately 320us = 0.00032 sec on 400Hz TWI
		adxl345::get_values(cached_x, cached_y, cached_z);
		if(cached_z < 0) //Device is upside down
		{
			cached_x = -cached_x;
			cached_y = -cached_y;
			swap_up_down_thresholds = true;
		}
		else
		{
			swap_up_down_thresholds = false;
		}
		
		TCNT2 = 0;
		TIFR2 |= _BV(TOV2); //Clear timer2 counter overflow (inversed bit)
	}
}

int8_t limit_speed(int16_t value)
{
	if(abs(value) < speed_threshold)
		return 0;
	
	int16_t speed = value / 2;
	
	if(speed > accelerometer::max_speed)
		speed = accelerometer::max_speed;
	else if(speed < accelerometer::min_speed)
		speed = accelerometer::min_speed;
	
	return static_cast<int8_t>(speed);
}
} //namespace

void accelerometer::init()
{
	TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20); //Set prescaler 1024 for timer 2
	TCNT2 = 0xff; //Overflow timer
}

void accelerometer::get_values(int16_t& x, int16_t& y, int16_t& z)
{
	get_values_internal();
	x = cached_x;
	y = cached_y;
	z = cached_z;
}

accelerometer::direction accelerometer::get_exclusive_direction()
{
	get_values_internal();
	
	int16_t threshold_up, threshold_down;
	if(swap_up_down_thresholds)
	{
		threshold_up = direction_threshold_down;
		threshold_down = direction_threshold_up;
	}
	else
	{
		threshold_up = direction_threshold_up;
		threshold_down = direction_threshold_down;
	}
	
	if(cached_x < -direction_threshold_x)
	{
		if(cached_y < -threshold_up && cached_y < cached_x)
			return direction_up;
		
		return direction_right;
	}
	else if(cached_x > direction_threshold_x)
	{
		if(cached_y > threshold_down && cached_y > cached_x)
			return direction_down;
		
		return direction_left;
	}
	
	if(cached_y < -threshold_up)
		return direction_up;
	else if(cached_y > threshold_down)
		return direction_down;
	
	return direction_none;
}

int8_t accelerometer::get_x_speed()
{
	get_values_internal();
	return limit_speed(cached_x);
}

int8_t accelerometer::get_y_speed()
{
	get_values_internal();
	return limit_speed(cached_y);
}

accelerometer::speed_action accelerometer::process_speed(speed_state& state, speed_type type)
{
	int8_t speed = type == speed_x ? get_x_speed() : get_y_speed();
	if(!speed)
	{
		state.speed_counter = 0;
	}
	else if(speed > 0)
	{
		if(!state.positive_speed)
		{
			state.speed_counter = 0;
			state.positive_speed = true;
		}
		
		if(++state.speed_counter > (max_speed - speed) * 2 / state.speed_measurement_multiplier + speed_action_addition_coeff)
		{
			state.speed_counter = 0;
			return action_move_left; //aka action_move_back
		}
	}
	else //speed < 0
	{
		if(state.positive_speed)
		{
			state.speed_counter = 0;
			state.positive_speed = false;
		}
		
		if(++state.speed_counter > (max_speed + speed) * 2 / state.speed_measurement_multiplier + speed_action_addition_coeff)
		{
			state.speed_counter = 0;
			return action_move_right; //aka action_move_fwd
		}
	}
	
	return action_stay;
}
