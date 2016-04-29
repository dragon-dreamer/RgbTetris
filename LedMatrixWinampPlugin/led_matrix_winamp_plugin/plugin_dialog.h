// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <Windows.h>

struct plugin_dialog_impl;
class plugin_dialog
{
public:
	typedef std::function<void(const std::wstring&, uint32_t)> start_callback;
	typedef std::function<void()> stop_callback;
	typedef std::function<void()> show_callback;
	typedef std::function<void()> destroy_callback;
	typedef std::function<void()> refresh_com_port_list_callback;
	typedef std::function<void(uint32_t)> change_mode_callback;

	typedef std::set<std::wstring> com_port_list;
	typedef std::vector<std::wstring> mode_list;

public:
	enum class status
	{
		stopped,
		working,
		device_offline
	};

public:
	plugin_dialog(HWND winamp_window, HINSTANCE plugin_instance);
	~plugin_dialog();

	void show();

	void set_status(status status_value);

	void set_com_port_list(const com_port_list& list);
	void set_modes_list(const mode_list& list);

	void on_show(const show_callback& show);
	void on_destroy(const destroy_callback& destroy);
	void on_start(const start_callback& start);
	void on_stop(const stop_callback& stop);
	void on_refresh_com_port_list(const refresh_com_port_list_callback& refresh_com_port_list);
	void on_change_mode(const change_mode_callback& change_mode);

private:
	std::unique_ptr<plugin_dialog_impl> impl_;

	static int static_dlg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	int dlg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
