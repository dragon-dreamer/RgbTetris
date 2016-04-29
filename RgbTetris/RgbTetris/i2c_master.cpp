// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "i2c_master.h"

#include <avr/io.h>
#include <util/twi.h>

namespace
{
bool start(uint8_t address)
{
	TWCR = 0;
	//Transmit START condition
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
	//Wait until START is transmitted
	loop_until_bit_is_set(TWCR, TWINT);
	
	//Check for success
	//TW_MT_ARB_LOST should not happen
	uint8_t status = TW_STATUS;
	if(status != TW_START && status != TW_REP_START)
		return false;
	
	//Set slave address
	TWDR = address;
	//Transmit address
	TWCR = _BV(TWINT) | _BV(TWEN);
	//Wait until address is transmitted
	loop_until_bit_is_set(TWCR, TWINT);
	//Check if device has acknowledged the read or write mode
	status = TW_STATUS;
	//TW_MT_ARB_LOST should not happen
	return status == TW_MT_SLA_ACK || status == TW_MR_SLA_ACK;
}

void stop()
{
	//Transmit STOP condition
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
	//Wait until STOP is transmitted
	loop_until_bit_is_clear(TWCR, TWSTO);
}

bool write(uint8_t data)
{
	//Load data
	TWDR = data;
	//Start transmission
	TWCR = _BV(TWINT) | _BV(TWEN);
	//Wait until data is transmitted
	loop_until_bit_is_set(TWCR, TWINT);
	
	//TW_MT_ARB_LOST / TW_MT_DATA_NACK should not happen
	return TW_STATUS == TW_MT_DATA_ACK;
}

uint8_t read(uint8_t& data, bool ack)
{
	TWCR = ack
		? _BV(TWINT) | _BV(TWEN) | _BV(TWEA)
		: _BV(TWINT) | _BV(TWEN);
	loop_until_bit_is_set(TWCR, TWINT);
	
	uint8_t status = TW_STATUS;
	if(status == TW_MR_DATA_NACK || status == TW_MR_DATA_ACK)
		data = TWDR;
	
	return status;
}

uint8_t receive_internal(uint8_t address, uint8_t* register_address, uint8_t* data, uint8_t length)
{
	address <<= 1;
	
	if(register_address)
	{
		address |= TW_WRITE; //0
		if(!start(address))
			return false;
	
		if(!write(*register_address))
		{
			stop();
			return 0;
		}
	}
	
	address |= TW_READ; //1
	if(!start(address))
	{
		if(register_address)
			stop();
		return 0;
	}
	
	--length;
	for(uint8_t i = 0; i != length; ++i)
	{
		switch(read(data[i], true))
		{
			case TW_MR_DATA_ACK:
				continue;
			
			case TW_MR_DATA_NACK:
				stop();
				return i + 1;
			
			default: //Any error
				stop();
				return i;
		}
	}
	
	uint8_t status = read(data[length], false);
	stop();
	return status == TW_MR_DATA_ACK || status == TW_MR_DATA_NACK
		? length + 1
		: length;
}
} //namespace

void i2c_master::init()
{
	TWBR = ((F_CPU / twi_frequency) - 16) / 2;
	//Prescaler is 1 by default, no need to set TWCR
}

bool i2c_master::transmit(uint8_t address, const uint8_t* data, uint8_t length)
{
	address <<= 1;
	address |= TW_WRITE;
	
	if(!start(address))
		return false;
	
	for(uint8_t i = 0; i != length; ++i)
	{
		if(!write(data[i]))
		{
			stop();
			return false;
		}
	}
	
	stop();
	return true;
}

uint8_t i2c_master::receive(uint8_t address, uint8_t register_address, uint8_t* data, uint8_t length)
{
	return receive_internal(address, &register_address, data, length);
}

uint8_t i2c_master::receive(uint8_t address, uint8_t* data, uint8_t length)
{
	return receive_internal(address, nullptr, data, length);
}
