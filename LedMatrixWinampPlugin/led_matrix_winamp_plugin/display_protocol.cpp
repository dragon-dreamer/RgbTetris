// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "display_protocol.h"

#include <stdint.h>
#include <vector>

#include "display.h"
#include "uart.h"

device_offline_exception::device_offline_exception()
	: std::runtime_error("")
{
}

void display_protocol::send_data_to_device(const display& data, uart& com_port)
{
	std::vector<uint8_t> data_bytes { 0xff, 0x00 }; //Sync bytes
	data_bytes.reserve(display::display_height * display::display_width * display::bytes_per_led
		+ 2 /*start bytes */);

	for(uint8_t y = 0; y != display::display_height; ++y)
	{
		for(uint8_t x = 0; x != display::display_width; ++x)
		{
			auto pixel = data.get_pixel(x, y);

			data_bytes.push_back(pixel.r);
			if(pixel.r == 0xff)
				data_bytes.push_back(0xff); //Double 0xff byte, as it has special meaning

			data_bytes.push_back(pixel.g);
			if(pixel.g == 0xff)
				data_bytes.push_back(0xff);

			data_bytes.push_back(pixel.b);
			if(pixel.b == 0xff)
				data_bytes.push_back(0xff);
		}
	}

	try
	{
		static const uint8_t device_ready_bytes = 0x78;
		com_port.write_data(data_bytes.data(), data_bytes.size());
		while(com_port.read_byte() != device_ready_bytes)
		{
		}
	}
	catch(const std::exception&)
	{
		throw device_offline_exception();
	}
}
