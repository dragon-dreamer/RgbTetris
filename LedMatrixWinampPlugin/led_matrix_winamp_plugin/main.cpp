// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include <Windows.h>

#include <memory>

#include "plugin.h"
#include "plugin_logic.h"

namespace
{
std::unique_ptr<plugin_logic> logic;

int on_init()
{
	try
	{
		logic.reset(new plugin_logic);
	}
	catch(const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "LED Matrix UART plugin error", MB_ICONERROR | MB_OK);
		return -1;
	}

	return 0;
}

void on_quit()
{
	logic.reset();
}

void on_config()
{
	logic->on_config();
}
} //namespace

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
	)
{
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		plugin::set_plugin_name("UART LED Matrix plugin");
		plugin::on_init(on_init);
		plugin::on_quit(on_quit);
		plugin::on_config(on_config);
	}

	return TRUE;
}
