// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "util.h"

#include <avr/cpufunc.h>

void util::delay(uint16_t x)
{
	for(; x; --x)
	{
		for(uint8_t y = 0; y != 100; ++y)
		{
			for(uint8_t z = 0; z != 8; ++z)
			{
				_NOP();
			}
		}
	}
}

static_assert(sizeof(util::packed_coord) == sizeof(uint8_t),
	"Wrong size of util::packed_coord structure");
static_assert(sizeof(util::coord) == sizeof(uint16_t),
	"Wrong size of util::coord structure");

uint16_t util::isqrt(uint16_t n)
{
	uint16_t root = 0, remainder = n, place = 1 << 14; //second-to-top bit set
		
	while(place > remainder)
		place = place >> 2;
		
	while(place)
	{
		if(remainder >= root + place)
		{
			remainder -= root + place;
			root += (place << 1);
		}
			
		root = root >> 1;
		place = place >> 2;
	}
		
	return root;
}
