// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "space_invaders.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <avr/pgmspace.h>

#include "accelerometer.h"
#include "bitmap.h"
#include "buttons.h"
#include "colors.h"
#include "game.h"
#include "move_helper.h"
#include "number_display.h"
#include "options.h"
#include "timer.h"
#include "util.h"
#include "ws2812_matrix.h"

namespace
{
enum level_type : uint8_t
{
	level_type_usual = 0,
	level_type_boss = 1
};

struct level_info
{
	uint8_t fall_speed : 3;
	uint8_t move_speed : 3;
	uint8_t type : 2;
};

struct alien
{
	uint8_t width : 2; //width - 1
	uint8_t height : 2; //height - 1
	uint8_t lives : 3; //lives - 1
	uint8_t shooting : 1;
	
	uint8_t color_r : 2;
	uint8_t color_g : 2;
	uint8_t color_b : 2; //2 free bits
	
	uint8_t init_x : 4;
	uint8_t init_y : 6; //can be greater than matrix height
};

struct loaded_alien : alien
{
	uint8_t current_lives;
};

constexpr uint8_t boss_frame_count = 2;
constexpr uint8_t max_weak_points = 2;
struct weak_point
{
	uint8_t x : 4;
	uint8_t y : 4;
};

struct boss_level
{
	uint8_t lives;
	uint8_t r_from : 4;
	uint8_t g_from : 4;
	uint8_t b_from : 4;
	uint8_t r_to : 4;
	uint8_t g_to : 4;
	uint8_t b_to : 4;
	uint8_t width : 4;
	uint8_t height : 4;
	const uint8_t* boss_frame[boss_frame_count];
	weak_point weak_points[max_weak_points];
};

const uint8_t boss_1_frame_0[] PROGMEM = {
	0x88, 0xff, 0x1a, 0x3d, 0x68, 0xfc, 0xfc, 0x68, 0x3d, 0x1a
};
const uint8_t boss_1_frame_1[] PROGMEM = {
	0x88, 0xff, 0x19, 0x3a, 0x6d, 0xfa, 0xfa, 0x6d, 0x3a, 0x19
};

const uint8_t boss_2_frame_0[] PROGMEM = {
	0x8a, 0xff, 0x0e, 0x18, 0xbe, 0x6d, 0x3c, 0x3c, 0x6d, 0xbe, 0x18, 0x0e
};
const uint8_t boss_2_frame_1[] PROGMEM = {
	0x8a, 0xff, 0x78, 0x1d, 0xbe, 0x6c, 0x3c, 0x3c, 0x6c, 0xbe, 0x1d, 0x78
};

const uint8_t boss_3_frame_0[] PROGMEM = {
	0x8a, 0xff, 0xe, 0x18, 0xbd, 0x6e, 0x3c, 0x3c, 0x6e, 0xbd, 0x18, 0x0e
};
const uint8_t boss_3_frame_1[] PROGMEM = {
	0x8a, 0xff, 0xe, 0x18, 0x3d, 0x77, 0x3c, 0x3c, 0x77, 0x3d, 0x18, 0x0e
};

const uint8_t boss_4_frame_0[] PROGMEM = {
	0x79, 0xff, 0x1f, 0xdf, 0xdb, 0xfc, 0x77, 0x9f, 0x7d, 0x1f
};
const uint8_t boss_4_frame_1[] PROGMEM = {
	0x79, 0xff, 0x9e, 0x9f, 0xf9, 0xed, 0x3f, 0xbb, 0x7f, 0x1e
};

const boss_level boss_levels[] PROGMEM = {
	{ 55,  0xF, 0xF, 0,  0xF, 0, 0,  10, 8, { boss_2_frame_0, boss_2_frame_1 }, { { 3, 2 }, { 6, 2 } } },
	{ 100,  0xF, 0, 0,  0, 0, 0xF,  10, 8, { boss_3_frame_0, boss_3_frame_1 }, { { 4, 2 }, { 5, 2 } } },
	{ 150,  0xF, 0, 0xA,  0xB, 0xA, 0,  9, 7, { boss_4_frame_0, boss_4_frame_1 }, { { 3, 1 }, { 4, 1 } } },
	{ 200,  0xF, 0xF, 0xF,  0, 0xF, 0x3,  8, 8, { boss_1_frame_0, boss_1_frame_1 }, { { 2, 3 }, { 5, 3 } } }
};

const level_info levels[] PROGMEM = {
	{ 1, 0, level_type_usual },
	{ 1, 1, level_type_usual },
	{ 1, 1, level_type_usual },
	{ 2, 1, level_type_usual },
	{ 2, 1, level_type_usual },
	{ 2, 2, level_type_usual },
	{ 2, 2, level_type_boss },
	{ 2, 2, level_type_usual },
	{ 2, 3, level_type_usual },
	{ 3, 2, level_type_usual },
	{ 3, 3, level_type_usual },
	{ 3, 2, level_type_usual },
	{ 3, 3, level_type_boss },
	{ 3, 4, level_type_usual },
	{ 3, 4, level_type_usual },
	{ 4, 4, level_type_usual },
	{ 4, 4, level_type_usual },
	{ 4, 5, level_type_usual },
	{ 5, 5, level_type_boss },
	{ 5, 5, level_type_usual },
	{ 6, 5, level_type_usual },
	{ 6, 6, level_type_usual },
	{ 6, 6, level_type_boss }
};

constexpr alien level_end { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

constexpr uint8_t h = ws2812_matrix::height;
constexpr uint8_t x_offset = ws2812_matrix::width > 10 ? (ws2812_matrix::width - 10) / 2 : 0;

const alien alien_level_0[] PROGMEM = {
	{ 1, 0, 0, 0,  3, 0, 0,  6 + x_offset, h - 1 },
	{ 1, 0, 0, 1,  0, 0, 3,  2 + x_offset, h - 1 },
	
	{ 1, 0, 0, 0,  0, 3, 0,  4 + x_offset, h - 3 },
	level_end
};

const alien alien_level_1[] PROGMEM = {
	{ 1, 0, 0, 1,  1, 1, 0,  7 + x_offset, h - 1 },
	{ 1, 0, 0, 0,  1, 1, 0,  1 + x_offset, h - 1 },
	{ 3, 0, 0, 1,  1, 2, 0,  3 + x_offset, h - 1 },
	
	{ 1, 0, 1, 0,  0, 1, 1,  2 + x_offset, h - 2 },
	{ 1, 0, 2, 1,  0, 2, 1,  4 + x_offset, h - 2 },
	{ 1, 0, 1, 0,  0, 1, 1,  6 + x_offset, h - 2 },
	level_end
};

const alien alien_level_2[] PROGMEM = {
	{ 2, 0, 0, 1,  3, 0, 0,  1 + x_offset, h - 1 },
	{ 2, 0, 0, 1,  3, 0, 0,  6 + x_offset, h - 1 },
	
	{ 1, 0, 2, 1,  0, 1, 3,  2 + x_offset, h - 3 },
	{ 1, 0, 2, 1,  0, 1, 3,  6 + x_offset, h - 3 },
	
	{ 3, 1, 4, 0,  3, 3, 0,  3 + x_offset, h - 6 },
	level_end
};

const alien alien_level_3[] PROGMEM = {
	{ 0, 0, 3, 1,  0, 0, 3,  3 + x_offset, h - 1 },
	{ 0, 0, 3, 1,  0, 0, 3,  5 + x_offset, h - 1 },
	{ 0, 0, 3, 1,  0, 0, 3,  7 + x_offset, h - 1 },
	
	{ 0, 0, 2, 1,  0, 1, 2,  3 + x_offset, h - 3 },
	{ 0, 0, 2, 1,  0, 1, 2,  5 + x_offset, h - 3 },
	{ 0, 0, 2, 1,  0, 1, 2,  7 + x_offset, h - 3 },
	
	{ 0, 0, 1, 1,  0, 2, 1,  3 + x_offset, h - 5 },
	{ 0, 0, 1, 1,  0, 2, 1,  5 + x_offset, h - 5 },
	{ 0, 0, 1, 1,  0, 2, 1,  7 + x_offset, h - 5 },
	
	{ 2, 0, 1, 0,  0, 3, 0,  4 + x_offset, h - 7 },
	level_end
};

const alien alien_level_4[] PROGMEM = {
	{ 1, 0, 3, 1,  3, 0, 1,  1 + x_offset, h - 1 },
	{ 1, 0, 3, 1,  3, 0, 1,  7 + x_offset, h - 1 },
	
	{ 0, 0, 2, 1,  3, 0, 0,  3 + x_offset, h - 2 },
	{ 0, 0, 2, 1,  3, 0, 0,  6 + x_offset, h - 2 },
	
	{ 1, 1, 7, 1,  3, 3, 3,  4 + x_offset, h - 4 },
	
	{ 0, 0, 2, 0,  3, 0, 0,  3 + x_offset, h - 5 },
	{ 0, 0, 2, 0,  3, 0, 0,  6 + x_offset, h - 5 },
	
	{ 1, 0, 3, 0,  3, 0, 1,  1 + x_offset, h - 6 },
	{ 1, 0, 3, 0,  3, 0, 1,  7 + x_offset, h - 6 },
	level_end
};

const alien alien_level_5[] PROGMEM = {
	{ 0, 2, 3, 1,  3, 0, 0,  6 + x_offset, h - 3 },
	{ 0, 2, 7, 0,  3, 0, 0,  2 + x_offset, h - 5 },
	
	{ 0, 1, 3, 1,  3, 0, 0,  4 + x_offset, h - 2 },
	{ 0, 1, 7, 0,  3, 0, 0,  4 + x_offset, h - 5 },
	
	{ 1, 0, 3, 1,  3, 0, 0,  2 + x_offset, h - 1 },
	{ 1, 0, 7, 0,  3, 0, 0,  5 + x_offset, h - 5 },
	
	{ 2, 0, 7, 1,  3, 0, 0,  3 + x_offset, h - 3 },
	level_end
};

const alien alien_level_7[] PROGMEM = {
	{ 2, 0, 5, 1,  3, 3, 0,  1 + x_offset, h + 1 },
	{ 2, 0, 5, 1,  3, 3, 0,  6 + x_offset, h + 1 },
	
	{ 3, 0, 3, 0,  1, 1, 3,  3 + x_offset, h - 1 },
	{ 3, 0, 5, 1,  1, 1, 3,  3 + x_offset, h - 4 },
	
	{ 0, 1, 4, 0,  1, 1, 3,  3 + x_offset, h - 3 },
	{ 0, 1, 4, 0,  1, 1, 3,  6 + x_offset, h - 3 },
	
	{ 0, 0, 2, 1,  3, 0, 0,  5 + x_offset, h - 2 },
	{ 0, 0, 2, 1,  3, 0, 0,  4 + x_offset, h - 3 },
	level_end
};

const alien alien_level_8[] PROGMEM = {
	{ 2, 0, 5, 1,  0, 3, 3,  1 + x_offset, h + 2 },
	{ 2, 0, 5, 1,  0, 3, 3,  6 + x_offset, h + 2 },
	
	{ 2, 0, 4, 1,  0, 2, 3,  2 + x_offset, h },
	{ 2, 0, 4, 1,  0, 2, 3,  5 + x_offset, h },
	
	{ 3, 0, 3, 1,  0, 1, 3,  3 + x_offset, h - 2 },
	
	{ 1, 0, 2, 0,  0, 0, 3,  4 + x_offset, h - 4 },
	
	{ 0, 0, 1, 1,  0, 0, 2,  4 + x_offset, h - 6 },
	{ 0, 0, 1, 1,  0, 0, 2,  5 + x_offset, h - 6 },
	level_end
};

const alien alien_level_9[] PROGMEM = {
	{ 1, 0, 4, 1,  3, 1, 0,  2 + x_offset, h - 1 },
	{ 1, 0, 4, 1,  3, 1, 0,  4 + x_offset, h - 1 },
	{ 1, 0, 4, 1,  3, 1, 0,  6 + x_offset, h - 1 },
	{ 1, 0, 4, 1,  3, 1, 0,  8 + x_offset, h - 1 },
	
	{ 1, 0, 3, 1,  3, 2, 0,  1 + x_offset, h - 3 },
	{ 1, 0, 3, 1,  3, 2, 0,  3 + x_offset, h - 3 },
	{ 1, 0, 3, 1,  3, 2, 0,  5 + x_offset, h - 3 },
	{ 1, 0, 3, 1,  3, 2, 0,  7 + x_offset, h - 3 },
	
	{ 1, 0, 2, 1,  3, 3, 0,  2 + x_offset, h - 5 },
	{ 1, 0, 2, 1,  3, 3, 0,  4 + x_offset, h - 5 },
	{ 1, 0, 2, 1,  3, 3, 0,  6 + x_offset, h - 5 },
	{ 1, 0, 2, 1,  3, 3, 0,  8 + x_offset, h - 5 },
	
	{ 1, 0, 1, 1,  1, 3, 0,  1 + x_offset, h - 7 },
	{ 1, 0, 1, 1,  1, 3, 0,  3 + x_offset, h - 7 },
	{ 1, 0, 1, 1,  1, 3, 0,  5 + x_offset, h - 7 },
	{ 1, 0, 1, 1,  1, 3, 0,  7 + x_offset, h - 7 }
};

const alien alien_level_10[] PROGMEM = {
	{ 0, 2, 2, 0,  0, 0, 1,  5 + x_offset, h - 3 },
	
	{ 0, 2, 4, 1,  0, 0, 2,  6 + x_offset, h - 6 },
	{ 0, 3, 3, 0,  0, 0, 3,  5 + x_offset, h - 7 },
	{ 0, 2, 4, 1,  0, 0, 2,  4 + x_offset, h - 6 },
	
	{ 0, 2, 5, 1,  1, 1, 3,  7 + x_offset, h - 8 },
	{ 0, 2, 5, 1,  1, 2, 3,  3 + x_offset, h - 8 },
	
	{ 0, 2, 7, 1,  2, 2, 3,  4 + x_offset, h - 9 },
	
	{ 1, 0, 7, 1,  3, 3, 3,  5 + x_offset, h - 9 },
	level_end
};

const alien alien_level_11[] PROGMEM = {
	{ 1, 0, 4, 0,  0, 3, 1,  3 + x_offset, h - 1 },
	{ 0, 0, 4, 0,  0, 3, 1,  5 + x_offset, h - 1 },
	{ 1, 0, 4, 0,  0, 3, 1,  6 + x_offset, h - 1 },
	
	{ 2, 0, 4, 0,  0, 3, 1,  4 + x_offset, h - 2 },
	
	{ 3, 0, 4, 0,  0, 3, 1,  1 + x_offset, h - 5 },
	{ 3, 0, 4, 0,  0, 3, 1,  6 + x_offset, h - 5 },
	
	{ 0, 3, 5, 1,  3, 1, 0,  5 + x_offset, h - 6 },
	
	{ 2, 0, 5, 1,  3, 1, 0,  2 + x_offset, h - 6 },
	{ 2, 0, 5, 1,  3, 1, 0,  6 + x_offset, h - 6 },
	
	{ 0, 2, 7, 1,  3, 1, 0,  5 + x_offset, h - 9 },
	level_end
};

const alien alien_level_12[] PROGMEM = {
	{ 2, 0, 3, 0,  1, 0, 3,  2 + x_offset, h - 1 },
	{ 2, 0, 3, 0,  1, 0, 3,  5 + x_offset, h - 1 },
	
	{ 0, 3, 3, 1,  1, 0, 3,  2 + x_offset, h - 5 },
	{ 0, 3, 3, 1,  1, 0, 3,  7 + x_offset, h - 5 },
	
	{ 1, 0, 3, 0,  1, 0, 3,  4 + x_offset, h - 2 },
	
	{ 3, 0, 3, 1,  1, 0, 3,  3 + x_offset, h - 3 },
	
	{ 3, 0, 4, 1,  1, 0, 3,  3 + x_offset, h - 5 },
	
	{ 0, 1, 7, 1,  0, 0, 3,  3 + x_offset, h - 7 },
	{ 0, 1, 7, 1,  0, 0, 3,  6 + x_offset, h - 7 },
	
	{ 0, 0, 7, 1,  0, 0, 3,  4 + x_offset, h - 7 },
	{ 0, 0, 7, 1,  0, 0, 3,  7 + x_offset, h - 7 },
	level_end
};

const alien alien_level_13[] PROGMEM = {
	{ 0, 2, 4, 0,  1, 2, 3,  4 + x_offset, h - 3 },
	{ 0, 2, 5, 1,  1, 2, 3,  8 + x_offset, h - 3 },
	
	{ 2, 0, 4, 0,  1, 2, 3,  5 + x_offset, h - 2 },
	{ 2, 0, 4, 1,  1, 2, 3,  5 + x_offset, h - 4 },
	
	{ 0, 0, 4, 0,  1, 2, 3,  6 + x_offset, h - 3 },
	{ 0, 0, 4, 1,  1, 2, 3,  6 + x_offset, h - 5 },
	
	{ 0, 0, 5, 1,  3, 0, 0,  5 + x_offset, h - 3 },
	{ 0, 0, 5, 1,  3, 0, 0,  7 + x_offset, h - 3 },
	
	{ 3, 1, 6, 1,  1, 2, 3,  4 + x_offset, h - 7 },
	
	{ 0, 0, 6, 1,  1, 2, 3,  8 + x_offset, h - 8 },
	
	{ 0, 0, 6, 1,  1, 2, 3,  3 + x_offset, h - 7 },
	{ 0, 0, 6, 1,  1, 2, 3,  2 + x_offset, h - 8 },
	
	{ 3, 0, 6, 1,  1, 2, 3,  3 + x_offset, h - 8 },
	
	{ 0, 3, 6, 1,  1, 2, 3,  1 + x_offset, h - 7 },
	level_end
};

const alien alien_level_14[] PROGMEM = {
	{ 2, 2, 6, 1,  0, 3, 1,  0 + x_offset, h - 3 },
	{ 2, 2, 6, 1,  0, 3, 1,  7 + x_offset, h - 3 },
	
	{ 1, 2, 6, 1,  3, 3, 3,  0 + x_offset, h - 7 },
	{ 1, 2, 6, 1,  3, 3, 3,  8 + x_offset, h - 7 },
	
	{ 2, 2, 7, 1,  1, 3, 0,  1 + x_offset, h - 11 },
	{ 2, 2, 7, 1,  1, 3, 0,  6 + x_offset, h - 11 },
	
	{ 1, 2, 6, 1,  0, 3, 2,  4 + x_offset, h - 3 },
	
	{ 3, 2, 6, 1,  3, 3, 3,  3 + x_offset, h - 7 },
	level_end
};

const alien alien_level_15[] PROGMEM = {
	{ 1, 2, 4, 1,  3, 0, 0,  3 + x_offset, h - 3 },
	{ 1, 2, 4, 1,  3, 0, 0,  6 + x_offset, h - 3 },
	
	{ 0, 0, 4, 1,  3, 0, 0,  5 + x_offset, h - 1 },
	{ 0, 0, 4, 1,  3, 0, 0,  5 + x_offset, h - 3 },
	
	{ 1, 0, 4, 1,  3, 0, 0,  2 + x_offset, h - 5 },
	{ 1, 0, 4, 1,  3, 0, 0,  7 + x_offset, h - 5 },
	
	{ 0, 0, 5, 1,  3, 3, 0,  2 + x_offset, h - 6 },
	{ 0, 0, 5, 1,  3, 3, 0,  8 + x_offset, h - 6 },
	
	{ 0, 2, 5, 1,  3, 0, 0,  4 + x_offset, h - 8 },
	{ 0, 2, 5, 1,  3, 0, 0,  6 + x_offset, h - 8 },
	
	{ 0, 0, 5, 1,  3, 0, 0,  3 + x_offset, h - 8 },
	{ 0, 0, 5, 1,  3, 0, 0,  7 + x_offset, h - 8 },
	
	{ 2, 1, 5, 1,  3, 0, 0,  4 + x_offset, h - 5 },
	
	{ 0, 0, 7, 0,  3, 3, 3,  5 + x_offset, h - 1 },
	level_end
};

const alien alien_level_16[] PROGMEM = {
	{ 1, 1, 6, 1,  3, 0, 0,  7 + x_offset, h - 2 },
	{ 1, 1, 6, 0,  3, 1, 0,  5 + x_offset, h - 2 },
	{ 1, 1, 6, 1,  3, 2, 0,  3 + x_offset, h - 2 },
	{ 1, 1, 6, 0,  3, 3, 0,  1 + x_offset, h - 2 },
	
	{ 1, 1, 6, 0,  0, 3, 1,  7 + x_offset, h - 4 },
	{ 1, 1, 6, 1,  0, 3, 0,  5 + x_offset, h - 4 },
	{ 1, 1, 6, 0,  1, 3, 0,  3 + x_offset, h - 4 },
	{ 1, 1, 6, 1,  2, 3, 0,  1 + x_offset, h - 4 },
	
	{ 1, 1, 6, 1,  0, 3, 2,  7 + x_offset, h - 6 },
	{ 1, 1, 6, 0,  0, 3, 3,  5 + x_offset, h - 6 },
	{ 1, 1, 6, 1,  0, 2, 3,  3 + x_offset, h - 6 },
	{ 1, 1, 6, 0,  0, 1, 3,  1 + x_offset, h - 6 },
	level_end
};

const alien alien_level_17[] PROGMEM = {
	{ 1, 0, 7, 1,  2, 3, 2,  7 + x_offset, h - 2 },
	{ 1, 0, 7, 1,  2, 3, 2,  4 + x_offset, h - 2 },
	{ 1, 0, 7, 1,  2, 3, 2,  1 + x_offset, h - 2 },
	
	{ 0, 1, 5, 0,  3, 0, 0,  2 + x_offset, h - 5 },
	{ 1, 1, 5, 1,  0, 0, 3,  4 + x_offset, h - 5 },
	{ 0, 1, 5, 0,  3, 0, 0,  7 + x_offset, h - 5 },
	
	{ 1, 0, 7, 1,  2, 2, 3,  7 + x_offset, h - 7 },
	{ 1, 0, 7, 1,  2, 2, 3,  4 + x_offset, h - 7 },
	{ 1, 0, 7, 1,  2, 2, 3,  1 + x_offset, h - 7 },
	level_end
};

const alien alien_level_18[] PROGMEM = {
	{ 3, 2, 6, 1,  3, 3, 3,  4 + x_offset, h - 3 },
	{ 0, 2, 6, 1,  3, 3, 3,  3 + x_offset, h - 3 },
	
	{ 0, 1, 6, 1,  3, 3, 3,  2 + x_offset, h - 3 },
	{ 0, 1, 6, 1,  3, 3, 3,  8 + x_offset, h - 3 },
	
	{ 0, 2, 6, 1,  3, 3, 3,  1 + x_offset, h - 5 },
	{ 0, 2, 6, 1,  3, 3, 3,  9 + x_offset, h - 5 },
	
	{ 0, 3, 6, 1,  3, 3, 3,  5 + x_offset, h - 7 },
	
	{ 3, 2, 7, 0,  3, 3, 3,  1 + x_offset, h - 8 },
	{ 3, 2, 7, 0,  3, 3, 3,  6 + x_offset, h - 8 },
	
	{ 0, 1, 7, 0,  3, 3, 3,  5 + x_offset, h - 9 },
	
	{ 2, 0, 7, 0,  3, 3, 3,  2 + x_offset, h - 9 },
	{ 2, 0, 7, 0,  3, 3, 3,  6 + x_offset, h - 9 },
	
	{ 0, 0, 7, 0,  3, 2, 1,  2 + x_offset, h - 10 },
	{ 0, 0, 7, 0,  3, 2, 1,  4 + x_offset, h - 10 },
	{ 0, 0, 7, 0,  3, 2, 1,  6 + x_offset, h - 10 },
	{ 0, 0, 7, 0,  3, 2, 1,  8 + x_offset, h - 10 }
};

const alien alien_level_19[] PROGMEM = {
	{ 3, 3, 7, 0,  3, 3, 0,  1 + x_offset, h - 4 },
	{ 3, 3, 7, 0,  3, 3, 0,  5 + x_offset, h - 4 },
	{ 3, 3, 7, 0,  3, 3, 0,  1 + x_offset, h - 8 },
	{ 3, 3, 7, 0,  3, 3, 0,  5 + x_offset, h - 8 },
	
	{ 1, 1, 7, 1,  3, 0, 0,  2 + x_offset, h - 3 },
	{ 1, 1, 7, 1,  3, 0, 0,  6 + x_offset, h - 3 },
	{ 1, 1, 7, 1,  3, 0, 0,  2 + x_offset, h - 7 },
	{ 1, 1, 7, 1,  3, 0, 0,  6 + x_offset, h - 7 },
	
	{ 0, 0, 7, 1,  0, 0, 3,  3 + x_offset, h - 3 },
	{ 0, 0, 7, 1,  0, 0, 3,  6 + x_offset, h - 3 },
	{ 0, 0, 7, 1,  0, 0, 3,  3 + x_offset, h - 6 },
	{ 0, 0, 7, 1,  0, 0, 3,  6 + x_offset, h - 6 },
	level_end
};

const alien* const level_map[] PROGMEM = {
	alien_level_0,
	alien_level_1,
	alien_level_2,
	alien_level_3,
	alien_level_4,
	alien_level_5,
	nullptr, //boss level 1
	alien_level_7,
	alien_level_8,
	alien_level_9,
	alien_level_10,
	alien_level_11,
	nullptr, //boss level 2
	alien_level_12,
	alien_level_13,
	alien_level_14,
	alien_level_15,
	alien_level_16,
	nullptr, //boss level 3
	alien_level_17,
	alien_level_18,
	alien_level_19,
	nullptr //boss level 4
};

constexpr uint8_t max_aliens = 16;
struct loaded_level
{
	loaded_level()
		:alien_count(0)
	{
	}
	
	level_info info;
	
	//If alien count is 0, then this level is boss level
	uint8_t alien_count;
	union
	{
		loaded_alien aliens[max_aliens];
		
		struct
		{
			boss_level boss_level_info;
			uint8_t boss_frame_number;
			uint8_t boss_max_lives;
			uint8_t boss_x;
			uint8_t boss_y;
		};
	};
};

constexpr uint8_t gun_y = 0;
constexpr uint8_t gun_width = 3;
constexpr uint8_t total_lives = 5;

constexpr uint8_t target_game_counter = 155;
constexpr uint8_t target_bullet_draw_counter = 8;
constexpr uint8_t target_bullet_counter = 54;

constexpr uint8_t max_bullet_count = 8;
constexpr uint8_t target_alien_shoot_counter = 3;
constexpr uint8_t base_alien_shoot_probability = 27;
constexpr uint8_t max_alien_shoot_probability = 128;
constexpr uint8_t boss_additional_shoot_probability = 33;
constexpr uint8_t target_load_next_level_counter = 255;
constexpr uint8_t bullet_color_gradient_step_count = 8;
constexpr uint8_t target_boss_animation_counter = 16;
constexpr uint8_t boss_score_multiplier = 16;
constexpr uint8_t strong_point_score = 4;
constexpr uint8_t weak_point_score = 2;
//These colors end up in RAM (use 12 bytes)
constexpr color::rgb player_bullet_color { 0xff, 0x10, 0 };
constexpr color::rgb player_bullet_color_2 { 0x20, 0xff, 0 };
constexpr color::rgb alien_bullet_color { 0, 0, 0xff };
constexpr color::rgb alien_bullet_color_2 { 0, 0xff, 0x70 };

struct gun_color
{
	uint8_t lives : 4;
	uint8_t r : 4;
	uint8_t g : 4;
	uint8_t b : 4;
};

const gun_color gun_color_map[] PROGMEM = {
	{ 4, 0x0, 0xf, 0 },
	{ 3, 0x8, 0xf, 0 },
	{ 2, 0xd, 0xa, 0 },
	{ 1, 0xf, 0x5, 0 },
	{ 0, 0x9, 0x0, 0 }
};

struct bullet
{
	util::coord coords;
	uint8_t is_valid;
};

struct bullet_info
{
	bullet_info()
		:bullet_counter(0),
		bullet_count(0)
	{
		memset(bullets, 0, sizeof(bullets));
	}
	
	uint8_t bullet_counter;
	uint8_t bullet_count;
	bullet bullets[max_bullet_count];
};

void load_level(uint8_t level_id, loaded_level& level)
{
	memcpy_P(&level.info, &levels[level_id], sizeof(level.info));
	level.info.fall_speed = 0b111 - level.info.fall_speed;
	level.info.move_speed = 0b111 - level.info.move_speed;
	level.alien_count = 0;
	
	if(level.info.type == level_type_boss)
	{
		uint8_t boss_level_id = 0;
		level.boss_frame_number = 0;
		
		{
			//Count current boss level number
			level_info info;
			for(uint8_t i = 0; i != level_id; ++i)
			{
				memcpy_P(&info, &levels[i], sizeof(info));
				if(info.type == level_type_boss)
					++boss_level_id;
			}
		}
		
		memcpy_P(&level.boss_level_info, &boss_levels[boss_level_id],
			sizeof(level.boss_level_info));
		
		level.boss_max_lives = level.boss_level_info.lives;
		level.boss_x = (ws2812_matrix::width - level.boss_level_info.width) / 2;
		level.boss_y = ws2812_matrix::height - level.boss_level_info.height;
	}
	else
	{
		alien* level_ptr;
		memcpy_P(&level_ptr, &level_map[level_id], sizeof(level_ptr));
		
		while(true)
		{
			memcpy_P(&level.aliens[level.alien_count], &level_ptr[level.alien_count], sizeof(alien));
			if(!level.aliens[level.alien_count].color_r
				&& !level.aliens[level.alien_count].color_g
				&& !level.aliens[level.alien_count].color_b)
			{
				break;
			}
			
			level.aliens[level.alien_count].current_lives = level.aliens[level.alien_count].lives + 1;
			if(++level.alien_count == max_aliens)
				break;
		}
	}
}

void get_alien_color(const loaded_alien& obj, uint8_t& r, uint8_t& g, uint8_t& b,
	uint8_t max_brightness)
{
	//Min color value=36
	//If brightness=15 then (36 * 15) / 256 = 2.
	//Max color value=255
	//If brightness=15 then (255 * 15) / 256 = 14.
	r = obj.color_r ? 0xff / (10 - obj.color_r * 3) : 0;
	g = obj.color_g ? 0xff / (10 - obj.color_g * 3) : 0;
	b = obj.color_b ? 0xff / (10 - obj.color_b * 3) : 0;
	
	//If max lives = 8 and current_lives = 1, then min color = 4
	//If brightness = 15 then (4 * 15) / 256 = 0
	uint8_t lives = obj.lives + 1;
	r = (r * obj.current_lives) / lives;
	g = (g * obj.current_lives) / lives;
	b = (b * obj.current_lives) / lives;
	
	uint8_t prev_r = r, prev_g = g, prev_b = b;
	color::scale_to_brightness(r, g, b, max_brightness);
	if(prev_r && !r)
		r = 1;
	if(prev_g && !g)
		g = 1;
	if(prev_b && !b)
		b = 1;
}

void fill_alien(const loaded_alien& obj, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t from_x = obj.init_x;
	uint8_t to_x = from_x + obj.width + 1;
	uint8_t from_y = obj.init_y;
	uint8_t to_y = from_y + obj.height + 1;
	for(uint8_t x = from_x; x != to_x; ++x)
	{
		for(uint8_t y = from_y; y != to_y; ++y)
			ws2812_matrix::set_pixel_color(x, y, r, g, b);
	}
}

constexpr uint8_t animation_step_count = 100;
constexpr uint8_t boss_animation_frame_count = 7;
void display_boss(const loaded_level& level, const color::rgb& rgb, uint16_t delay)
{
	util::delay(delay);
	bitmap::display_bitmap_P(level.boss_level_info.boss_frame[level.boss_frame_number],
		level.boss_x, level.boss_y, rgb.r, rgb.g, rgb.b);
	ws2812_matrix::show();
}

void boss_killed_animation(const loaded_level& level, uint8_t max_brightness)
{
	//First blink
	color::rgb prev_rgb
		{ level.boss_level_info.r_to, level.boss_level_info.g_to, level.boss_level_info.b_to };
	for(uint8_t i = 0; i != 7; ++i)
	{
		display_boss(level, { 0, 0, 0 }, 700);
		display_boss(level, prev_rgb, 700);
	}
	
	//Now some random gradients
	color::rgb next_rgb, rgb;
	for(uint8_t frame = 0; frame != boss_animation_frame_count; ++frame)
	{
		if(frame == boss_animation_frame_count - 1)
			next_rgb = { 0, 0, 0 };
		else
			game::get_random_color(next_rgb, max_brightness);
		
		for(uint8_t animation_step = 0; animation_step != animation_step_count; ++animation_step)
		{
			color::gradient(prev_rgb, next_rgb,
				animation_step_count, animation_step, rgb);
			display_boss(level, rgb, 10);
		}
		
		prev_rgb = next_rgb;
	}
}

void draw_level(const loaded_level& level, uint8_t max_brightness)
{
	color::rgb rgb;
	if(level.alien_count)
	{
		for(uint8_t i = 0; i != level.alien_count; ++i)
		{
			const loaded_alien& obj = level.aliens[i];
			if(!obj.current_lives)
				continue;
			
			get_alien_color(obj, rgb.r, rgb.g, rgb.b, max_brightness);
			fill_alien(obj, rgb.r, rgb.g, rgb.b);
		}
	}
	else if(level.boss_level_info.lives)
	{
		color::rgb from { static_cast<uint8_t>(level.boss_level_info.r_from * 17),
			static_cast<uint8_t>(level.boss_level_info.g_from * 17),
			static_cast<uint8_t>(level.boss_level_info.b_from * 17) };
		color::rgb to { static_cast<uint8_t>(level.boss_level_info.r_to * 17),
			static_cast<uint8_t>(level.boss_level_info.g_to * 17),
			static_cast<uint8_t>(level.boss_level_info.b_to * 17) };
		color::gradient(to, from, level.boss_max_lives,
			level.boss_level_info.lives, rgb);
		color::scale_to_brightness(rgb, max_brightness);
		bitmap::display_bitmap_P(level.boss_level_info.boss_frame[level.boss_frame_number],
			level.boss_x, level.boss_y, rgb.r, rgb.g, rgb.b);
		
		color::scale_to_brightness(to, max_brightness);
		for(uint8_t i = 0; i != max_weak_points; ++i)
		{
			ws2812_matrix::set_pixel_color(level.boss_x + level.boss_level_info.weak_points[i].x,
				level.boss_y + level.boss_level_info.weak_points[i].y, to);
		}
	}
}

void clear_level(const loaded_level& level)
{
	if(level.alien_count)
	{
		for(uint8_t i = 0; i != level.alien_count; ++i)
		{
			const loaded_alien& obj = level.aliens[i];
			if(!obj.current_lives)
				continue;
			
			fill_alien(obj, 0, 0, 0);
		}
	}
	else
	{
		bitmap::display_bitmap_P(level.boss_level_info.boss_frame[level.boss_frame_number],
			level.boss_x, level.boss_y, 0, 0, 0);
	}
}

void draw_gun(int8_t prev_gun_x, int8_t gun_x, uint8_t lives, uint8_t max_brightness)
{
	gun_color elem { 0, 0, 0, 0 };
	for(uint8_t i = 0; i != sizeof(gun_color_map) / sizeof(gun_color_map[0]); ++i)
	{
		memcpy_P(&elem, &gun_color_map[i], sizeof(elem));
		if(lives > elem.lives)
			break;
	}
	
	for(uint8_t i = 0; i != gun_width; ++i)
		ws2812_matrix::clear_pixel(static_cast<uint8_t>(prev_gun_x + i), gun_y);
	
	//Scale colors to 0xff max value
	color::rgb rgb { static_cast<uint8_t>(elem.r * 17),
		static_cast<uint8_t>(elem.g * 17),
	static_cast<uint8_t>(elem.b * 17) };
	color::scale_to_brightness(rgb, max_brightness);
	
	for(uint8_t i = 0; i != gun_width; ++i)
		ws2812_matrix::set_pixel_color(static_cast<uint8_t>(gun_x + i), gun_y, rgb);
}

bool move_down(loaded_level& level)
{
	if(level.alien_count)
	{
		for(uint8_t i = 0; i != level.alien_count; ++i)
		{
			loaded_alien& obj = level.aliens[i];
			if(!obj.current_lives)
				continue;
			
			if(!--obj.init_y)
				return false;
		}
	}
	else if(level.boss_level_info.lives)
	{
		//Move boss down and up
		if(level.boss_y == ws2812_matrix::height - level.boss_level_info.height)
			--level.boss_y;
		else
			++level.boss_y;
	}
	
	return true;
}

void move_left_right(loaded_level& level, bullet_info& alien_bullets, int8_t direction)
{
	if(level.alien_count)
	{
		uint8_t max_bullet_heights[ws2812_matrix::width];
		memset(max_bullet_heights, 0, sizeof(max_bullet_heights));
		for(uint8_t i = 0; i != max_bullet_count; ++i)
		{
			bullet& b = alien_bullets.bullets[i];
			if(b.is_valid)
			{
				if(max_bullet_heights[b.coords.x] < b.coords.y)
					max_bullet_heights[b.coords.x] = b.coords.y;
			}
		}
		
		for(uint8_t i = 0; i != level.alien_count; ++i)
		{
			loaded_alien& obj = level.aliens[i];
			if(!obj.current_lives)
				continue;
			
			//Don't move if there's alien bullet to the left or right
			if(max_bullet_heights[obj.init_x + direction] >= obj.init_y)
				return;
			
			if(direction > 0)
			{
				if(obj.width + 1 + obj.init_x >= ws2812_matrix::width)
					return;
			}
			else
			{
				if(!obj.init_x)
					return;
			}
		}
		
		for(uint8_t i = 0; i != level.alien_count; ++i)
		{
			auto& obj = level.aliens[i];
			obj.init_x += direction;
		}
	}
	else if(level.boss_level_info.lives)
	{
		//As bullets are always created below the boss, we can freely move left or right
		if((direction < 0 && level.boss_x)
			|| (direction > 0 && level.boss_x + level.boss_level_info.width < ws2812_matrix::width))
		{
			level.boss_x += direction;
		}
	}
}

uint8_t find_free_bullet(const bullet_info& info)
{
	for(uint8_t i = 0; i != max_bullet_count; ++i)
	{
		if(!info.bullets[i].is_valid)
			return i;
	}
	
	return 0xff;
}

bool add_bullet(uint8_t x, uint8_t y, bullet_info& info)
{
	if(info.bullet_count >= max_bullet_count)
		return false;
	
	uint8_t free_bullet_index = find_free_bullet(info);
	info.bullets[free_bullet_index].coords.y = y;
	info.bullets[free_bullet_index].coords.x = x;
	info.bullets[free_bullet_index].is_valid = true;
	if(++info.bullet_count >= max_bullet_count)
		return false;
	
	return true;
}

void hide_bullet_point(const bullet& b, uint8_t gun_x)
{
	if(b.coords.y || b.coords.x < gun_x || b.coords.x >= gun_x + gun_width)
		ws2812_matrix::clear_pixel(b.coords);
}

void invalidate_bullet(bullet_info& info, bullet& b, uint8_t gun_x)
{
	b.is_valid = false;
	--info.bullet_count;
	hide_bullet_point(b, gun_x);
}

void move_bullets(bullet_info& info, uint8_t gun_x, uint8_t border_value, int8_t offset)
{
	for(uint8_t i = 0; i != max_bullet_count; ++i)
	{
		auto& b = info.bullets[i];
		if(b.is_valid)
		{
			if(b.coords.y == border_value)
			{
				invalidate_bullet(info, b, gun_x);
			}
			else
			{
				hide_bullet_point(b, gun_x);
				b.coords.y += offset;
			}
		}
	}
}

void draw_bullets_color(const bullet bullets[max_bullet_count],
	const color::rgb& from, const color::rgb& to,
	uint8_t& gradient_step, uint8_t max_brightness)
{
	color::rgb rgb;
	color::gradient(from, to, bullet_color_gradient_step_count, gradient_step, rgb);
	if(++gradient_step == bullet_color_gradient_step_count * 2)
		gradient_step = 0;
	
	color::scale_to_brightness(rgb, max_brightness);
	for(uint8_t i = 0; i != max_bullet_count; ++i)
	{
		const auto& obj = bullets[i];
		if(obj.is_valid)
			ws2812_matrix::set_pixel_color(obj.coords, rgb);
	}
}

void invalidate_alien(loaded_alien& obj)
{
	fill_alien(obj, 0, 0, 0);
}

bool process_bullets(bullet_info& player_bullets, bullet_info& alien_bullets,
	loaded_level& level, uint32_t& score, uint8_t score_multiplier,
	bool& load_next_level, uint8_t gun_x, uint8_t& lives,
	uint8_t max_brightness)
{
	bool score_changed = false;
	
	{
		uint8_t bullet_positions[ws2812_matrix::height][ws2812_matrix::width];
		memset(bullet_positions, 0, sizeof(bullet_positions));
		
		//First check bullet & alien bullet collisions
		for(uint8_t i = 0; i != max_bullet_count; ++i)
		{
			bullet& b = player_bullets.bullets[i];
			if(b.is_valid)
				bullet_positions[b.coords.y][b.coords.x] = i + 1; //Save bullet id + 1
		}
		
		for(uint8_t i = 0; i != max_bullet_count; ++i)
		{
			bullet& b = alien_bullets.bullets[i];
			uint8_t player_bullet_id = bullet_positions[b.coords.y][b.coords.x];
			if(b.is_valid && player_bullet_id)
			{
				//Alien bullet & player bullet collision!
				invalidate_bullet(alien_bullets, b, gun_x);
				invalidate_bullet(player_bullets,
					player_bullets.bullets[player_bullet_id - 1], gun_x);
				score += score_multiplier;
				score_changed = true;
			}
		}
	}
	
	for(uint8_t i = 0; i != max_bullet_count; ++i)
	{
		bullet& b = player_bullets.bullets[i];
		if(!b.is_valid)
			continue;
		
		if(level.alien_count)
		{
			for(uint8_t a = 0; a != level.alien_count; ++a)
			{
				loaded_alien& obj = level.aliens[a];
				if(obj.current_lives)
				{
					if(b.coords.x >= obj.init_x && b.coords.x < obj.init_x + obj.width + 1
						&& b.coords.y >= obj.init_y && b.coords.y < obj.init_y + obj.height + 1)
					{
						//Player bullet + alien collision
						invalidate_bullet(player_bullets, b, gun_x);
						if(!--obj.current_lives)
						{
							score += (obj.lives + 1) * score_multiplier;
							score_changed = true;
							invalidate_alien(obj);
						}
						
						break;
					}
				}
			}
		}
		else if(level.boss_level_info.lives)
		{
			//Boss level is already drawn at this point, we can use led matrix data to check collisions
			//Bullets are not drawn at this point
			if(ws2812_matrix::is_on(b.coords))
			{
				invalidate_bullet(player_bullets, b, gun_x);
				
				if(rand() % 8 == 0)
				{
					score += strong_point_score;
					score_changed = true;
					--level.boss_level_info.lives;
				}
				else
				{
					for(uint8_t i = 0; i != max_weak_points; ++i)
					{
						uint8_t weak_x = level.boss_x + level.boss_level_info.weak_points[i].x;
						uint8_t weak_y = level.boss_y + level.boss_level_info.weak_points[i].y;
						if(b.coords.x == weak_x && b.coords.y == weak_y)
						{
							score += weak_point_score;
							score_changed = true;
							--level.boss_level_info.lives;
							break;
						}
					}
				}
				
				if(!level.boss_level_info.lives)
				{
					boss_killed_animation(level, max_brightness);
					score += score_multiplier * boss_score_multiplier;
					score_changed = true;
					load_next_level = true;
					break;
				}
			}
		}
	}
	
	for(uint8_t i = 0; i != max_bullet_count; ++i)
	{
		bullet& b = alien_bullets.bullets[i];
		if(!b.is_valid)
			continue;
		
		//Alien bullet + gun collision
		if(b.coords.y == gun_y && b.coords.x >= gun_x && b.coords.x < gun_x + gun_width)
		{
			invalidate_bullet(alien_bullets, b, gun_x);
			if(!--lives)
				return score_changed;
		}
	}
	
	if(!load_next_level)
	{
		if(level.alien_count)
		{
			load_next_level = true;
			for(uint8_t a = 0; a != level.alien_count; ++a)
			{
				if(level.aliens[a].current_lives)
				{
					load_next_level = false;
					break;
				}
			}
		}
	}
	
	return score_changed;
}

void add_boss_bullets(bullet_info& alien_bullets, loaded_level& level, uint8_t shoot_probability)
{
	//Boss bullets should be created below the boss
	shoot_probability += boss_additional_shoot_probability;
	for(uint8_t x = level.boss_x; x != level.boss_x + level.boss_level_info.width; ++x)
	{
		if(ws2812_matrix::is_on(x, level.boss_y)
			&& rand() % max_alien_shoot_probability < shoot_probability)
		{
			if(!add_bullet(x, level.boss_y, alien_bullets))
				return;
		}
	}
}

//shoot_probability = 0 to 128 (max_alien_shoot_probability)
void add_alien_bullets(bullet_info& alien_bullets, loaded_level& level, uint8_t shoot_probability)
{
	if(alien_bullets.bullet_count >= max_bullet_count)
		return;
	
	if(!level.alien_count)
	{
		add_boss_bullets(alien_bullets, level, shoot_probability);
		return;
	}
	
	uint8_t max_shoot_heights[ws2812_matrix::width];
	memset(max_shoot_heights, ws2812_matrix::height - 1, sizeof(max_shoot_heights));
	for(uint8_t i = 0; i != level.alien_count; ++i)
	{
		loaded_alien& obj = level.aliens[i];
		if(obj.current_lives)
		{
			for(uint8_t x = obj.init_x; x != obj.init_x + obj.width + 1; ++x)
			{
				if(max_shoot_heights[x] > obj.init_y)
					max_shoot_heights[x] = obj.init_y;
			}
		}
	}
	
	for(uint8_t i = 0; i != level.alien_count; ++i)
	{
		loaded_alien& obj = level.aliens[i];
		if(obj.current_lives && obj.shooting)
		{
			for(uint8_t x = obj.init_x; x != obj.init_x + obj.width + 1; ++x)
			{
				if(obj.init_y > max_shoot_heights[x])
					continue;
				
				if(rand() % max_alien_shoot_probability < shoot_probability)
				{
					if(!add_bullet(x, obj.init_y - 1, alien_bullets))
						return;
				}
			}
		}
	}
}

uint32_t loop()
{
	uint32_t score = 0;
	uint8_t level_id = 0;
	bool hard_mode = false;
	uint8_t max_brightness = options::get_max_brightness();
	bool accelerometer_enabled = options::is_accelerometer_enabled();
	accelerometer::speed_state speed_state(7);
	int8_t gun_x = (ws2812_matrix::width - gun_width) / 2;
	uint8_t game_counter = 0;
	uint8_t lives = total_lives;
	bool need_redraw = false;
	bullet_info bullets;
	bullet_info alien_bullets;
	uint8_t bullet_draw_counter = 0;
	bool need_process_bullets = true;
	bool aliens_moved_down_or_shooted = false;
	bool load_next_level = true;
	bool boss_animation = false;
	uint8_t load_next_level_counter = 1;
	uint8_t boss_animation_counter = 0;
	
	loaded_level level;
	draw_gun(gun_x, gun_x, lives, max_brightness);
	uint8_t fall_counter = 0, move_counter = 0, shoot_counter = 0;
	uint8_t gradient_player_bullet_step = 0, gradient_alien_bullet_step = 0;
		
	while(true)
	{
		timer::wait_for_interrupt();
		
		if(load_next_level && !--load_next_level_counter)
		{
			if(level_id == sizeof(levels) / sizeof(levels[0]))
			{
				hard_mode = true;
				level_id = 0;
			}
			
			load_next_level = false;
			boss_animation = false;
			game_counter = 0;
			
			load_level(level_id, level);
			if(hard_mode)
			{
				level.info.fall_speed = 1;
				level.info.move_speed = 1;
			}
			
			draw_level(level, max_brightness);
			need_process_bullets = true;
			fall_counter = static_cast<uint8_t>(rand()) % level.info.fall_speed;
			move_counter = static_cast<uint8_t>(rand()) % level.info.move_speed;
			shoot_counter = static_cast<uint8_t>(rand()) % target_alien_shoot_counter;
			++level_id;
		}
		
		if(++game_counter == target_game_counter)
		{
			game_counter = 0;
			
			if(++shoot_counter == target_alien_shoot_counter)
			{
				shoot_counter = 0;
				need_process_bullets = true;
				add_alien_bullets(alien_bullets, level, base_alien_shoot_probability + level_id * 2);
				aliens_moved_down_or_shooted = true;
			}
			else //Don't shoot and fall at the same time
			{
				if(fall_counter == level.info.fall_speed)
				{
					fall_counter = 0;
					
					clear_level(level);
					if(!move_down(level))
						break; //Game over
					
					need_process_bullets = true;
					aliens_moved_down_or_shooted = true;
				}
				else
				{
					++fall_counter;
				}
			}
			
			if(move_counter == level.info.move_speed)
			{
				move_counter = 0;
				clear_level(level);
				move_left_right(level, alien_bullets, rand() % 2 ? 1 : -1);
				need_process_bullets = true;
			}
			else
			{
				++move_counter;
			}
		}
		
		switch(move_helper::process_speed(&speed_state, nullptr, accelerometer_enabled))
		{
			case move_direction_left:
				if(gun_x < ws2812_matrix::width - gun_width + 1)
				{
					draw_gun(gun_x, gun_x + 1, lives, max_brightness);
					++gun_x;
					need_redraw = true;
				}
				break;
			
			case move_direction_right:
				if(gun_x > 2 - gun_width)
				{
					draw_gun(gun_x, gun_x - 1, lives, max_brightness);
					--gun_x;
					need_redraw = true;
				}
				break;
			
			default:
				break;
		}
		
		auto button_up_state = buttons::get_button_status(buttons::button_up);
		auto button_down_state = buttons::get_button_status(buttons::button_down);
		if(button_up_state == buttons::button_status_pressed) //Shoot
		{
			if(bullets.bullet_count < max_bullet_count && !bullets.bullet_counter)
			{
				if(score)
				{
					--score;
					number_display::output_number(score);
				}
				
				add_bullet(gun_x + (gun_width / 2), gun_y + 1, bullets);
				bullets.bullet_counter = target_bullet_counter;
				need_process_bullets = true;
			}
		}
		else if(button_up_state == buttons::button_status_still_pressed
			&& button_down_state == buttons::button_status_still_pressed)
		{
			if(!game::pause_with_screen_backup())
			{
				number_display::output_number(score);
				return score;
			}
			
			number_display::output_number(score);
		}
		
		if(bullets.bullet_counter)
			--bullets.bullet_counter;
		
		//Don't move bullets up if aliens moved down in this iteration
		if(aliens_moved_down_or_shooted)
		{
			aliens_moved_down_or_shooted = false;
		}
		else
		{
			if(bullet_draw_counter == target_bullet_draw_counter)
			{
				bullet_draw_counter = 0;
				move_bullets(bullets, gun_x, ws2812_matrix::height - 1, 1);
				need_process_bullets = true;
				if(!level.alien_count)
					boss_animation = true;
			}
			else if(bullet_draw_counter == target_bullet_draw_counter - 1)
			{
				++bullet_draw_counter;
				move_bullets(alien_bullets, gun_x, 0, -1);
				need_process_bullets = true;
			}
			else
			{
				++bullet_draw_counter;
			}
		}
		
		if(need_process_bullets)
		{
			need_process_bullets = false;
			need_redraw = true;
			
			if(boss_animation && ++boss_animation_counter == target_boss_animation_counter)
			{
				boss_animation_counter = 0;
				boss_animation = false;
				clear_level(level);
				if(++level.boss_frame_number == boss_frame_count)
					level.boss_frame_number = 0;
			}
			
			if(!level.alien_count && level.boss_level_info.lives)
			{
				//Boss mode (boss is alive)
				//Clear bullets on led matrix
				move_bullets(bullets, gun_x, ws2812_matrix::height, 0);
				//Draw boss level if changed, as graphical matrix data is used to detect collisions
				draw_level(level, max_brightness);
			}
			
			//1. Calculates bullets and aliens intersections
			//2. Calculates bullets and alien bullets intersections
			//3. Calculates gun and alien bullets intersections
			uint8_t old_lives = lives;
			if(process_bullets(bullets, alien_bullets, level, score, level_id,
				load_next_level, gun_x, lives, max_brightness))
			{
				number_display::output_number(score);
			}
			
			if(!lives)
				break; //Game over
			
			if(load_next_level && !load_next_level_counter)
			{
				boss_animation = false;
				clear_level(level);
				load_next_level_counter = target_load_next_level_counter;
			}
			
			//Draw level and bullets after processing, first bullets
			draw_bullets_color(bullets.bullets, player_bullet_color, player_bullet_color_2,
				gradient_player_bullet_step, max_brightness);
			draw_bullets_color(alien_bullets.bullets, alien_bullet_color, alien_bullet_color_2,
				gradient_alien_bullet_step, max_brightness);
			draw_level(level, max_brightness);
			
			if(old_lives != lives)
				draw_gun(gun_x, gun_x, lives, max_brightness);
		}
		
		if(need_redraw)
		{
			need_redraw = false;
			ws2812_matrix::show();
		}
	}
	
	return score;
}
} //namespace

void space_invaders::run()
{
	game::intro();
	buttons::enable_repeat(buttons::mask_right | buttons::mask_left | buttons::mask_up, true);
	
	uint32_t score = loop();
	
	buttons::enable_repeat(buttons::mask_right | buttons::mask_left | buttons::mask_up, false);
	game::end(score, game::game_space_invaders);
}
