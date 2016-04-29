// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "effect_manager.h"

#include <algorithm>
#include <ctime>
#include <functional>
#include <vector>

#include "colors.h"
#include "display.h"
#include "display_protocol.h"
#include "equalizer.h"

effect_manager::effect_manager()
	: running_(false)
	, com_port_(nullptr)
	, effect_processor_(effect_processor::spectrum_analyzer)
{
	random_gen_.seed(static_cast<std::mt19937::result_type>(std::time(0)));
}

effect_manager::~effect_manager()
{
	stop();
}

void effect_manager::start(uart& com_port)
{
	if(worker_.joinable())
		worker_.join();

	bool expected = false;
	if(running_.compare_exchange_strong(expected, true))
	{
		com_port_ = &com_port;
		worker_ = std::thread(std::bind(&effect_manager::worker, this));
	}
	else
	{
		throw std::runtime_error("Unable to start effect manager twice");
	}
}

void effect_manager::stop()
{
	bool expected = true;
	running_.compare_exchange_strong(expected, false);
	
	if(worker_.joinable())
		worker_.join();
}

namespace
{
struct sa_gradient
{
	color::rgb from_low;
	color::rgb from_high;
	color::rgb peak;
};

struct frequency_point
{
	frequency_point(uint8_t x, uint8_t y)
		: x(x), y(y)
		, current_value(0)
	{
	}

	uint8_t x;
	uint8_t y;
	color::rgb current_color;
	uint8_t current_value;
	display matrix;
};

struct effect_gradient
{
	color::rgb step1; //for levels 0-4
	color::rgb step2; //5-9
	color::rgb step3; //10-15
};

struct low_high_frequency_gradient
{
	color::rgb low_frequency;
	color::rgb high_frequency;
};
} //namespace

void effect_manager::effect_glowing_dots()
{
	std::vector<uint8_t> transformation;
	for(uint8_t i = 0; i != display::display_width * display::display_height; ++i)
		transformation.push_back(i);
	std::shuffle(transformation.begin(), transformation.end(), random_gen_);

	const std::vector<low_high_frequency_gradient> gradients
	{
		{ { 0, 0, 0xff }, { 0xff, 0xff, 0 } },
		{ { 0, 0xff, 0 }, { 0xff, 0x10, 0x10 } },
		{ { 0xff, 0xff, 0xff }, { 0, 0xff, 0xff } },
		{ { 0xff, 0, 0x70 }, { 0xff, 0, 0 } },
		{ color::blueviolet, color::gold },
		{ color::greenyellow, color::firebrick },
		{ color::lightskyblue, color::magenta }
	};

	std::vector<uint8_t> peaks(equalizer::band_count);

	static const uint32_t max_gradient_step = 500;
	size_t current_gradient = 0;
	uint32_t gradient_step = 0;

	display matrix;
	while(running_ && effect_processor_ == effect_processor::glowing_dots)
	{
		auto data = equalizer::get_raw_equalizer_data();

		uint8_t pixel_index = 0;
		for(size_t i = 0; i != equalizer::band_count; ++i)
		{
			uint8_t eq_value = data.at(i);
			if(eq_value > max_eq_value)
				eq_value = max_eq_value;
			else if(eq_value < 4)
				eq_value = 0;

			if(peaks[i] < eq_value)
				peaks[i] = eq_value;
			else if(peaks[i])
				--peaks[i];

			color::rgb low_freq, high_freq;
			const auto& grad = gradients[current_gradient];
			const auto& next_grad = gradients[(current_gradient + 1) % gradients.size()];
			color::gradient(grad.low_frequency, next_grad.low_frequency, max_gradient_step, gradient_step, low_freq);
			color::gradient(grad.high_frequency, next_grad.high_frequency, max_gradient_step, gradient_step, high_freq);

			color::rgb to;
			color::gradient(low_freq, high_freq, equalizer::band_count - 1, i, to);

			color::rgb result_color;
			color::gradient({ 0, 0, 0 }, to, effect_manager::max_eq_value, peaks[i], result_color);

			uint8_t pixel_count = 2;
			uint8_t y = i / display::display_width;
			if(y == 0 || y == (equalizer::band_count / display::display_width) - 1)
				pixel_count = 3;

			for(uint8_t px = 0; px != pixel_count; ++px)
			{
				uint8_t x = transformation[pixel_index] % display::display_width;
				y = transformation[pixel_index] / display::display_width;
				++pixel_index;
				matrix.set_pixel(x, y, result_color);
			}
		}

		if(++gradient_step == max_gradient_step)
		{
			gradient_step = 0;
			current_gradient = (current_gradient + 1) % gradients.size();
		}

		display_protocol::send_data_to_device(matrix, *com_port_);
	}
}

