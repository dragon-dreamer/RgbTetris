// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

//Uses i2c
class adxl345 : static_class
{
public:
	static constexpr uint8_t adxl345_address = 0x53; //ALT address pin low
	static constexpr uint8_t device_id = 0xE5;
	
	enum class datarate : uint8_t
	{
		rate_0p10 = 0b0000, //default
		rate_0p20 = 0b0001,
		rate_0p39 = 0b0010,
		rate_0p78 = 0b0011,
		rate_1p56 = 0b0100,
		rate_3p13 = 0b0101,
		rate_6p25 = 0b0110,
		rate_12p5 = 0b0111,
		rate_25 = 0b1000,
		rate_50 = 0b1001,
		rate_100 = 0b1010,
		rate_200 = 0b1011,
		rate_400 = 0b1100,
		rate_800 = 0b1101,
		rate_1600 = 0b1110,
		rate_3200 = 0b1111
	};

	enum class range : uint8_t
	{
		range_2g = 0b00, //default
		range_4g = 0b01,
		range_8g = 0b10,
		range_16g = 0b11
	};
	
	enum class device_register : uint8_t
	{
		DEVID = 0x00,
		THRESH_TAP = 0x1d,
		OFSX = 0x1e,
		OFSY = 0x1f,
		OFSZ = 0x20,
		DUR = 0x21,
		LATENT = 0x22,
		WINDOW = 0x23,
		THRESH_ACT = 0x24,
		THRESH_INACT = 0x25,
		TIME_INACT = 0x26,
		ACT_INACT_CTL = 0x27,
		THRESH_FF = 0x28,
		TIME_FF = 0x29,
		TAP_AXES = 0x2a,
		ACT_TAP_STATUS = 0x2b,
		BW_RATE = 0x2c,
		POWER_CTL = 0x2d,
		INT_ENABLE = 0x2e,
		INT_MAP = 0x2f,
		INT_SOURCE = 0x30,
		DATA_FORMAT = 0x31,
		DATAX0 = 0x32,
		DATAX1 = 0x33,
		DATAY0 = 0x34,
		DATAY1 = 0x35,
		DATAZ0 = 0x36,
		DATAZ1 = 0x37,
		FIFO_CTL = 0x38,
		FIFO_STATUS = 0x39
	};
	
public:
	static bool begin();
	static void enable_measurements(bool enable);
	
	static void set_range(range value);
	static range get_range();
	
	static void set_data_rate(datarate data_rate);
	static datarate get_data_rate();
	
	static bool get_values(int16_t& x, int16_t& y, int16_t& z);

	static uint8_t get_device_id();
	
	static void write_register(device_register reg, uint8_t value);
	static uint8_t read_register(device_register reg);
};
