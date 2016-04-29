// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "uart.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>

#include "accelerometer.h"
#include "adxl345.h"
#include "buttons.h"
#include "colors.h"
#include "number_display.h"
#include "options.h"
#include "queue.h"
#include "util.h"
#include "ws2812_matrix.h"

//Max data rate of MAX232A is 200 kbps

namespace
{
void init_uart()
{
#	define BAUD_TOL 3
#	define BAUD 115200 //2.1% error on 16 MHz
#	include <util/setbaud.h>

	UBRR1H = UBRRH_VALUE;
	UBRR1L = UBRRL_VALUE;

#		if USE_2X
	UCSR1A |= (1 << U2X1);
#		else
	UCSR1A &= ~(1 << U2X1);
#		endif

#	undef BAUD

	//Default uart mode is 8 bit data, 1 stop bit
	
	//Enable send and receive, also enable data received interrupt
	UCSR1B |= _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1);
}

void stop_uart()
{
	//Disable send and receive, disable data received interrupt
	UCSR1B &= ~(_BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1));
}

struct color_data
{
	uint8_t color[3];
};

//This struct and color index is used to read color bytes from UART
volatile color_data current_color { { 0, 0, 0 } };
volatile uint8_t current_color_index = 0;

//Current command to execute
volatile uart::command_id current_command = uart::command_id::no_command;

//This value is true if command byte has arrived (0xff)
volatile bool special_byte_arrived = false;

//If we're going to execute a command, this is used to count and store
//command argument bytes.
constexpr uint8_t max_command_args = 3;
volatile uint8_t needed_command_args = 0;
volatile uint8_t command_args[max_command_args] = {};

//Interrupt is executed when data received,
//this interrupt allows to separately process color bytes and
//command data.
ISR(USART1_RX_vect)
{
	uint8_t value = UDR1;
	
	//If there's a command with arguments not yet received...
	if(needed_command_args)
	{
		//Store the argument
		--needed_command_args;
		command_args[needed_command_args] = value;
		return;
	}
	
	//If command byte arrived...
	if(value == uart::command_byte)
	{
		//...and we haven't yet received one...
		if(!special_byte_arrived)
		{
			//... then the next byte will determine what to do
			special_byte_arrived = true;
			return;
		}
	}
	
	//If command byte arrived previously, then we have new command
	if(special_byte_arrived)
	{
		special_byte_arrived = false;
		//If received value is equal to command byte (we received two command bytes),
		//then this byte is just color value.
		if(value != uart::command_byte)
		{
			//Otherwise, we have a new command
			//First check if we are executing a command currently
			if(current_command != uart::command_id::no_command)
				return; //If so, we have to drop the byte :(
			
			if(value < static_cast<uint8_t>(uart::command_id::max_command_value))
				current_command = static_cast<uart::command_id>(value);
			else
				return; //Unknown command, ignore it
			
			switch(current_command)
			{
			case uart::command_id::sync_display_coords:
				current_color_index = 0;
				return;
			
			//Some commands require several arguments
			case uart::command_id::set_brightness:
			case uart::command_id::set_accel_state:
				needed_command_args = 1;
				return;
			
			case uart::command_id::set_number_display_value:
				needed_command_args = 3;
				return;
			
			default:
				return;
			}
		}
	}
	
	//If we reached this part of function - we have color byte
	
	if(current_command == uart::command_id::put_color)
		return; //If we're already putting color to display, we have to ignore next color byte.
	
	//Save color byte
	current_color.color[current_color_index] = value;
	if(++current_color_index == sizeof(current_color))
	{
		//If we received three color bytes (RGB), then we need to
		//put them to display matrix
		current_command = uart::command_id::put_color;
		current_color_index = 0;
	}
}

template<typename Packet, typename Queue>
void send_packet(const Packet& packet, Queue& q)
{
	for(uint8_t i = 0; i != sizeof(Packet); ++i)
		q.push_back(reinterpret_cast<const uint8_t*>(&packet)[i]);
}

template<typename Queue>
void try_send_data_to_uart(Queue& q)
{
	uint8_t value_to_send;
	//If we can send data and have something to send
	if(bit_is_set(UCSR1A, UDRE1) && q.pop_front(value_to_send))
		UDR1 = value_to_send;
}

