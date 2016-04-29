// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "static_class.h"

///Draws color changing dot on display. This dot can be moved
///either using buttons or accelerometer, depending on settings.
///Draws dot coordinates on number display, too.
///Can be stopped by pressing up and down buttons simultaneously.
class debugger : static_class
{
public:
	static void run();
};
