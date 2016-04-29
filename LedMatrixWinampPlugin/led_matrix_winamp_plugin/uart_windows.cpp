// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "uart.h"

#include <Windows.h>

#include <vector>

struct uart_impl
{
	uart_impl()
		: com_handle(INVALID_HANDLE_VALUE)
		, timeouts_set(false)
		, working(false)
		, something_sent(false)
	{
	}

	HANDLE com_handle;
	COMMTIMEOUTS old_timeouts;
	bool timeouts_set;
	bool working;
	bool something_sent;
};

uart::uart(const std::wstring& name, uint32_t baud_rate)
{
	try
	{
		impl_.reset(new uart_impl);

		impl_->com_handle = ::CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
			nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(impl_->com_handle == INVALID_HANDLE_VALUE)
			throw uart_exception("Unable to open COM port");

		if(!::GetCommTimeouts(impl_->com_handle, &impl_->old_timeouts))
			throw uart_exception("Unable to get COM port timeouts");

		COMMTIMEOUTS new_timeouts;
		new_timeouts.ReadIntervalTimeout = 0;
		new_timeouts.ReadTotalTimeoutMultiplier = 1000;
		new_timeouts.WriteTotalTimeoutMultiplier = 1000;
		new_timeouts.ReadTotalTimeoutConstant = 1000;
		new_timeouts.WriteTotalTimeoutConstant = 1000;
		if(!::SetCommTimeouts(impl_->com_handle, &new_timeouts))
			throw uart_exception("Unable to set COM port timeouts");

		impl_->timeouts_set = true;

		DCB com_init;
		com_init.DCBlength = sizeof(DCB);
		if(!::GetCommState(impl_->com_handle, &com_init))
			throw uart_exception("Unable to get COM port state");

		if(!::BuildCommDCBW((L"baud=" + std::to_wstring(baud_rate) + L" parity=N data=8 stop=1").c_str(), &com_init))
			throw uart_exception("Unable to build COM port DCB");

		if(!::SetCommState(impl_->com_handle, &com_init))
			throw uart_exception("Unable to set COM port state");

		if(!::SetCommMask(impl_->com_handle, EV_TXEMPTY))
			throw uart_exception("Unable to set COM port mask");

		impl_->working = true;
	}
	catch(const std::exception&)
	{
		close();
		throw;
	}
}

void uart::close()
{
	if(impl_->working && impl_->something_sent)
	{
		DWORD event_mask = 0;
		while(true)
		{
			if(::WaitCommEvent(impl_->com_handle, &event_mask, 0))
			{
				if(event_mask & EV_TXEMPTY)
					break;
			}

			::Sleep(10);
		}
	}

	if(impl_->timeouts_set)
		::SetCommTimeouts(impl_->com_handle, &impl_->old_timeouts);

	if(impl_->com_handle != INVALID_HANDLE_VALUE)
		::CloseHandle(impl_->com_handle);
}

uart::~uart()
{
	close();
}

namespace
{
struct reg_key_holder
{
	reg_key_holder(HKEY key)
		: key(key)
	{
	}

	reg_key_holder()
		: key(nullptr)
	{
	}

	~reg_key_holder()
	{
		if(key != nullptr)
			::RegCloseKey(key);
	}

	operator HKEY*()
	{
		return &key;
	}

	operator HKEY()
	{
		return key;
	}

	HKEY key;

	reg_key_holder(const reg_key_holder&) = delete;
	reg_key_holder& operator=(const reg_key_holder&) = delete;
};
} //namespace

uart::uart_port_list uart::get_available_ports()
{
	uart_port_list ret;

	reg_key_holder key;
	if(ERROR_SUCCESS != ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, key))
		throw uart_exception("Unable to open registry key HKLM\\HARDWARE\\DEVICEMAP\\SERIALCOMM");
	
	DWORD num_of_values = 0;
	DWORD max_value_name_len = 0;
	DWORD max_value_data_len = 0;
	if(ERROR_SUCCESS != ::RegQueryInfoKeyW(key, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		&num_of_values, &max_value_name_len, &max_value_data_len, nullptr, nullptr))
	{
		throw uart_exception("Unable to enumerate registry key values");
	}

	if(!num_of_values)
		return ret;

	DWORD value_name_buffer_size = max_value_name_len + 1;
	DWORD value_data_buffer_size = max_value_data_len + 1;
	std::vector<wchar_t> name, data;
	name.resize(value_name_buffer_size);
	data.resize(value_data_buffer_size);

	for(DWORD i = 0; i != num_of_values; ++i)
	{
		value_name_buffer_size = max_value_name_len + 1;
		value_data_buffer_size = max_value_data_len + 1;
		DWORD type = 0;
		if(ERROR_SUCCESS != ::RegEnumValueW(key, i, name.data(), &value_name_buffer_size, NULL, &type,
			reinterpret_cast<LPBYTE>(data.data()), &value_data_buffer_size))
		{
			throw uart_exception("Unable to enumerate registry key values");
		}

		if(type == REG_SZ)
			ret.insert(data.data());
	}

	return ret;
}

void uart::write_byte(uint8_t byte)
{
	write_data(&byte, sizeof(byte));
}

uint8_t uart::read_byte()
{
	uint8_t ret = 0;
	DWORD read = 0;
	if(!::ReadFile(impl_->com_handle, &ret, sizeof(ret), &read, nullptr) || read != sizeof(ret))
		throw uart_exception("Unable to read byte");

	return ret;
}

void uart::write_data(const uint8_t* data, uint32_t size)
{
	DWORD written = 0;
	impl_->something_sent = true;
	if(!::WriteFile(impl_->com_handle, data, size, &written, 0) || written != size)
		throw uart_exception("Unable to write byte");
}