// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

class effect : static_class
{
public:
	enum class direction : uint8_t
	{
		fwd,
		back,
		left,
		right
	};

public:
	///Draws line of text, which runs in specified direction. PROGMEM version.
	static void marquee_P(const char* text, direction dir, uint8_t text_shift,
		uint16_t delay);
	
	///Draws line of text, which runs in specified direction.
	static void marquee(const char* text, direction dir, uint8_t text_shift,
		uint16_t delay);
};
