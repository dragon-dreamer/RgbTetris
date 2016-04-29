// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "static_class.h"

///This class is used for options reset before boot (before menu is shown).
///This is needed if you set too high brightness, and device doesn't work
///with weak power supply (hangs or smth). This will reset brightness to its default
///level (see options.h) before drawing something on display. This also turns
///accelerometer off.
class options_reset : static_class
{
public:
	///Resets options if "up" and "down" buttons
	///are pressed simultaneously before powering the device on.
	static void reset_if_needed();
};