void effect_manager::effect_spectrum_analyzer()
{
	static const uint32_t max_gradient_step = 2000;

	size_t current_gradient = 0;
	uint32_t gradient_step = 0;

	display matrix;
	std::vector<double> prev_data;
	std::vector<double> peaks;
	prev_data.resize(display::display_height);
	peaks.resize(display::display_height);

	const std::vector<sa_gradient> gradients
	{
		{ { 0, 0, 0xff }, { 0xff, 0xff, 0 }, { 0, 0xff, 0 } },
		{ { 0, 0xff, 0 }, { 0xff, 0, 0 }, { 0, 0, 0xff } },
		{ { 0xff, 0xff, 0xff }, { 0, 0, 0xff }, { 0xff, 0, 0 } },
		{ { 0xff, 0, 0 }, { 0x30, 0, 0xa0 }, { 0xff, 0xff, 0 } },
		{ { 0xff, 0x80, 0 }, { 0, 0xff, 0x50 }, { 0x70, 0x70, 0xff } },
		{ { 0x50, 0x50, 0 }, { 0xff, 0xff, 0 }, { 0, 0x90, 0x20 } }
	};

	while(running_ && effect_processor_ == effect_processor::spectrum_analyzer)
	{
		auto data = equalizer::cut_sa_data(equalizer::get_raw_equalizer_data(),
			display::display_height, equalizer::cut_mode::single);

		matrix.clear();
		for(uint8_t y = 0; y != display::display_height; ++y)
		{
			uint8_t eq_value = data.at(y);
			if(eq_value > max_eq_value)
				eq_value = max_eq_value;

			double max_x = static_cast<double>(display::display_width) * eq_value / max_eq_value;
			double prev_value = prev_data[y];
			if(prev_value > max_x)
			{
				max_x = prev_value - 0.7 - (display::display_width - prev_value) / 20;
				if(max_x < 0)
					max_x = 0;
			}
			prev_data[y] = max_x;

			if(max_x - 1 >= peaks[y])
			{
				peaks[y] = max_x - 1;
			}
			else
			{
				peaks[y] -= (display::display_width - peaks[y]) / 10;
				if(peaks[y] < 0)
					peaks[y] = 0;
			}

			color::rgb low, high, peak;
			const auto& grad = gradients[current_gradient];
			const auto& next_grad = gradients[(current_gradient + 1) % gradients.size()];
			color::gradient(grad.from_low, next_grad.from_low, max_gradient_step, gradient_step, low);
			color::gradient(grad.from_high, next_grad.from_high, max_gradient_step, gradient_step, high);
			color::gradient(grad.peak, next_grad.peak, max_gradient_step, gradient_step, peak);

			if(++gradient_step == max_gradient_step)
			{
				gradient_step = 0;
				current_gradient = (current_gradient + 1) % gradients.size();
			}

			for(uint32_t x = 0; x != static_cast<uint32_t>(max_x); ++x)
			{
				color::rgb color(high.r * x / (display::display_width - 1)
					+ low.r * (display::display_width - x - 1) / display::display_width,
					high.g * x / (display::display_width - 1)
					+ low.g * (display::display_width - x - 1) / display::display_width,
					high.b * x / (display::display_width - 1)
					+ low.b * (display::display_width - x - 1) / display::display_width);
				matrix.set_pixel(x, y, color);
			}

			const double peak_value = peaks.at(y);
			if(peak_value > 1)
				matrix.set_pixel(static_cast<uint8_t>(peak_value), y, peak);
		}

		display_protocol::send_data_to_device(matrix, *com_port_);
	}
}

