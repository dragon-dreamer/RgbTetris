// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

///Timer class that ticks with ~ 144 Hz frequency with 16 MHz quartz
class timer : static_class
{
public:
	using on_interrupt_function = void(*)();
	
public:
	static constexpr uint16_t prescaler = 1024;
	static constexpr uint8_t counter_value = 109;
	static constexpr float frequency = (static_cast<float>(F_CPU) / prescaler) / counter_value;
	
public:
	static void init();
	
	///Function func will be called on timer interrupt. Only
	///one function can be specified at a time.
	static void on_interrupt(on_interrupt_function func);
	
	///Waits until timer interrupt occurs.
	///Provides possibility to run code with timer interrupt frequency,
	///but outside of interrupt handler routine.
	static void wait_for_interrupt();
};
