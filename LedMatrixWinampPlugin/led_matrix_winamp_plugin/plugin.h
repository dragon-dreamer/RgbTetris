// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <functional>
#include <string>

#include "general_purpose_plugin.h"
#include "static_class.h"

///Winamp plugin helpers
class plugin : static_class
{
public:
	static void set_plugin_name(const std::string& name);

	static void on_init(const std::function<int()>& callback);
	static void on_config(const std::function<void()>& callback);
	static void on_quit(const std::function<void()>& callback);

	static winampGeneralPurposePlugin* get_info();

	static HWND get_winamp_window();
	static HINSTANCE get_plugin_instance();

private:
	static int init();
	static void config();
	static void quit();

	static std::function<int()> on_init_callback_;
	static std::function<void()> on_config_callback_;
	static std::function<void()> on_quit_callback_;
	static std::string plugin_name_;
	static winampGeneralPurposePlugin plugin_;
};
