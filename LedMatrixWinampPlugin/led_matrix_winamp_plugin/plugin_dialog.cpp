// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#include "plugin_dialog.h"

#include <cassert>
#include <utility>
#include <Windows.h>

#include "resource.h"
#define WA_DLG_IMPLEMENT
#include "wa_dlg.h"

namespace
{
const UINT set_status_message = WM_USER + 1;
} //namespace

struct plugin_dialog_impl
{
	explicit plugin_dialog_impl(HWND winamp_window, HINSTANCE plugin_instance)
		: winamp_window(winamp_window)
		, plugin_instance(plugin_instance)
		, dialog_window(nullptr)
		, parent_window(nullptr)
		, shown(false)
		, status(plugin_dialog::status::stopped)
	{
		memset(&wa_wnd, 0, sizeof(wa_wnd));
	}

	embedWindowState wa_wnd;
	HWND winamp_window;
	HINSTANCE plugin_instance;
	HWND dialog_window;
	HWND parent_window;
	bool shown;
	plugin_dialog::status status;

	plugin_dialog::com_port_list com_ports;
	plugin_dialog::mode_list modes;

	plugin_dialog::show_callback on_show;
	plugin_dialog::start_callback on_start;
	plugin_dialog::stop_callback on_stop;
	plugin_dialog::destroy_callback on_destroy;
	plugin_dialog::refresh_com_port_list_callback on_refresh_com_port_list;
	plugin_dialog::change_mode_callback on_change_mode;

	template<typename Callback, typename... Params>
	void safe_callback_call(const Callback& callback, Params&&... params)
	{
		try
		{
			if(callback)
				callback(std::forward<Params>(params)...);
		}
		catch(const std::exception& e)
		{
			::MessageBoxA(dialog_window, e.what(), "Error", MB_ICONERROR | MB_OK);
		}
	}

	void enable_needed_controls(bool started)
	{
		::EnableWindow(::GetDlgItem(dialog_window, IDC_STOP), started);
		bool has_list_items = ::SendDlgItemMessageW(dialog_window, IDC_COM_PORT, CB_GETCOUNT, 0, 0)
			&& ::SendDlgItemMessageW(dialog_window, IDC_MODE, CB_GETCOUNT, 0, 0);
		::EnableWindow(::GetDlgItem(dialog_window, IDC_START), !started && has_list_items);
		::EnableWindow(::GetDlgItem(dialog_window, IDC_REFRESH), !started);
		::EnableWindow(::GetDlgItem(dialog_window, IDC_COM_PORT), !started);
	}

	void refresh_status()
	{
		if(dialog_window)
		{
			switch(status)
			{
			case plugin_dialog::status::stopped:
				::SetDlgItemTextW(dialog_window, IDC_STATUS, L"Stopped");
				enable_needed_controls(false);
				break;

			case plugin_dialog::status::working:
				::SetDlgItemTextW(dialog_window, IDC_STATUS, L"Working");
				enable_needed_controls(true);
				break;

			case plugin_dialog::status::device_offline:
				::SetDlgItemTextW(dialog_window, IDC_STATUS, L"Stopped, device offline");
				enable_needed_controls(false);
				break;

			default:
				break;
			}
		}
	}

	template<typename Container>
	void refresh_list(const Container& container, int dlg_item_id)
	{
		if(dialog_window)
		{
			::SendDlgItemMessageW(dialog_window, dlg_item_id, CB_RESETCONTENT, 0, 0);

			for(const auto& elem : container)
			{
				::SendDlgItemMessageW(dialog_window, dlg_item_id, CB_ADDSTRING,
					0, reinterpret_cast<LPARAM>(elem.c_str()));
			}
			
			::SendDlgItemMessageW(dialog_window, dlg_item_id, CB_SETCURSEL, 0, 0);
		}
	}
	
	uint32_t get_selection_index(int dlg_item_id) const
	{
		return ::SendDlgItemMessageW(dialog_window, dlg_item_id, CB_GETCURSEL, 0, 0);
	}

	std::wstring get_selection(int dlg_item_id) const
	{
		uint32_t index = get_selection_index(dlg_item_id);
		uint32_t text_len = ::SendDlgItemMessageW(dialog_window, dlg_item_id, CB_GETLBTEXTLEN, index, 0);
		std::wstring str;
		str.resize(text_len + 1);
		::SendDlgItemMessageW(dialog_window, dlg_item_id, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(&str[0]));
		str.resize(str.size() - 1); //Strip last nullbyte
		return str;
	}
};

void plugin_dialog::set_status(status status_value)
{
	::PostMessageW(impl_->dialog_window, set_status_message, 0, static_cast<LPARAM>(status_value));
}

void plugin_dialog::set_com_port_list(const com_port_list& list)
{
	impl_->com_ports = list;
	impl_->refresh_list(impl_->com_ports, IDC_COM_PORT);
	impl_->refresh_status();
}

void plugin_dialog::set_modes_list(const mode_list& list)
{
	impl_->modes = list;
	impl_->refresh_list(impl_->modes, IDC_MODE);
	impl_->refresh_status();
}

void plugin_dialog::on_show(const show_callback& show)
{
	impl_->on_show = show;
}

void plugin_dialog::on_start(const start_callback& start)
{
	impl_->on_start = start;
}

void plugin_dialog::on_stop(const stop_callback& stop)
{
	impl_->on_stop = stop;
}

void plugin_dialog::on_refresh_com_port_list(const refresh_com_port_list_callback& refresh_com_port_list)
{
	impl_->on_refresh_com_port_list = refresh_com_port_list;
}

void plugin_dialog::on_destroy(const destroy_callback& destroy)
{
	impl_->on_destroy = destroy;
}

