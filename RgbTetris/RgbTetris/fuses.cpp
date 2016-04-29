// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include <avr/io.h>
#include <avr/fuse.h>

#ifndef __AVR_ATmega644PA__
//This means, that all inputs and outputs are set for this MCU,
//and all ATMega644PA timers are required.
static_assert(false, "This code is written specifically for ATMega644PA MCU. Be careful when porting.");
#endif

static_assert(sizeof(int) >= 2, "This code is adapted to work with size of int >= 2 bytes");

//16 MHz external oscillator
FUSES =
{
	FUSE_CKSEL3, // .low
	(FUSE_SPIEN & FUSE_BOOTSZ1 & FUSE_BOOTSZ0 & FUSE_EESAVE), // .high
	EFUSE_DEFAULT, // .extended
};