constexpr uint8_t target_check_buttons_counter = 70;
//Main UART processing loop
void process_uart()
{
	uint8_t max_brightness = options::get_max_brightness();
	bool is_accelerometer_enabled = options::is_accelerometer_enabled();
	
	//Current coord and color value for display pixel
	util::coord coord { 0, 0 };
	color::rgb rgb;
	
	//Assume all buttons are not pressed initially
	bool pressed_buttons[buttons::btn_count] = {};
	
	//Counter used to check button states one by one
	//(each in target_check_buttons_counter iterations)
	uint8_t check_buttons_counter = 0;
	uint8_t current_button_id = 0;
	
	//Bytes to transmit queue
	queue<16, uint8_t> tx_queue;
	
	while(true)
	{
		try_send_data_to_uart(tx_queue);
		
		//Command processor
		switch(current_command)
		{
		case uart::command_id::sync_display_coords:
			coord.x = coord.y = 0;
			break;
		
		case uart::command_id::put_color:
			{
				rgb = { current_color.color[0], current_color.color[1], current_color.color[2] };
				//Flush command as soon as possible, as we're processing color data stream
				current_command = uart::command_id::no_command;
				
				color::scale_to_brightness(rgb, max_brightness);
				ws2812_matrix::set_pixel_color_fast(coord, rgb);
				if(++coord.x == ws2812_matrix::width)
				{
					coord.x = 0;
				
					if(++coord.y == ws2812_matrix::height)
					{
						coord.y = 0;
						ws2812_matrix::show();
						//Force ready packet as soon as possible
						while(!tx_queue.push_back(uart::ready_sequence))
							try_send_data_to_uart(tx_queue);
					}
				}
			}
			continue;
		
		case uart::command_id::get_brightness:
			if(tx_queue.free_bytes() < sizeof(uart::brightness_response))
				continue;
			
			send_packet(uart::brightness_response {
				static_cast<uint8_t>(uart::command_id::get_brightness),
				max_brightness, uart::ready_sequence }, tx_queue);
			break;
		
		case uart::command_id::get_accel_values:
			if(tx_queue.free_bytes() < sizeof(uart::accel_values_response))
				continue;
			
			{
				uart::accel_values_response packet = { static_cast<uint8_t>(uart::command_id::get_accel_values),
					{}, uart::ready_sequence };
				if(is_accelerometer_enabled)
				{
					accelerometer::get_values(packet.accel_value[0],
						packet.accel_value[1], packet.accel_value[2]);
				}
				
				send_packet(packet, tx_queue);
			}
			break;
		
		case uart::command_id::set_accel_state:
			if(needed_command_args || !tx_queue.free_bytes())
				continue;
			
			{
				bool new_state = static_cast<bool>(command_args[0]);
				if(new_state != is_accelerometer_enabled)
				{
					is_accelerometer_enabled = new_state;
					adxl345::enable_measurements(is_accelerometer_enabled);
				}
			}
			
			tx_queue.push_back(uart::ready_sequence);
			break;
			
		case uart::command_id::get_accel_state:
			if(tx_queue.free_bytes() < sizeof(uart::accel_state_response))
				continue;
			
			send_packet(uart::accel_state_response { static_cast<uint8_t>(0xE0 | is_accelerometer_enabled),
				uart::ready_sequence }, tx_queue);
			break;
		
		case uart::command_id::set_number_display_value:
			if(needed_command_args || !tx_queue.free_bytes())
				continue;
			
			{
				uint32_t value = 0;
				value = command_args[2];
				value |= static_cast<uint16_t>(command_args[1]) << 8;
				value |= static_cast<uint32_t>(command_args[0] & 0x01) << 16;
				number_display::output_number(value);
			}
			
			tx_queue.push_back(uart::ready_sequence);
			break;
		
		case uart::command_id::set_brightness:
			if(needed_command_args || !tx_queue.free_bytes())
				continue;
			
			{
				uint8_t value = command_args[0];
				if(value < options::min_available_brightness)
					value = options::min_available_brightness;
				else if(value > options::max_available_brightness)
					value = options::max_available_brightness;
			
				max_brightness = value;
			}
			
			tx_queue.push_back(uart::ready_sequence);
			break;
		
		default:
			if(++check_buttons_counter == target_check_buttons_counter)
			{
				check_buttons_counter = 0;
				bool pressed = buttons::get_button_status(static_cast<buttons::button_id>(current_button_id))
					!= buttons::button_status_not_pressed;
				if(pressed_buttons[current_button_id] != pressed)
				{
					if(tx_queue.push_back(static_cast<uint8_t>(0xF0 | (current_button_id << 1) | pressed)))
						pressed_buttons[current_button_id] = pressed;
					else
						continue; //Skip further checks if tx_queue is full
				}
				
				if(++current_button_id == buttons::btn_count)
					current_button_id = 0;
				
				if(pressed_buttons[buttons::button_up] && pressed_buttons[buttons::button_down])
				{
					options::set_max_brightness(max_brightness);
					options::set_accelerometer_enabled(is_accelerometer_enabled);
					return;
				}
			}
			continue;
		}
		
		current_command = uart::command_id::no_command;
	}
}

void reset()
{
	ws2812_matrix::clear();
	buttons::flush_pressed();
}
} //namespace

void uart::run()
{
	init_uart();
	
	reset();
	process_uart();
	
	stop_uart();
	reset();
}