void plugin_dialog::on_change_mode(const change_mode_callback& change_mode)
{
	impl_->on_change_mode = change_mode;
}

int plugin_dialog::dlg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		impl_->dialog_window = hWnd;

		WADlg_init(impl_->winamp_window);

		::SetWindowTextW(impl_->parent_window, L"UART LED Matrix Plugin");

		impl_->safe_callback_call(impl_->on_show);

		impl_->refresh_list(impl_->com_ports, IDC_COM_PORT);
		impl_->refresh_list(impl_->modes, IDC_MODE);
		impl_->refresh_status();

		ShowWindow(impl_->parent_window, SW_SHOWNORMAL);
		break;

		// need to correctly handle skin changes
	case WM_DISPLAYCHANGE:
		WADlg_init(impl_->winamp_window);
		{
			// this is a list of controls to winampise
			int tabs[] = { IDC_COM_PORT | DCW_SUNKENBORDER, IDC_MODE | DCW_SUNKENBORDER, IDC_START | DCW_SUNKENBORDER,
				IDC_STOP | DCW_SUNKENBORDER, IDC_REFRESH | DCW_SUNKENBORDER };
			WADlg_DrawChildWindowBorders(hWnd, tabs, sizeof(tabs) / sizeof(tabs[0]));
		}

		// if winamp changes skin then it sends a dummy WM_DISPLAYCHANGE
		// message. on receive do a quick hide and reshow
		// (simple hack to ensure the window is shown correctly with
		//  the newly loaded skin)
		if(!wParam && !lParam)
		{
			ShowWindow(hWnd, SW_HIDE);
			ShowWindow(hWnd, SW_SHOW);
		}
		break;

	case WM_PAINT:
	{
		// this is a list of controls to winampise
		int tabs[] = { IDC_COM_PORT | DCW_SUNKENBORDER, IDC_MODE | DCW_SUNKENBORDER, IDC_START | DCW_SUNKENBORDER,
			IDC_STOP | DCW_SUNKENBORDER, IDC_REFRESH | DCW_SUNKENBORDER };
		WADlg_DrawChildWindowBorders(hWnd, tabs, sizeof(tabs) / sizeof(tabs[0]));
	}
	break;

	case set_status_message:
	{
		status new_status = static_cast<status>(lParam);
		impl_->status = new_status;
		impl_->refresh_status();
	}
	break;

	case WM_COMMAND:
		if(HIWORD(wParam) == CBN_SELCHANGE && lParam == reinterpret_cast<LPARAM>(::GetDlgItem(hWnd, IDC_MODE)))
		{
			impl_->safe_callback_call(impl_->on_change_mode, impl_->get_selection_index(IDC_MODE));
			break;
		}

		switch(LOWORD(wParam))
		{
		case IDC_REFRESH:
			impl_->safe_callback_call(impl_->on_refresh_com_port_list);
			break;

		case IDC_START:
			impl_->safe_callback_call(impl_->on_start, impl_->get_selection(IDC_COM_PORT),
				impl_->get_selection_index(IDC_MODE));
			break;

		case IDC_STOP:
			impl_->safe_callback_call(impl_->on_stop);
			break;

		default:
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(impl_->parent_window);
		break;

	case WM_DESTROY:
		WADlg_close();
		break;

	case WM_NCDESTROY:
		impl_->dialog_window = impl_->parent_window = nullptr;
		impl_->shown = false;
		impl_->safe_callback_call(impl_->on_destroy);
		break;
	}

	return 0;
}

int plugin_dialog::static_dlg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	{
		int ret = WADlg_handleDialogMsgs(hWnd, uMsg, wParam, lParam);
		if(ret)
			return ret;
	}

	if(uMsg == WM_INITDIALOG)
		::SetWindowLongPtrW(hWnd, GWLP_USERDATA, lParam);

	plugin_dialog* dialog = reinterpret_cast<plugin_dialog*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	if(dialog)
		return dialog->dlg_proc(hWnd, uMsg, wParam, lParam);

	return 0;
}

void plugin_dialog::show()
{
	if(impl_->shown)
		return;

	impl_->shown = true;

	{
		RECT rc;
		::GetWindowRect(impl_->winamp_window, &rc);
		impl_->wa_wnd.r.left = rc.right;
		impl_->wa_wnd.r.top = rc.top;
	}

	// sets the width and height if zero or below the minimum width
	// then the window is created with the default size
	impl_->wa_wnd.r.right = 0;
	static const int window_height = 180;
	impl_->wa_wnd.r.bottom = impl_->wa_wnd.r.top + window_height;

	impl_->wa_wnd.flags = EMBED_FLAGS_NORESIZE;

	impl_->parent_window = reinterpret_cast<HWND>(::SendMessageW(impl_->winamp_window, WM_WA_IPC,
		reinterpret_cast<WPARAM>(&impl_->wa_wnd), IPC_GET_EMBEDIF));
	if(impl_->parent_window){
		// here we attach a dialog into the window
		// if need by this could been done with CreateWindow(), etc depending
		// on what you want to do
		::CreateDialogParamW(impl_->plugin_instance, MAKEINTRESOURCE(IDD_PLUGIN_DIALOG),
			impl_->parent_window, reinterpret_cast<DLGPROC>(static_dlg_proc), reinterpret_cast<LPARAM>(this));
	}
}

plugin_dialog::plugin_dialog(HWND winamp_window, HINSTANCE plugin_instance)
	: impl_(new plugin_dialog_impl(winamp_window, plugin_instance))
{
}

plugin_dialog::~plugin_dialog()
{
	assert(!impl_->shown);
}
