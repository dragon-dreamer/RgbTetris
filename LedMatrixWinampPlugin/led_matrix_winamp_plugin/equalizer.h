// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <Windows.h>

#include <array>
#include <stdint.h>
#include <vector>

#include "static_class.h"

///Winamp equalizer data wrapper
class equalizer : static_class
{
public:
	static const uint32_t max_band_index = 69;
	static const uint32_t band_count = max_band_index + 1;

	///Size of this array is taken from Winamp plugin samples (Winamp SDK)
	typedef std::array<char, 75 * 2 + 8> sadata;

	///Shrink mode enumeration for cut_sa_data() function
	enum class cut_mode
	{
		///Calculate average value for each band value taken from
		///equalizer data. This means, if you request several values, each value
		///returned will be average of itself and all non-returned previous
		///values from raw equalizer data
		average,
		///Take single values from raw equalizer data
		single
	};

public:
	///Initializes wrapper
	static void init(HWND winamp_window);

	///Returns raw equalizer data.
	///The first 70 lines will be the SA data (every line is a band),
	///and then 3 empty lines, then the rest will be the oscilloscope data.
	static sadata get_raw_equalizer_data();

	/** Shrinks raw equalizer data.
	*   @param data Raw equalizer data
	*   @param band_count How many values to keep
	*   @param mode Shrink mode
	*   @returns Cut equalizer data (requested number of values) */
	static std::vector<uint8_t> cut_sa_data(const sadata& data, uint8_t band_count, cut_mode mode);

private:
	typedef char* (__cdecl *export_sa_get)(char data[std::tuple_size<sadata>::value]);
	typedef void (__cdecl *export_sa_setreq)(int want);
	static export_sa_get export_sa_get_;
	static export_sa_setreq export_sa_setreq_;
};
