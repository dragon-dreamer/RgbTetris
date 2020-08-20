// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

///Helper utilities
class util : static_class
{
public:
	///Packed coordinate structure (1 byte size)
	struct packed_coord
	{
		uint8_t x : 4;
		uint8_t y : 4;
		
		operator uint8_t() const
		{
			return *reinterpret_cast<const uint8_t*>(this);
		}
	};
	
	///Coordinate structure (2 byte size)
	struct coord
	{
		uint8_t x;
		uint8_t y;
		
		operator uint8_t() const
		{
			return *reinterpret_cast<const uint16_t*>(this);
		}
	};

public:
	///Not calibrated delay
	static void delay(uint16_t delay);
	
	template<typename T>
	static constexpr T max(T a, T b)
	{
		return a > b ? a : b;
	}
	
	template<typename T>
	static constexpr T min(T a, T b)
	{
		return a < b ? a : b;
	}
	
	template<typename T>
	static void swap(T& a, T& b)
	{
		T temp = a;
		a = b;
		b = temp;
	}
	
	///Fast integer square root
	static uint16_t isqrt(uint16_t n);
	
	static constexpr uint16_t round_to_power_of_2(uint16_t value)
	{
		--value;
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		++value;
		return value;
	}
	
	template<bool B, typename T, typename F>
	struct conditional { using type = T; };
	
	template<typename T, typename F>
	struct conditional<false, T, F> { using type = F; };
	
	template<bool B, typename T, typename F>
	using conditional_t = typename conditional<B, T, F>::type;
	
	template<uint8_t MatrixWidth, uint8_t MatrixHeight>
	using smallest_coord = conditional_t<(MatrixWidth > 16 || MatrixHeight > 16), coord, packed_coord>;
};
