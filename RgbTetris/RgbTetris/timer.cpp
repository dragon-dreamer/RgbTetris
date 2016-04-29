// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "timer.h"

#include <avr/io.h>
#include <avr/interrupt.h>

namespace
{
volatile bool signaled = false;

void empty_function()
{
}

volatile timer::on_interrupt_function on_interrupt_callback = empty_function;
} //namespace

ISR(TIMER0_COMPA_vect)
{
	on_interrupt_callback();
	signaled = true;
}

void timer::init()
{
	//16000000/1024 = 15625
	//15625 / 109 = ~ 144 Hz
	OCR0A = counter_value;
	TCCR0A = _BV(WGM01); //CTC mode
	TCCR0B = _BV(CS02) | _BV(CS00); //Prescaler = 1024
	TIMSK0 |= (1 << OCIE0A); // Enable CTC interrupt
}

void timer::on_interrupt(on_interrupt_function func)
{
	on_interrupt_callback = func;
}

void timer::wait_for_interrupt()
{
	while(!signaled)
	{
	}
	
	signaled = false;
}
