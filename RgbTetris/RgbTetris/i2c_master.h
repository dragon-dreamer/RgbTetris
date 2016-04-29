// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

///Implements I2C master functionality.
class i2c_master : static_class
{
public:
	static constexpr uint32_t twi_frequency = 400000;
	
public:
	///Initializes TWI frequency to twi_frequency.
	static void init();
	
	///Transmits data to slave.
	static bool transmit(uint8_t address, const uint8_t* data, uint8_t length);
	
	///Receives data from slave. Returns number of received bytes.
	static uint8_t receive(uint8_t address, uint8_t* data, uint8_t length);
	
	///Receives data from specified slave register address. Returns number of received bytes.
	static uint8_t receive(uint8_t address, uint8_t register_address, uint8_t* data, uint8_t length);
};
