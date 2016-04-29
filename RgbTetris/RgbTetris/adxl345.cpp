// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include <limits.h>

#include "adxl345.h"
#include "i2c_master.h"

bool adxl345::begin()
{
	return get_device_id() == device_id;
}

void adxl345::enable_measurements(bool enable)
{
	write_register(device_register::POWER_CTL, enable ? 0x08 : 0);
}

uint8_t adxl345::get_device_id()
{
	return read_register(device_register::DEVID);
}

void adxl345::set_range(range value)
{
	uint8_t format = read_register(device_register::DATA_FORMAT);

	format &= ~0x0F;
	format |= static_cast<uint8_t>(value);
	//Set full res bit
	format |= 0x08;
	
	write_register(device_register::DATA_FORMAT, format);
}

adxl345::range adxl345::get_range()
{
	return static_cast<range>(read_register(device_register::DATA_FORMAT) & 0x03);
}

void adxl345::set_data_rate(datarate data_rate)
{
	write_register(device_register::BW_RATE, static_cast<uint8_t>(data_rate));
}

adxl345::datarate adxl345::get_data_rate()
{
	return static_cast<datarate>(read_register(device_register::BW_RATE) & 0x0F);
}

void adxl345::write_register(device_register reg, uint8_t value)
{
	uint8_t data[2] = { static_cast<uint8_t>(reg), value };
	i2c_master::transmit(adxl345_address, data, sizeof(data));
}

uint8_t adxl345::read_register(device_register reg)
{
	uint8_t data = 0;
	i2c_master::receive(adxl345_address, static_cast<uint8_t>(reg), &data, sizeof(data));
	return data;
}

bool adxl345::get_values(int16_t& x, int16_t& y, int16_t& z)
{
	uint8_t data[6];
	if(i2c_master::receive(adxl345_address, static_cast<uint8_t>(device_register::DATAX0),
		data, sizeof(data)) != sizeof(data))
	{
		return false;
	}
	
	x = static_cast<int16_t>(data[0]) | (static_cast<int16_t>(data[1]) << 8);
	y = static_cast<int16_t>(data[2]) | (static_cast<int16_t>(data[3]) << 8);
	z = static_cast<int16_t>(data[4]) | (static_cast<int16_t>(data[5]) << 8);
	return true;
}