void effect_manager::effect_color_waves()
{
	const std::vector<effect_gradient> gradients
	{
		{ { 0xff, 0, 0 }, { 0xff, 0xff, 0 }, { 0, 0, 0xff } },
		{ { 0, 0xff, 0 }, { 0, 0xff, 0xff }, { 0, 0xff, 0 } },
		{ { 0xff, 0, 0x60 }, { 0, 0xff, 0x60 }, { 0xff, 0xff, 0xff } },
		{ { 0x80, 0x80, 0 }, { 0xff, 0xff, 0xff }, { 0xff, 0, 0 } },
		{ { 0xff, 0x33, 0 }, { 0, 0xff, 0 }, { 0x50, 0xff, 0x50 } }
	};

	std::vector<frequency_point> freq_points
	{
		{ 3, 2 },
		{ 6, 2 },

		{ 2, 4 },
		{ 7, 4 },

		{ 1, 6 },
		{ 8, 6 },

		{ 1, 9 },
		{ 8, 9 },

		{ 2, 11 },
		{ 7, 11 },

		{ 3, 13 },
		{ 6, 13 }
	};

	display matrix;
	size_t current_gradient = 0;
	uint32_t current_gradient_step = 0;
	static const uint32_t max_gradient_step = 2000;
	while(running_ && effect_processor_ == effect_processor::color_waves)
	{
		auto data = equalizer::cut_sa_data(equalizer::get_raw_equalizer_data(),
			static_cast<uint8_t>(freq_points.size()), equalizer::cut_mode::single);

		for(size_t i = 0; i != freq_points.size(); ++i)
		{
			uint8_t eq_value = data.at(i);
			if(eq_value > max_eq_value)
				eq_value = max_eq_value;

			auto& freq_point = freq_points[i];
			uint8_t color_value = freq_point.current_value > eq_value ? freq_point.current_value : eq_value;

			//Determine point color
			const auto& grad = gradients[current_gradient];
			const auto& next_grad = gradients[(current_gradient + 1) % gradients.size()];
			color::rgb step1, step2, step3;
			color::gradient(grad.step1, next_grad.step1, max_gradient_step, current_gradient_step, step1);
			color::gradient(grad.step2, next_grad.step2, max_gradient_step, current_gradient_step, step2);
			color::gradient(grad.step3, next_grad.step3, max_gradient_step, current_gradient_step, step3);

			if(++current_gradient_step == max_gradient_step)
			{
				current_gradient_step = 0;
				current_gradient = (current_gradient + 1) % gradients.size();
			}

			color::rgb from, to;
			uint8_t step = 0;
			if(color_value < 5)
			{
				from = { 0, 0, 0 };
				to = step1;
				step = color_value;
			}
			else if(color_value < 10)
			{
				from = step1;
				to = step2;
				step = color_value - 5;
			}
			else
			{
				from = step2;
				to = step3;
				step = color_value - 10;
			}
			
			color::rgb rgb;
			color::gradient(from, to, 4, step, rgb);

			freq_point.current_color = rgb;
			if(freq_point.current_value < eq_value)
				freq_point.current_value = eq_value;
			else if(freq_point.current_value)
				--freq_point.current_value;
		}

		for(size_t i = 0; i != freq_points.size(); ++i)
		{
			auto& freq_point = freq_points[i];
			freq_point.matrix.set_pixel(freq_point.x, freq_point.y, freq_point.current_color);
			for(uint8_t r = 7; r != 1; --r)
			{
				for(int8_t x = -r; x != r + 1; ++x)
				{
					for(int8_t y = -r; y != r + 1; ++y)
					{
						if(abs(x) + abs(y) == r)
						{
							int8_t prev_x = x;
							int8_t prev_y = y;
							if(x < 0)
							{
								prev_x = x + 1;
							}
							else if(x > 0)
							{
								prev_x = x - 1;
							}
							else
							{
								if(y > 0)
									prev_y = y - 1;
								else
									prev_y = y + 1;
							}

							auto color = freq_point.matrix.get_pixel(freq_point.x + prev_x, freq_point.y + prev_y);
							color.r /= 2;
							color.g /= 2;
							color.b /= 2;
							freq_point.matrix.set_pixel(freq_point.x + x, freq_point.y + y, color);
						}
					}
				}
			}

			for(int8_t x = -1; x != 2; ++x)
			{
				for(int8_t y = -1; y != 2; ++y)
				{
					if(abs(x) + abs(y) == 1)
						freq_point.matrix.set_pixel(freq_point.x + x, freq_point.y + y, freq_point.current_color);
				}
			}
		}

		for(uint8_t x = 0; x != display::display_width; ++x)
		{
			for(uint8_t y = 0; y != display::display_height; ++y)
			{
				uint32_t color_r = 0, color_g = 0, color_b = 0;
				for(size_t i = 0; i != freq_points.size(); ++i)
				{
					auto color = freq_points[i].matrix.get_pixel(x, y);
					color_r += color.r;
					color_g += color.g;
					color_b += color.b;
				}

				color_r = (std::min)(color_r, 0xffu);
				color_g = (std::min)(color_g, 0xffu);
				color_b = (std::min)(color_b, 0xffu);

				matrix.set_pixel(x, y, { static_cast<uint8_t>(color_r), static_cast<uint8_t>(color_g), static_cast<uint8_t>(color_b) });
			}
		}

		display_protocol::send_data_to_device(matrix, *com_port_);
	}
}

void effect_manager::set_effect_processor(effect_processor processor)
{
	effect_processor_ = processor;
}

void effect_manager::on_error(const on_error_callback& error)
{
	error_callback_ = error;
}

void effect_manager::worker()
{
	try
	{
		while(running_)
		{
			switch(effect_processor_)
			{
			case effect_processor::spectrum_analyzer:
				effect_spectrum_analyzer();
				break;

			case effect_processor::color_waves:
				effect_color_waves();
				break;

			case effect_processor::glowing_dots:
				effect_glowing_dots();
				break;

			default:
				break;
			}

			display_protocol::send_data_to_device(display(), *com_port_);
		}
	}
	catch(...)
	{
		if(error_callback_)
			error_callback_();

		running_ = false;
	}
}
