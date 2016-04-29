// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "equalizer.h"

#include <cassert>
#include <stdexcept>
#include <Windows.h>

#include "wa_ipc.h"

equalizer::export_sa_get equalizer::export_sa_get_ = nullptr;
equalizer::export_sa_setreq equalizer::export_sa_setreq_ = nullptr;

void equalizer::init(HWND winamp_window)
{
	export_sa_get_ = reinterpret_cast<export_sa_get>(::SendMessageW(winamp_window, WM_WA_IPC, 2, IPC_GETSADATAFUNC));
	export_sa_setreq_ = reinterpret_cast<export_sa_setreq>(::SendMessageW(winamp_window, WM_WA_IPC, 1, IPC_GETSADATAFUNC));
	if(!export_sa_get_ || !export_sa_setreq_)
		throw std::runtime_error("Unable to initialize equalizer");
}

equalizer::sadata equalizer::get_raw_equalizer_data()
{
	assert(export_sa_setreq_ && export_sa_get_ && "Equalizer is not initialized");
	export_sa_setreq_(0);
	sadata ret;
	export_sa_get_(ret.data());
	return ret;
}

std::vector<uint8_t> equalizer::cut_sa_data(const sadata& data, uint8_t needed_band_count, cut_mode mode)
{
	if(needed_band_count > band_count)
		throw std::invalid_argument("needed_band_count > band_count");

	std::vector<uint8_t> ret;
	ret.reserve(needed_band_count);

	double window_size = band_count / static_cast<double>(needed_band_count);
	double next_index = window_size;

	uint32_t value = 0;
	uint32_t value_count = 0;
	if(mode == cut_mode::single)
		ret.push_back(data[0]);

	for(uint32_t i = 0; i != band_count; ++i)
	{
		value += static_cast<uint8_t>(data[i]);
		++value_count;
		if(i + 1 >= next_index)
		{
			if(mode == cut_mode::average)
				ret.push_back(static_cast<uint8_t>(value / value_count));
			else
				ret.push_back(static_cast<uint8_t>(data[i]));

			value_count = 0;
			value = 0;
			next_index += window_size;
		}
	}

	if(mode == cut_mode::average && value_count)
		ret.push_back(static_cast<uint8_t>(value / value_count));

	return ret;
}

