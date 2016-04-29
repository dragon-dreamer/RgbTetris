// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

///Classic "Snake" game where you control a snake and eat food, which
///causes your snake to grow. Moving speed also increases as you earn more score.
class snake : static_class
{
public:
	static constexpr uint8_t init_len = 3;
	static constexpr uint8_t max_init_len = 5;
	
public:
	static void run();
};
