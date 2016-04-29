// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

///5-digit number display
class number_display : static_class
{
public:
	static constexpr uint8_t max_digits = 5;
	static constexpr uint32_t max_number = 99999;
	
public:
	static void init();
	
	///Removes all numbers from display
	static void clear();
	
	/** Outputs number.
	*   @param value Number to display.
	*   @param point_mask This is a mask which configures which digit positions
	*          should have dot symbol displayed.
	*   @param empty_if_zero IF this flag is true, display will be empty in case if value is zero.
	*          Otherwise "0" will be displayed. */
	static void output_number(uint32_t value, uint8_t point_mask, bool empty_if_zero = false);
	
	///Outputs number
	static void output_number(uint32_t value);
	
	///Outputs raw data. See number_display.cpp or schematic
	///for details on number display connection.
	static void output_data(const uint8_t value[max_digits]);
	
	///Returns raw digit data for specified number value (0 to 9).
	static uint8_t get_symbol_data(uint8_t number_value);
};
