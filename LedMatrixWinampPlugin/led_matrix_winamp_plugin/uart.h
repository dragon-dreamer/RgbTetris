// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <memory>
#include <stdint.h>
#include <stdexcept>
#include <string>
#include <set>

struct uart_impl;

class uart_exception : public std::runtime_error
{
public:
	uart_exception(const std::string& message)
		: std::runtime_error(message)
	{
	}
};

///Generalized UART class
class uart
{
public:
	typedef std::set<std::wstring> uart_port_list;

public:
	explicit uart(const std::wstring& name, uint32_t baud_rate);
	~uart();

	static uart_port_list get_available_ports();

	void write_byte(uint8_t byte);
	void write_data(const uint8_t* data, uint32_t size);
	uint8_t read_byte();

private:
	void close();

	std::unique_ptr<uart_impl> impl_;
};
