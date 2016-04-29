// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <random>
#include <thread>

#include "uart.h"

///Generates effects based on Winamp equalizer data and sends
///effect data to the device via UART
class effect_manager
{
public:
	///Effect types
	enum class effect_processor
	{
		spectrum_analyzer,
		color_waves,
		glowing_dots
	};

	typedef std::function<void()> on_error_callback;

	///Maximum Winamp equalizer value. All values greater
	///than this will be cut to this value.
	static const uint32_t max_eq_value = 15;

public:
	effect_manager();
	~effect_manager();

	void stop();
	void start(uart& com_port);
	
	///Sets callback that is called in case of error
	///(such as device offline error). Effect generation is
	///stopped when error occures.
	void on_error(const on_error_callback& error);

	void set_effect_processor(effect_processor processor);

private:
	std::atomic<bool> running_;
	std::thread worker_;
	uart* com_port_;
	std::atomic<effect_processor> effect_processor_;
	on_error_callback error_callback_;
	std::mt19937 random_gen_;

	void worker();

	void effect_spectrum_analyzer();
	void effect_color_waves();
	void effect_glowing_dots();
};
