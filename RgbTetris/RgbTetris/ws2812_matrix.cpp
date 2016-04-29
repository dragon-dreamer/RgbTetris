// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "ws2812_matrix.h"

#include <string.h>

#include <avr/io.h>

namespace
{
//Configuration
#define MATRIX_PORT PORTA
#define MATRIX_DDR DDRA
#define MATRIX_PIN PA3
#define MATRIX_PINMASK _BV(MATRIX_PIN)

uint8_t pixels_[ws2812_matrix::height][ws2812_matrix::width][ws2812_matrix::bytes_per_led];
} //namespace

void ws2812_matrix::init()
{
	MATRIX_PORT &= ~MATRIX_PINMASK;
	MATRIX_DDR |= MATRIX_PINMASK;
	
	//Timer 1 prescaler = 8, frequency = 2 MHz, 1 tick in 0.5us
	//50us = 100 ticks
	OCR1A = 99; //99 + 1 tick to set OCF1A
	TCCR1B = _BV(CS11) | _BV(WGM12); //Set prescaler 8 for timer 1, CTC mode
	TCNT1 = 98;
}

void ws2812_matrix::set_pixel_color_fast(const util::coord& coords, const color::rgb& rgb)
{
	uint8_t* p = pixels_[coords.y][coords.x];
	p[r_offset] = rgb.r;
	p[g_offset] = rgb.g;
	p[b_offset] = rgb.b;
}

void ws2812_matrix::set_pixel_color_fast(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t* p = pixels_[y][x];
	p[r_offset] = r;
	p[g_offset] = g;
	p[b_offset] = b;
}

void ws2812_matrix::get_pixel_color_fast(uint8_t x, uint8_t y, uint8_t& r, uint8_t& g, uint8_t& b)
{
	const uint8_t* p = pixels_[y][x];
	r = p[r_offset];
	g = p[g_offset];
	b = p[b_offset];
}

void ws2812_matrix::copy_color(uint8_t from_x, uint8_t from_y, uint8_t to_x, uint8_t to_y)
{
	if(to_x >= width || to_y >= height)
		return;
	
	uint8_t* p_to = pixels_[to_y][to_x];
	if(from_x >= width || from_y >= height)
	{
		p_to[r_offset] = 0;
		p_to[g_offset] = 0;
		p_to[b_offset] = 0;
		return;
	}
	
	const uint8_t* p_from = pixels_[from_y][from_x];
	p_to[r_offset] = p_from[r_offset];
	p_to[g_offset] = p_from[g_offset];
	p_to[b_offset] = p_from[b_offset];
}

void ws2812_matrix::set_pixel_color_fast(uint8_t x, uint8_t y, uint32_t color)
{
	uint8_t r = static_cast<uint8_t>(color >> 16);
	uint8_t g = static_cast<uint8_t>(color >> 8);
	uint8_t b = static_cast<uint8_t>(color);
	
	uint8_t* p = pixels_[y][x];
	p[r_offset] = r;
	p[g_offset] = g;
	p[b_offset] = b;
}

uint32_t ws2812_matrix::get_pixel_color_fast(uint8_t x, uint8_t y)
{
	const uint8_t *p = pixels_[y][x];
	return (static_cast<uint32_t>(p[r_offset]) << 16) |
		(static_cast<uint32_t>(p[g_offset]) << 8) |
		static_cast<uint32_t>(p[b_offset]);
}

bool ws2812_matrix::is_on(uint8_t x, uint8_t y)
{
	if(x >= width || y >= height)
		return false;
	
	return is_on_fast(x, y);
}

bool ws2812_matrix::is_on(const util::coord& coord)
{
	return is_on(coord.x, coord.y);
}

bool ws2812_matrix::is_on_fast(uint8_t x, uint8_t y)
{
	const uint8_t *p = pixels_[y][x];
	return p[r_offset] || p[g_offset] || p[b_offset];
}

void ws2812_matrix::set_pixel_color(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
	if(x >= width || y >= height)
		return;
	
	set_pixel_color_fast(x, y, r, g, b);
}

void ws2812_matrix::get_pixel_color(uint8_t x, uint8_t y, uint8_t& r, uint8_t& g, uint8_t& b)
{
	if(x >= width || y >= height)
		return;
	
	get_pixel_color_fast(x, y, r, g, b);
}

void ws2812_matrix::set_pixel_color(uint8_t x, uint8_t y, uint32_t color)
{
	if(x >= width || y >= height)
		return;
	
	set_pixel_color_fast(x, y, color);
}

void ws2812_matrix::set_pixel_color(uint8_t x, uint8_t y, const color::rgb& rgb)
{
	set_pixel_color(x, y, rgb.r, rgb.g, rgb.b);
}

void ws2812_matrix::set_pixel_color(const util::coord& coord, const color::rgb& rgb)
{
	set_pixel_color(coord.x, coord.y, rgb.r, rgb.g, rgb.b);
}

void ws2812_matrix::clear_pixel(uint8_t x, uint8_t y)
{
	set_pixel_color(x, y, 0, 0, 0);
}

