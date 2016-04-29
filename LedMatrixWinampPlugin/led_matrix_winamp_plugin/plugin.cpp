// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "plugin.h"

#include <string>

#include "general_purpose_plugin.h"

std::function<int()> plugin::on_init_callback_;
std::function<void()> plugin::on_config_callback_;
std::function<void()> plugin::on_quit_callback_;
std::string plugin::plugin_name_;
winampGeneralPurposePlugin plugin::plugin_;

void plugin::set_plugin_name(const std::string& name)
{
	plugin_name_ = name;
}

void plugin::on_init(const std::function<int()>& callback)
{
	on_init_callback_ = callback;
}

void plugin::on_config(const std::function<void()>& callback)
{
	on_config_callback_ = callback;
}

void plugin::on_quit(const std::function<void()>& callback)
{
	on_quit_callback_ = callback;
}

int plugin::init()
{
	try
	{
		if(on_init_callback_)
			return on_init_callback_();
	}
	catch(...)
	{
		return 1;
	}

	return 0;
}

void plugin::config()
{
	try
	{
		if(on_config_callback_)
			on_config_callback_();
	}
	catch(...)
	{
	}
}

void plugin::quit()
{
	try
	{
		if(on_quit_callback_)
			on_quit_callback_();
	}
	catch(...)
	{
	}
}

HWND plugin::get_winamp_window()
{
	return plugin_.hwndParent;
}

HINSTANCE plugin::get_plugin_instance()
{
	return plugin_.hDllInstance;
}

winampGeneralPurposePlugin* plugin::get_info()
{
	plugin_ = {
		GPPHDR_VER,  // version of the plugin, defined in "gen_myplugin.h"
		const_cast<char*>(plugin_name_.c_str()), // name/title of the plugin, defined in "gen_myplugin.h"
		&plugin::init,        // function name which will be executed on init event
		&plugin::config,      // function name which will be executed on config event
		&plugin::quit,        // function name which will be executed on quit event
		0,           // handle to Winamp main window, loaded by winamp when this dll is loaded
		0            // hinstance to this dll, loaded by winamp when this dll is loaded
	};

	return &plugin_;
}

extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin()
{
	return plugin::get_info();
}
