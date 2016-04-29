// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>
#include <string.h>

#include <avr/pgmspace.h>

class base_queue
{
public:
	bool empty() const
	{
		return read_ptr_ == write_ptr_;
	}

	void clear()
	{
		read_ptr_ = write_ptr_ = 0;
	}

protected:
	base_queue()
		:read_ptr_(0),
		write_ptr_(0)
	{
	}

protected:
	uint16_t read_ptr_, write_ptr_;
};

///This is something like fixed-size fast queue with
///possibility of random access to its values.
template<uint16_t Size, typename ElementType = uint8_t>
class queue : public base_queue
{
	static_assert((Size & (Size - 1)) == 0, "Queue size must be a power of 2");
	
public:
	using value_type = ElementType;
	static const uint16_t mask = Size - 1;
	
public:
	queue()
		:base_queue()
	{
	}
	
	bool push_back(ElementType data)
	{
		if(full())
			return false;

		buf_[write_ptr_++ & mask] = data;
		return true;
	}
	
	bool push_back(const char* data)
	{
		if(free_bytes() < strlen(data))
			return false;

		while(*data && push_back(*data))
			++data;

		return true;
	}
	
	bool push_back(const uint8_t* data, uint8_t size)
	{
		if(free_bytes() < size)
			return false;

		while(size-- && push_back(*data))
			++data;

		return true;
	}
	
	bool push_back_P(const char* data)
	{
		if(free_bytes() < strlen_P(data))
			return false;
		
		uint8_t value;
		do
		{
			value = pgm_read_byte(data);
			++data;
		}
		while(value && push_back(value));

		return true;
	}
	
	bool pop_front(ElementType& value)
	{
		if(empty())
			return false;

		value = buf_[read_ptr_++ & mask];
		return true;
	}

	bool get(ElementType& value, uint16_t index) const
	{
		if(empty() || index >= count())
			return false;

		value = buf_[(read_ptr_ + index) & mask];
		return true;
	}

	bool set(ElementType value, uint16_t index)
	{
		if(empty() || index >= count())
			return false;

		buf_[(read_ptr_ + index) & mask] = value;
		return true;
	}

	uint16_t count() const
	{
		return write_ptr_ - read_ptr_;
	}

	uint16_t free_bytes() const
	{
		return static_cast<uint16_t>(Size - count());
	}

	bool full() const
	{
		return (static_cast<uint16_t>(write_ptr_ - read_ptr_) & static_cast<uint16_t>(~mask)) != 0;
	}

private:
	ElementType buf_[Size];
};