void ws2812_matrix::clear_pixel(const util::coord& coord)
{
	set_pixel_color(coord.x, coord.y, 0, 0, 0);
}

uint32_t ws2812_matrix::get_pixel_color(uint8_t x, uint8_t y)
{
	if(x >= width || y >= height)
		return 0;
	
	return get_pixel_color_fast(x, y);
}

void ws2812_matrix::clear()
{
	memset(pixels_, 0, byte_count);
}

uint8_t* ws2812_matrix::get_pixels()
{
	return (uint8_t*)pixels_;
}

namespace
{
inline void wait_till_leds_ready()
{
	loop_until_bit_is_set(TIFR1, OCF1A);
}

inline void set_draw_end()
{
	TCNT1 = 0; //Clear timer1 counter
	TIFR1 |= _BV(OCF1A); //Clear timer1 counter overflow (write 1 to clear)
}
} //namespace

void ws2812_matrix::show()
{
	wait_till_leds_ready();
	
	//0 code = 5 ticks from high to low  (always) | 0.4us(+-0.15), 0.08/tick   [min=0.05; max=0.11]
	//1 code = 13 ticks from high to low (always) | 0.8us(+-0.15), 0.062/tick  [min=0.05; max=0.073]
	//////////////////////////////////////////////////////////////////////////
	//0 code = 14 ticks from low to high (always) | 0.85us(+-0.15), 0.06/tick  [min=0.05; max=0.0714]
	//1 code = 6 ticks from low to high (always)  | 0.45us(+-0.15), 0.075/tick [min=0.05; max=0.1]
	//Min available time = 0.05us/tick; max available time = 0.0714us/tick
	//Max available frequency = 20 MHz; min available frequency = 14 MHz
	static_assert(F_CPU >= 14500000 && F_CPU <= 19500000,
		"Only 14.5 to 19.5 MHz frequency is supported");
	
	//These are not real variables, they're used to create readable aliases
	//for controller registers in assembler code
	uint8_t color, bit_number;
	asm volatile(
		//Disable interrupts
		"in __tmp_reg__, __SREG__"           "\n\t"
		"cli"                                "\n\t"
		
		"read_byte:"                         "\n\t"
		//Decrement color byte counter
		"sbiw %[byte_count], 1"              "\n\t" //2 ticks
		//Process byte, if we still have any, otherwise exit
		"brmi end"                           "\n\t" //1-2 ticks (2 ticks if jump is performed)
		
		//Set LED matrix control pin to 0
		"cbi %[port], %[pin_number]"         "\n\t" //2 ticks
		//Read next color byte and increment array pointer
		"ld %[color], %a[pixels]+"           "\n\t" //2 ticks
		"ldi %[bit_number], 8"               "\n\t" //1 tick
		"nop"                                "\n\t" //1 tick
		
		"process_byte:"                      "\n\t"
		//Set LED matrix control pin to 1
		"sbi %[port], %[pin_number]"         "\n\t" //2 ticks
		
		//If MSB is not set, set port LED matrix bit to zero
		"sbrs %[color], 7"                   "\n\t" //1-2 ticks (2 ticks if jump is performed)
		"rjmp set_bit_to_zero"               "\n\t" //2 ticks
		//Otherwise, do nothing, but keep tick count the same
		"nop"                                "\n\t" //1 tick
		"rjmp skip_set_port_to_zero"         "\n\t" //2 ticks
		"set_bit_to_zero:"                   "\n\t"
		"cbi %[port], %[pin_number]"         "\n\t" //2 ticks
		"skip_set_port_to_zero:"             "\n\t"
		
		//Next bit...
		"dec %[bit_number]"                  "\n\t" //1 tick
		//Read next byte if no bits left in current one
		"breq read_byte"                     "\n\t" //1-2 ticks (2 ticks if jump is performed)
		
		//Shift bit number left
		"lsl %[color]"                       "\n\t" //1 tick
		"rjmp .+0"                           "\n\t" //2 ticks (does nothing)
		"nop"                                "\n\t" //1 tick
		"cbi %[port], %[pin_number]"         "\n\t" //2 ticks
		"rjmp .+0"                           "\n\t" //2 ticks (does nothing)
		"rjmp process_byte"                  "\n\t" //2 ticks
		
		//Restore SREG (turn interrupts on)
		"end:" "\n\t"
		"cbi %[port], %[pin_number]"        "\n\t"
		"out __SREG__, __tmp_reg__"         "\n\t"
		: [color] "=&r" (color),
		  [bit_number] "=&r" (bit_number)
		: [port] "I" (_SFR_IO_ADDR(MATRIX_PORT)),
		  [pixels]    "e" (pixels_),
		  [byte_count] "w" (byte_count),
		  [pin_number] "M" (MATRIX_PIN)
	);
	
	set_draw_end();
}

void ws2812_matrix::shift_right()
{
	for(uint8_t y = 0; y != height; ++y)
	{
		for(uint8_t x = 0; x != width - 1; ++x)
		{
			pixels_[y][x][r_offset] = pixels_[y][x + 1][r_offset];
			pixels_[y][x][g_offset] = pixels_[y][x + 1][g_offset];
			pixels_[y][x][b_offset] = pixels_[y][x + 1][b_offset];
		}
		
		pixels_[y][width - 1][r_offset] = 0;
		pixels_[y][width - 1][g_offset] = 0;
		pixels_[y][width - 1][b_offset] = 0;
	}
}

