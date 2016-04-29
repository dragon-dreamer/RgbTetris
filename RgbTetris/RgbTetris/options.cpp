// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "options.h"

#include <avr/eeprom.h>

namespace
{
struct checked_uint8_t
{
	uint8_t value;
	uint8_t checksum;
};

struct checked_uint32_t
{
	uint32_t value;
	uint8_t checksum;
};

checked_uint8_t max_brightness_saved EEMEM { options::default_max_brightness, options::default_max_brightness };
checked_uint8_t accelerometer_state_saved EEMEM { options::default_accelerometer_state, options::default_accelerometer_state };
checked_uint32_t high_score[game::max_game_id] EEMEM { };

uint8_t calc_checksum(uint32_t value)
{
	return static_cast<uint8_t>(value & 0xFF)
		+ static_cast<uint8_t>((value >> 8) & 0xFF)
		+ static_cast<uint8_t>((value >> 16) & 0xFF)
		+ static_cast<uint8_t>((value >> 24) & 0xFF);
}

bool load_eemem_with_check(const checked_uint8_t* data, uint8_t& result)
{
	checked_uint8_t mem;
	eeprom_read_block(&mem, data, sizeof(mem));
	result = mem.value;
	return mem.checksum == result;
}

bool load_eemem_with_check(const checked_uint32_t* data, uint32_t& result)
{
	checked_uint32_t mem;
	eeprom_read_block(&mem, data, sizeof(mem));
	result = mem.value;
	return mem.checksum == calc_checksum(result);
}

void save_eemem_with_check(checked_uint8_t* data, uint8_t value)
{
	checked_uint8_t mem;
	mem.value = value;
	mem.checksum = value;
	eeprom_update_block(&mem, data, sizeof(mem));
}

void save_eemem_with_check(checked_uint32_t* data, uint32_t value)
{
	checked_uint32_t mem;
	mem.value = value;
	mem.checksum = calc_checksum(value);
	eeprom_update_block(&mem, data, sizeof(mem));
}
} //namespace

uint8_t options::get_max_brightness()
{
	uint8_t result;
	if(!load_eemem_with_check(&max_brightness_saved, result))
		return default_max_brightness;
	
	if(result > max_available_brightness)
		return max_available_brightness;
	
	if(result < min_available_brightness)
		return min_available_brightness;
	
	return result;
}

void options::set_max_brightness(uint8_t value)
{
	save_eemem_with_check(&max_brightness_saved, value);
}

bool options::is_accelerometer_enabled()
{
	uint8_t result;
	return load_eemem_with_check(&accelerometer_state_saved, result)
		? static_cast<bool>(result) : default_accelerometer_state;
}

void options::set_accelerometer_enabled(bool value)
{
	save_eemem_with_check(&accelerometer_state_saved, value);
}

void options::reset_high_scores()
{
	for(uint8_t i = 0; i != game::max_game_id; ++i)
		reset_high_score(static_cast<game::game_id>(i));
}

void options::reset_high_score(game::game_id id)
{
	save_eemem_with_check(&high_score[id], 0);
}

uint32_t options::get_high_score(game::game_id id)
{
	uint32_t result = 0;
	load_eemem_with_check(&high_score[id], result);
	if(result > game::max_score)
		result = game::max_score;
	
	return result;
}

void options::set_high_score(game::game_id id, uint32_t score)
{
	save_eemem_with_check(&high_score[id], score);
}
