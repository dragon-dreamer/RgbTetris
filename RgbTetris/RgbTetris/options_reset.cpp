// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "options_reset.h"

#include "adxl345.h"
#include "buttons.h"
#include "options.h"

void options_reset::reset_if_needed()
{
	if(buttons::is_pressed(buttons::button_up)
		&& buttons::is_pressed(buttons::button_down))
	{
		options::set_max_brightness(options::default_max_brightness);
		options::set_accelerometer_enabled(options::default_accelerometer_state);
		adxl345::enable_measurements(options::default_accelerometer_state);
	}
}