void ws2812_matrix::shift_left()
{
	for(uint8_t y = 0; y != height; ++y)
	{
		for(uint8_t x = width - 1; x; --x)
		{
			pixels_[y][x][r_offset] = pixels_[y][x - 1][r_offset];
			pixels_[y][x][g_offset] = pixels_[y][x - 1][g_offset];
			pixels_[y][x][b_offset] = pixels_[y][x - 1][b_offset];
		}
		
		pixels_[y][0][r_offset] = 0;
		pixels_[y][0][g_offset] = 0;
		pixels_[y][0][b_offset] = 0;
	}
}

void ws2812_matrix::shift_up()
{
	for(uint8_t x = 0; x != width; ++x)
	{
		for(uint8_t y = height - 1; y; --y)
		{
			pixels_[y][x][r_offset] = pixels_[y - 1][x][r_offset];
			pixels_[y][x][g_offset] = pixels_[y - 1][x][g_offset];
			pixels_[y][x][b_offset] = pixels_[y - 1][x][b_offset];
		}
		
		pixels_[0][x][r_offset] = 0;
		pixels_[0][x][g_offset] = 0;
		pixels_[0][x][b_offset] = 0;
	}
}

void ws2812_matrix::shift_down()
{
	for(uint8_t x = 0; x != width; ++x)
	{
		for(uint8_t y = 0; y != height - 1; ++y)
		{
			pixels_[y][x][r_offset] = pixels_[y + 1][x][r_offset];
			pixels_[y][x][g_offset] = pixels_[y + 1][x][g_offset];
			pixels_[y][x][b_offset] = pixels_[y + 1][x][b_offset];
		}
		
		pixels_[height - 1][x][r_offset] = 0;
		pixels_[height - 1][x][g_offset] = 0;
		pixels_[height - 1][x][b_offset] = 0;
	}
}

void ws2812_matrix::clear_horizontal_lines(uint8_t y_from, uint8_t y_to)
{
	memset(reinterpret_cast<uint8_t*>(pixels_) + y_from * width * bytes_per_led,
		0, (y_to - y_from + 1) * width * bytes_per_led);
}

void ws2812_matrix::shift_right(uint8_t y_from, uint8_t y_to)
{
	for(uint8_t y = y_from; y != y_to + 1; ++y)
	{
		for(uint8_t x = 0; x != width - 1; ++x)
		{
			pixels_[y][x][r_offset] = pixels_[y][x + 1][r_offset];
			pixels_[y][x][g_offset] = pixels_[y][x + 1][g_offset];
			pixels_[y][x][b_offset] = pixels_[y][x + 1][b_offset];
		}
		
		pixels_[y][width - 1][r_offset] = 0;
		pixels_[y][width - 1][g_offset] = 0;
		pixels_[y][width - 1][b_offset] = 0;
	}
}

void ws2812_matrix::shift_left(uint8_t y_from, uint8_t y_to)
{
	for(uint8_t y = y_from; y != y_to + 1; ++y)
	{
		for(uint8_t x = width - 1; x; --x)
		{
			pixels_[y][x][r_offset] = pixels_[y][x - 1][r_offset];
			pixels_[y][x][g_offset] = pixels_[y][x - 1][g_offset];
			pixels_[y][x][b_offset] = pixels_[y][x - 1][b_offset];
		}
		
		pixels_[y][0][r_offset] = 0;
		pixels_[y][0][g_offset] = 0;
		pixels_[y][0][b_offset] = 0;
	}
}

void ws2812_matrix::shift_up(uint8_t x_from, uint8_t x_to)
{
	for(uint8_t x = x_from; x != x_to + 1; ++x)
	{
		for(uint8_t y = height - 1; y; --y)
		{
			pixels_[y][x][r_offset] = pixels_[y - 1][x][r_offset];
			pixels_[y][x][g_offset] = pixels_[y - 1][x][g_offset];
			pixels_[y][x][b_offset] = pixels_[y - 1][x][b_offset];
		}
		
		pixels_[0][x][r_offset] = 0;
		pixels_[0][x][g_offset] = 0;
		pixels_[0][x][b_offset] = 0;
	}
}

void ws2812_matrix::shift_down(uint8_t x_from, uint8_t x_to)
{
	for(uint8_t x = x_from; x != x_to + 1; ++x)
	{
		for(uint8_t y = 0; y != height - 1; ++y)
		{
			pixels_[y][x][r_offset] = pixels_[y + 1][x][r_offset];
			pixels_[y][x][g_offset] = pixels_[y + 1][x][g_offset];
			pixels_[y][x][b_offset] = pixels_[y + 1][x][b_offset];
		}
		
		pixels_[height - 1][x][r_offset] = 0;
		pixels_[height - 1][x][g_offset] = 0;
		pixels_[height - 1][x][b_offset] = 0;
	}
}