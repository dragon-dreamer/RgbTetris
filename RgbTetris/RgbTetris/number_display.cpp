// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "number_display.h"

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/pgmspace.h>

//Number display control pins connection
#define INDICATOR_PORT PORTA
#define INDICATOR_DDR DDRA
#define DS PA2
#define SHCP PA0
#define STCP PA1

namespace
{
//"dot" symbol pin number
constexpr uint8_t dp_offset = 1;
///Digit symbols data
const uint8_t digit_symbols[] PROGMEM = {
	0b01111101, //0
	0b00001001, //1
	0b11010101, //2
	0b10011101, //3
	0b10101001, //4
	0b10111100, //5
	0b11111100, //6
	0b00001101, //7
	0b11111101, //8
	0b10111101  //9
};
} //namespace

void number_display::init()
{
	DDRA |= _BV(DS) | _BV(SHCP) | _BV(STCP);
}

uint8_t number_display::get_symbol_data(uint8_t number_value)
{
	return pgm_read_byte(&digit_symbols[number_value]);
}

void number_display::clear()
{
	for(uint8_t digit = 0; digit != max_digits; ++digit)
	{
		for(uint8_t i = 0; i != 8; ++i)
		{
			INDICATOR_PORT |= _BV(SHCP);
			INDICATOR_PORT &= ~_BV(SHCP);
		}
	}
	
	INDICATOR_PORT |= _BV(STCP);
	INDICATOR_PORT &= ~_BV(STCP);
}

void number_display::output_data(const uint8_t value[max_digits])
{
	for(uint8_t digit = max_digits; digit; --digit)
	{
		uint8_t digit_value = value[digit - 1];
		for(uint8_t i = 0; i != 8; ++i)
		{
			if(digit_value & (1 << i))
				INDICATOR_PORT |= _BV(DS);
			
			INDICATOR_PORT |= _BV(SHCP);
			INDICATOR_PORT &= ~_BV(SHCP);
			
			INDICATOR_PORT &= ~_BV(DS);
		}
	}
	
	INDICATOR_PORT |= _BV(STCP);
	INDICATOR_PORT &= ~_BV(STCP);
}

void number_display::output_number(uint32_t value)
{
	output_number(value, 0, false);
}

void number_display::output_number(uint32_t value, uint8_t point_mask, bool empty_if_zero)
{
	uint8_t digits[max_digits] = {};
	uint8_t zeros_position = max_digits;
	if(empty_if_zero && !value)
	{
		zeros_position = 0;
	}
	else
	{
		for(uint8_t digit = 0; digit != max_digits; ++digit)
		{
			const uint8_t digit_result = value % 10;
			digits[digit] = pgm_read_byte(&digit_symbols[digit_result]);
			value /= 10;
			if(!digit_result)
			{
				if(digit && zeros_position == max_digits)
					zeros_position = digit;
			}
			else
			{
				zeros_position = max_digits;
			}
		}
	}
	
	for(uint8_t digit = 0; digit != max_digits; ++digit)
	{
		uint8_t digit_value = digits[digit];
		if(digit >= zeros_position)
			digit_value = 0;
		
		if(point_mask & (1 << digit))
			digit_value |= (1 << dp_offset);
		
		for(uint8_t i = 0; i != 8; ++i)
		{
			if(digit_value & (1 << i))
				INDICATOR_PORT |= _BV(DS);
			
			INDICATOR_PORT |= _BV(SHCP);
			INDICATOR_PORT &= ~_BV(SHCP);
			
			INDICATOR_PORT &= ~_BV(DS);
		}
	}
	
	INDICATOR_PORT |= _BV(STCP);
	INDICATOR_PORT &= ~_BV(STCP);
}