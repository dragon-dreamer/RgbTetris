// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

///Move direction enumeration, which is used
///by several classes.
enum move_direction : uint8_t
{
	move_direction_none = 0,
	move_direction_up = 1 << 0,
	move_direction_left = 1 << 1,
	move_direction_right = 1 << 2,
	move_direction_down = 1 << 3
};
