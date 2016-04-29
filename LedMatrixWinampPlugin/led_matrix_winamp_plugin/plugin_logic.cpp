// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "plugin_logic.h"

#include <functional>

#include "equalizer.h"
#include "plugin.h"

plugin_logic::plugin_logic()
{
	equalizer::init(plugin::get_winamp_window());
	manager_.reset(new effect_manager);
	manager_->on_error(std::bind(&plugin_logic::on_effect_error, this));
}

void plugin_logic::on_effect_error()
{
	com_port_.reset();
	if(dialog_)
		dialog_->set_status(plugin_dialog::status::device_offline);
}

void plugin_logic::set_port_list()
{
	dialog_->set_com_port_list(uart::get_available_ports());
}

void plugin_logic::set_modes_list()
{
	dialog_->set_modes_list({ L"Spectrum Analyzer", L"Color Waves", L"Glowing Dots" });
}

void plugin_logic::change_effect(uint32_t mode)
{
	manager_->set_effect_processor(static_cast<effect_manager::effect_processor>(mode));
}

void plugin_logic::on_effect_start(const std::wstring& com_port_name, uint32_t mode_index)
{
	try
	{
		com_port_.reset(new uart(com_port_name, default_baud_rate));
		manager_->start(*com_port_);
	}
	catch(const std::exception&)
	{
		manager_->stop();
		com_port_.reset();
		throw;
	}

	dialog_->set_status(plugin_dialog::status::working);
}

void plugin_logic::on_effect_stop()
{
	manager_->stop();
	com_port_.reset();
	dialog_->set_status(plugin_dialog::status::stopped);
}

void plugin_logic::on_config()
{
	if(!dialog_)
	{
		dialog_.reset(new plugin_dialog(plugin::get_winamp_window(), plugin::get_plugin_instance()));

		dialog_->on_show([this]() { set_port_list(); set_modes_list(); });
		dialog_->on_refresh_com_port_list(std::bind(&plugin_logic::set_port_list, this));
		dialog_->on_start(std::bind(&plugin_logic::on_effect_start, this, std::placeholders::_1, std::placeholders::_2));
		dialog_->on_stop(std::bind(&plugin_logic::on_effect_stop, this));
		dialog_->on_destroy(std::bind(&plugin_logic::on_effect_stop, this));
		dialog_->on_change_mode(std::bind(&plugin_logic::change_effect, this, std::placeholders::_1));
	}

	dialog_->show();
}
