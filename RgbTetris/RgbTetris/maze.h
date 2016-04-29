// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "static_class.h"

///"Maze" game. Generates some random mazes, which become more difficult
///each level. You have limited time to find exit.
class maze : static_class
{
public:
	static void run();
};
