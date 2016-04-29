// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "static_class.h"

///"Asteroids" game. You control the plane which flies through
///a "cave". Some asteroids fly towards your plane, you can shoot
///them down. Flying speed increases with time, the "cave" also
///becomes more tight.
class flight : static_class
{
public:
	static void run();
};
