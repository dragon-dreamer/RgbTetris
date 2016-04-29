// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdexcept>

#include "static_class.h"

class display;
class uart;

class device_offline_exception : public std::runtime_error
{
public:
	device_offline_exception();
};

///Allows to send pictures to the device via UART
class display_protocol : static_class
{
public:
	/** Sends data to device
	*   @param data Data to send 
	*   @param com_port UART instance
	*   @throw device_offline_exception in case device doesn't respond */
	static void send_data_to_device(const display& data, uart& com_port);
};
