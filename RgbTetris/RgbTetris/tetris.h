// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "static_class.h"

///Classic "Tetris" game. The more score you earn, the
///faster is blocks fall speed. Some blocks are also added
///to the game after you reach certain score value.
class tetris : static_class
{
public:
	static void run();
};
