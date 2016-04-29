// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <memory>

#include "effect_manager.h"
#include "plugin_dialog.h"
#include "uart.h"

///Plugin logic class (connects GUI, UART and effect manager)
class plugin_logic
{
public:
	static const uint32_t default_baud_rate = 115200;

public:
	plugin_logic();

	///This should be called on Winamp config() callback.
	///Shows plugin dialog box.
	void on_config();

private:
	void on_effect_error();

	void set_port_list();
	void set_modes_list();

	void on_effect_start(const std::wstring& com_port_name, uint32_t mode_index);
	void on_effect_stop();

	void change_effect(uint32_t mode);

private:
	std::unique_ptr<uart> com_port_;
	std::unique_ptr<plugin_dialog> dialog_;
	std::unique_ptr<effect_manager> manager_;
};
