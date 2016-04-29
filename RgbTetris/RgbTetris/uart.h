// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <stdint.h>

#include "static_class.h"

///Starts UART and waits for data to be sent from other device (e.g. computer).
///You can exit this mode by pressing "up" and "down" buttons simultaneously.
/** Protocol description:
*   - Baud rate is 115200, 8 bit data, 1 stop bit.
*   - On power on device waits for incoming bytes.
*   - Each incoming byte is color component. Each LED has 3 color components (R, G, B).
*   - When 3 * display_height * display_width bytes are received, device will show the picture
*     on display and send "ready" packet (byte 0x78) after the picture is drawn. Sender must wait for
*     "ready" packet before sending other packets.
*   - 0xff is special byte value. Next byte that follows 0xff describes what to do next (see command_id
*     description for more information). If the byte that follows 0xff is 0xff, too, then this is
*     color byte 0xff. */
class uart : static_class
{
public:
	///Special command byte value. The byte that follows this byte
	///determines what to do. Allowed bytes described in command_id
	///anumeration.
	static constexpr uint8_t command_byte = 0xff;
	
	///This enumeration defines supported commands. It's nessessary to wait for
	///ready_sequence before executing next command (except sync_display_coords, which returns nothing),
	///otherwise incoming bytes can be dropped.
	enum class command_id : uint8_t
	{
		///Set color byte to 0xff (two bytes 0xff, 0xff translate to color byte 0xff)
		color_component = command_byte,
		
		///Set display coordinates to [x = 0; y = 0]
		sync_display_coords = 0x00,
		
		///Request display brightness value
		get_brightness = 0x01,
		
		///Sets display brightness value. Next display frame will have
		///new brightness. New value will be saved to EEPROM on UART mode exit.
		set_brightness = 0x02,
		
		///Request device accelerometer state (on or off)
		get_accel_state = 0x03,
		
		///Enable or disable device accelerometer
		set_accel_state = 0x04,
		
		///Request device accelerometer values. If accelerometer is off,
		///zeros will be returned
		get_accel_values = 0x05,
		
		///Set number to show on numeric display
		set_number_display_value = 0x06,
		
		//The following values are internal and not supported by protocol
		max_command_value,
		put_color,
		no_command
	};
	
	static constexpr uint8_t ready_sequence = 0x78;
	
	///This structure is sent after command_byte and command_id::set_brightness
	struct set_brightness_packet_data
	{
		uint8_t brightness_value;
	};
	
	///This structure is sent after command_byte and command_id::set_accel_state
	struct set_accel_state_packet_data
	{
		bool enable_accelerometer;
	};
	
	///This structure is sent after command_byte and command_id::set_number_display_value
	struct set_number_display_packet_data
	{
		uint8_t low_number_byte;
		uint8_t middle_number_byte;
		uint8_t high_number_byte;
	};
	
	///command_id::get_brightness response
	struct brightness_response
	{
		uint8_t signature; //command_id::get_brightness
		uint8_t brightness_value;
		uint8_t ready; //ready_sequence
	};
	
	///command_id::get_accel_values response
	struct accel_values_response
	{
		uint8_t signature; //command_id::get_accel_values
		int16_t accel_value[3]; //Little-endian [X; Y; Z] values
		uint8_t ready; //ready_sequence
	};
	
	///command_id::get_accel_state response
	struct accel_state_response
	{
		uint8_t data; //0xE0 | is_accelerometer_enabled
		uint8_t ready; //ready_sequence
	};
	
	///This packet can be sent asynchronously in case
	///state of device button changes
	struct button_state_packet
	{
		//0xF0 | (current_button_id << 1) | pressed, where
		//current_button_id is buttons::button_id
		uint8_t data;
	};
	
public:
	static void run();
};
