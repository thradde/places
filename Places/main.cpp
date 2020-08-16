
// Copyright (C) 2020 IDEAL Software GmbH, Neuss, Germany.
// www.idealsoftware.com
// Author: Thorsten Radde
//
// This program is free software; you can redistribute it and/or modify it under the terms of the GNU 
// General Public License as published by the Free Software Foundation; either version 2 of the License, 
// or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even 
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General Public 
// License for more details.
//
// You should have received a copy of the GNU General Public License along with this program; if not, 
// write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
//


#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#include <Commctrl.h>

#include "memdebug.h"

#include <SFML/Graphics.hpp>

#include <Shellapi.h>		// for DragAcceptFiles()
#include <Shlobj.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <process.h>

#include <vector>
#include <set>
#include <unordered_map>
using namespace std;

#define ASSERT
#include "SystemTraySDK.h"
#include "resource.h"
#include "hook.h"
#include "RfwString.h"
#include "exceptions.h"
#include "Mutex.h"
#include "stream.h"
#include "platform.h"
#include "generic.h"
#include "configuration.h"
#include "popup_menu.h"
#include "md5.h"
#include "bitmap_cache.h"
#include "icon.h"
#include "icon_page.h"
#include "undo.h"
#include "icon_manager.h"

#include "TrayIcon.c"		// created with Gimp


// Use commctrl32 version 6 for newer visual style
#pragma comment(linker,	"\"/manifestdependency:type='win32' \
						name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
						processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define APP_NAME		_T("Places")
#define APP_CLASS_NAME	_T("PlacesClass")
#define DATA_FOLDER		_T("\\Places\\")
#define SETTINGS_FILE	_T("places.settings")
#define DATA_FILE		_T("places.database")

#define SHADER_FOLDER	_T("Shader\\")
#define IMAGE_FOLDER	_T("Wallpaper\\")

bool gbTerminateActivityThread;
unsigned __stdcall ActivityThread(void *param);
unsigned __stdcall ResyncThread(void *param);
unsigned __stdcall ShellExecuteThread(void *param);


/*
Tips:
-----
    - A great set of free and beautiful icons can be found in the "Open Icon Library". Use the 128 x 128 pixel size.
    - The background patterns shipped with Places are taken from http://subtlepatterns.com/, you can find there many other patterns.
    - You can activate Places with a double-click into the free desktop space of the Windows program manager
    - You can drag a folder into Places. If you click onto the folder, it will open in Windows Explorer.
    - You can edit an icon's file path and set it to: http://www.something.com. A click onto it will open the URL in the default web browser.
            
    Useful locations:
    - My Computer:          File: c:\windows\explorer.exe               Parameters: =
    - Network Computers:    File: c:\windows\explorer.exe               Parameters: ::{208d2c60-3aea-1069-a2d7-08002b30309d}
    - Recycle Bin:          File: c:\windows\explorer.exe               Parameters: ::{645ff040-5081-101b-9f08-00aa002f954e}
    - A hard drive:         File: c:\windows\explorer.exe               Parameters: <drive letter>: (eg "c:")
    - Control Panel:        File: c:\windows\system32\control.exe       Parameters: <none>
    - Add/Remove Programs:  File: c:\windows\system32\control.exe       Parameters: appwiz.cpl
    - Printers:             File: c:\windows\system32\control.exe       Parameters: printers.cpl
    - Sound Properties:     File: c:\windows\system32\control.exe       Parameters: mmsys.cpl sounds
    - Scanners and Cameras: File: c:\windows\system32\control.exe       Parameters: sticpl.cpl
    - Fonts:                File: c:\windows\system32\control.exe       Parameters: fonts.cpl
    - Device Manager:       File: c:\windows\system32\mmc.exe           Parameters: devmgmt.msc
    - Computer Management:  File: c:\windows\system32\mmc.exe           Parameters: compmgmt.msc
    - Disk Management:      File: c:\windows\system32\mmc.exe           Parameters: diskmgmt.msc
*/


// --------------------------------------------------------------------------------------------------------------------------------------------
//																	Macros
// --------------------------------------------------------------------------------------------------------------------------------------------
#define EDIT_MAX_CHARS	1024


// --------------------------------------------------------------------------------------------------------------------------------------------
//																	const
// --------------------------------------------------------------------------------------------------------------------------------------------
const int HoverTime = 300;		// in ms
const int HoverTolerance = 3;	// in pixel


// --------------------------------------------------------------------------------------------------------------------------------------------
//																Globals
// --------------------------------------------------------------------------------------------------------------------------------------------
static HINSTANCE ghInstance;
static CSettings gSettings;	// global settings, like rows, cols, font size, etc.

static RString gstrIconTitle, gstrFilePath, gstrParameters, gstrIconPath;
static bool gbOpenAsAdmin;
static TCHAR szDlgExchange[EDIT_MAX_CHARS + 1];

typedef struct 
{
	LPCTSTR	TitleEn;	// english title
	LPCTSTR	TitleDe;	// german title
	LPCTSTR	Icon;		// icon file name
	LPCTSTR	Path;		// icon file path
	LPCTSTR	Params;		// icon parameters
} TDefaultIcons;


TDefaultIcons gDefaultIcons[] =
{
	_T("Computer"),					_T("Arbeitsplatz"),				_T("drive-harddisk-8.png"),				_T("<explorer>"),					_T("="),
	_T("Network"),					_T("Netzwerk"),					_T("network-local.png"),				_T("<explorer>"),					_T("shell:NetworkPlacesFolder"),
	_T("Recycle Bin"),				_T("Papierkorb"),				_T("user-trash-full.png"),				_T("<explorer>"),					_T("::{645ff040-5081-101b-9f08-00aa002f954e}"),
	_T("Computer\nManagement"),		_T("Computer\nVerwaltung"),		_T("computer-4.png"),					_T("<mmc>"),						_T("compmgmt.msc"),
	_T("Drive\nManagement"),		_T("Datenträger\nVerwaltung"),	_T("server-database.png"),				_T("<mmc>"),						_T("diskmgmt.msc"),
	_T("Device\nManagement"),		_T("Geräte\nManager"),			_T("configure-4.png"),					_T("<mmc>"),						_T("devmgmt.msc"),
	_T("IDEAL\nSoftware"),			_T("IDEAL\nSoftware"),			_T("globe.png"),						_T("http://www.idealsoftware.com"),		_T(""),

	_T("Control Panel"),			_T("System-\nsteuerung"),		_T("system-settings.png"),				_T("<control>"),					_T(""),
	_T("Add / Remove\nPrograms"),	_T("Programme\nentfernen"),		_T("system-installer.png"),				_T("<control>"),					_T("appwiz.cpl"),
	_T("Sound Settings"),			_T("Sound\nEinstellungen"),		_T("preferences-desktop-sound.png"),	_T("<control>"),					_T("mmsys.cpl sounds"),
	_T("Devices\nand Printers"),	_T("Geräte\nund Drucker"),		_T("printer-7.png"),					_T("<control>"),					_T("printers"),
	_T("Fonts"),					_T("Fonts"),					_T("fontforge.png"),					_T("<explorer>"),					_T("shell:Fonts"),
	_T("Scanners\nand Cameras"),	_T("Scanner\nund Kameras"),		_T("camera-photo-10.png"),				_T("<control>"),					_T("sticpl.cpl"),
	_T("Registry\nEditor"),			_T("Registry\nEditor"),			_T("regedit.png"),						_T("regedit.exe"),					_T(""),
	nullptr,						nullptr,						nullptr,								nullptr,							nullptr,
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//															DlgProcEditIconTitle()
// --------------------------------------------------------------------------------------------------------------------------------------------
LRESULT CALLBACK DlgIconProperties(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
		PostMessage(hWndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hWndDlg, ID_ICON_TITLE), TRUE);
		SendMessage(GetDlgItem(hWndDlg, ID_ICON_TITLE), EM_LIMITTEXT, EDIT_MAX_CHARS, 0);
		SetDlgItemText(hWndDlg, ID_ICON_TITLE, gstrIconTitle.c_str());

		SendMessage(GetDlgItem(hWndDlg, ID_FILE_PATH), EM_LIMITTEXT, EDIT_MAX_CHARS, 0);
		SetDlgItemText(hWndDlg, ID_FILE_PATH, gstrFilePath.c_str());

		SendMessage(GetDlgItem(hWndDlg, ID_PARAMETERS), EM_LIMITTEXT, EDIT_MAX_CHARS, 0);
		SetDlgItemText(hWndDlg, ID_PARAMETERS, gstrParameters.c_str());

		SendMessage(GetDlgItem(hWndDlg, ID_ICON_PATH), EM_LIMITTEXT, EDIT_MAX_CHARS, 0);
		SetDlgItemText(hWndDlg, ID_ICON_PATH, gstrIconPath.c_str());

		SendMessage(GetDlgItem(hWndDlg, ID_RUN_AS_ADMIN), BM_SETCHECK, gbOpenAsAdmin ? BST_CHECKED : BST_UNCHECKED, 0);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILE_BROWSE:
			{
				GetDlgItemText(hWndDlg, ID_FILE_PATH, szDlgExchange, _tcschars(szDlgExchange));
				CFileDialog file_dialog(szDlgExchange);
				if (file_dialog.GetOpenName(hWndDlg))
					SetDlgItemText(hWndDlg, ID_FILE_PATH, file_dialog.GetPath().c_str());
			}
			break;

		case ID_ICON_BROWSE:
			{
				GetDlgItemText(hWndDlg, ID_ICON_PATH, szDlgExchange, _tcschars(szDlgExchange));
				CFileDialog file_dialog(szDlgExchange);
				if (file_dialog.GetOpenName(hWndDlg))
					SetDlgItemText(hWndDlg, ID_ICON_PATH, file_dialog.GetPath().c_str());
			}
			break;

		case IDOK:
			GetDlgItemText(hWndDlg, ID_ICON_TITLE, szDlgExchange, _tcschars(szDlgExchange));
			gstrIconTitle = szDlgExchange;

			GetDlgItemText(hWndDlg, ID_FILE_PATH, szDlgExchange, _tcschars(szDlgExchange));
			gstrFilePath = szDlgExchange;

			GetDlgItemText(hWndDlg, ID_PARAMETERS, szDlgExchange, _tcschars(szDlgExchange));
			gstrParameters = szDlgExchange;

			GetDlgItemText(hWndDlg, ID_ICON_PATH, szDlgExchange, _tcschars(szDlgExchange));
			gstrIconPath = szDlgExchange;

			gbOpenAsAdmin = SendMessage(GetDlgItem(hWndDlg, ID_RUN_AS_ADMIN), BM_GETCHECK, 0, 0) == BST_CHECKED;

			EndDialog(hWndDlg, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hWndDlg, 0);
			return TRUE;
		}
		break;
	}

	return FALSE;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															PickColorDialog()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool PickColorDialog(HWND hwndParent, COLORREF &color)
{
	CHOOSECOLOR cc;						// common dialog box structure 
	static COLORREF acrCustClr[16] =	// array of custom colors 
	{
		RGB(0,     5,   5),
		RGB(0,    15,  55),
		RGB(0,    25, 155),
		RGB(0,    35, 255),
		RGB(10,    0,   5),
		RGB(10,   20,  55),
		RGB(10,   40, 155),
		RGB(10,   60, 255),
		RGB(100,   5,   5),
		RGB(100,  25,  55),
		RGB(100,  50, 155),
		RGB(100, 125, 255),
		RGB(200, 120,   5),
		RGB(200, 150,  55),
		RGB(200, 200, 155),
		RGB(200, 250, 255),
	};

	// Initialize CHOOSECOLOR 
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hwndParent;
	cc.lpCustColors = (LPDWORD)acrCustClr;
	cc.rgbResult = color;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColor(&cc)) 
	{
		color = cc.rgbResult;
		return true;
	}

	return false;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															HotKey Control Subclass
// --------------------------------------------------------------------------------------------------------------------------------------------
WNDPROC wpOrigEditProc;
WNDPROC wpOrigCmbProc;
WNDPROC wpOrigDropDownProc;

// instance data of hotkey control (so we can have multiple hotkey controls in the same dialog)
class CHotkeyData
{
public:
	BYTE	HotkeyModifiers;
	BYTE	VirtKeyCode;
	BYTE	StoredHotkeyModifiers;
	BYTE	StoredVirtKeyCode;
	RString strKeyState;
	bool	bRecordedShift;
	bool	bRecordedTab;

public:
	CHotkeyData()
		: HotkeyModifiers(0),
		VirtKeyCode(0),
		StoredHotkeyModifiers(0),
		StoredVirtKeyCode(0),
		bRecordedShift(false),
		bRecordedTab(false)
	{
	}
};

typedef unordered_map<HWND, CHotkeyData> THotkeyData;
typedef THotkeyData::iterator THotkeyDataIter;

THotkeyData mapHotkeyData;

#define WM_MY_SETKEYS				(WM_USER)
#define WM_MY_GETHOTKEYMODIFIERS	(WM_USER + 1)
#define WM_MY_GETVIRTKEY			(WM_USER + 2)

// What the original HOTKEY_CLASS of Windows (found docu in the internet) does:
// WM_CHAR			Retrieves the virtual key code.
// WM_CREATE		Initializes the hot key control, clears any hot key rules, and uses the system font.
// WM_ERASEBKGND	Hides the caret, calls the DefWindowProc function, and shows the caret again.
// WM_GETDLGCODE	Returns a combination of the DLGC_WANTCHARS and DLGC_WANTARROWS values.
// WM_GETFONT		Retrieves the font.
// WM_KEYDOWN		Calls the DefWindowProc function if the key is ENTER, TAB, SPACE BAR, DEL, ESC, or BACKSPACE.If the key is SHIFT, CTRL, or ALT, it checks whether the combination is valid and, if it is, sets the hot key using the combination.All other keys are set as hot keys without their validity being checked first.
// WM_KEYUP			Retrieves the virtual key code.
// WM_KILLFOCUS		Destroys the caret.
// WM_LBUTTONDOWN	Sets the focus to the window.
// WM_NCCREATE		Sets the WS_EX_CLIENTEDGE window style.
// WM_PAINT			Paints the hot key control.
// WM_SETFOCUS		Creates and shows the caret.
// WM_SETFONT		Sets the font.
// WM_SYSCHAR		Retrieves the virtual key code.
// WM_SYSKEYDOWN	Calls the DefWindowProc function if the key is ENTER, TAB, SPACE BAR, DEL, ESC, or BACKSPACE.If the key is SHIFT, CTRL, or ALT, it checks whether the combination is valid and, if it is, sets the hot key using the combination.All other keys are set as hot keys without their validity being checked first.
// WM_SYSKEYUP		Retrieves the virtual key code.

// Edit Hotkey Subclass procedure
LRESULT APIENTRY EditHotkeyProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CHotkeyData &data = mapHotkeyData[hwnd];
	BYTE LastHotkeyModifiers;
	LRESULT lres;

	switch (uMsg)
	{
	case WM_GETDLGCODE:
		lres = CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);
		lres |= DLGC_WANTCHARS + DLGC_WANTARROWS;
		return lres;

	case WM_CREATE:
		//mapHotkeyData.insert(pair<HWND, CHotkeyData>(hwnd, CHotkeyData())); ==> done by declaration: CHotkeyData &data = mapHotkeyData[hwnd];
		break;	// use break, so DefWindowProc of original control is called

	case WM_NCDESTROY:
		mapHotkeyData.erase(hwnd);
		break;	// use break, so DefWindowProc of original control is called

	case WM_MY_SETKEYS:
		data.StoredHotkeyModifiers = (BYTE)wParam;
		data.StoredVirtKeyCode = (BYTE)lParam;
		data.strKeyState = GetKeyComboString(data.StoredHotkeyModifiers, data.StoredVirtKeyCode);
		Edit_SetText(hwnd, data.strKeyState.c_str());
		break;

	case WM_MY_GETHOTKEYMODIFIERS:
		return data.StoredHotkeyModifiers;

	case WM_MY_GETVIRTKEY:
		return data.StoredVirtKeyCode;

	case WM_KILLFOCUS:
		if (data.StoredHotkeyModifiers == 0 || data.StoredVirtKeyCode == 0)
		{
			data.StoredHotkeyModifiers = 0;
			data.StoredVirtKeyCode = 0;
			data.strKeyState = GetKeyComboString(data.StoredHotkeyModifiers, data.StoredVirtKeyCode);
			Edit_SetText(hwnd, data.strKeyState.c_str());
		}
		data.HotkeyModifiers = 0;	// this one is important if edit control was quit by shift + tab keycombo
		data.VirtKeyCode = 0;
		break;

	case WM_SETFOCUS:
		// record keys that are pressed right now
		data.bRecordedShift = GetKeyState(VK_SHIFT) != 0;
		data.bRecordedTab = GetKeyState(VK_TAB) != 0;
		break;

	case WM_CHAR:
	case WM_SYSCHAR:
		return 0;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		switch (wParam)
		{
		case VK_MENU:	data.HotkeyModifiers &= ~HotkeyAlt; break;
		case VK_CONTROL:data.HotkeyModifiers &= ~HotkeyCtrl; break;

		case VK_SHIFT:
			if (data.bRecordedShift)
			{
				data.bRecordedShift = false;
				return 0;
			}
			data.HotkeyModifiers &= ~HotkeyShift;
			break;

		case VK_LWIN:
		case VK_RWIN:
			data.HotkeyModifiers &= ~HotkeyWin;
			break;

		case VK_TAB:
			break;

		default:
			data.StoredVirtKeyCode = data.VirtKeyCode;
			data.VirtKeyCode = 0;
		}

		if (data.HotkeyModifiers == 0 && data.StoredVirtKeyCode == 0)
			data.VirtKeyCode = 0;

		if (data.StoredVirtKeyCode == 0)
			data.StoredHotkeyModifiers = data.HotkeyModifiers;

		data.strKeyState = GetKeyComboString(data.StoredHotkeyModifiers, data.StoredVirtKeyCode);
		Edit_SetText(hwnd, data.strKeyState.c_str());
		return 0;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		// Calls the DefWindowProc function if the key is ENTER, TAB, SPACE BAR, DEL, ESC, or BACKSPACE.
		// If the key is SHIFT, CTRL, or ALT, it checks whether the combination is valid and, if it is, 
		// sets the hot key using the combination.
		// All other keys are set as hot keys without their validity being checked first.
		LastHotkeyModifiers = data.HotkeyModifiers;
		switch (wParam)
		{
		case VK_DELETE:
		case VK_BACK:
			data.HotkeyModifiers = 0;
			data.VirtKeyCode = 0;
			break;

		case VK_RETURN:
		case VK_TAB:
		case VK_ESCAPE:
			return CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);

		case VK_MENU:	data.HotkeyModifiers |= HotkeyAlt; break;
		case VK_CONTROL:data.HotkeyModifiers |= HotkeyCtrl; break;
		case VK_SHIFT:	data.HotkeyModifiers |= HotkeyShift; break;
		case VK_LWIN:
		case VK_RWIN:	data.HotkeyModifiers |= HotkeyWin; break;

		default:
			data.VirtKeyCode = (BYTE)(wParam & 0xff);
			if (data.HotkeyModifiers == 0)
			{
				data.StoredHotkeyModifiers = 0;
				data.StoredVirtKeyCode = 0;
				data.VirtKeyCode = 0;
			}
		}

		if (LastHotkeyModifiers != data.HotkeyModifiers)
		{
			data.VirtKeyCode = 0;
			data.StoredVirtKeyCode = 0;
		}

		if (data.StoredVirtKeyCode == 0)
			data.StoredHotkeyModifiers = data.HotkeyModifiers;
		//data.StoredVirtKeyCode = data.VirtKeyCode;
		data.strKeyState = GetKeyComboString(data.StoredHotkeyModifiers, data.VirtKeyCode);
		Edit_SetText(hwnd, data.strKeyState.c_str());
		return 0;
	}

	return CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);
}



// Combobox Drop Down List Subclass procedure
LRESULT APIENTRY CmbDropDownProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_XBUTTONUP:
		{
			WORD btn = HIWORD(wParam);
			if (btn <= 5)
			{
				SendMessage(hwnd, LB_SETCURSEL, btn + 1, 0);
				PostMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 1);
			}

			return 0;
		}
		break;

	case WM_MBUTTONUP:
		SendMessage(hwnd, LB_SETCURSEL, 1, 0);
		PostMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 1);
		break;
	}

	return CallWindowProc(wpOrigDropDownProc, hwnd, uMsg, wParam, lParam);
}


// Combobox Mouse Button Subclass procedure
LRESULT APIENTRY CmbMouseBtnProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_XBUTTONUP:
		{
			WORD btn = HIWORD(wParam);
			if (btn <= 5)
				SendMessage(hwnd, CB_SETCURSEL, btn + 1, 0);

			return 0;
		}
		break;

	case WM_MBUTTONUP:
		SendMessage(hwnd, CB_SETCURSEL, 1, 0);
		break;
	}

	return CallWindowProc(wpOrigCmbProc, hwnd, uMsg, wParam, lParam);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															DlgProcSettings()
// --------------------------------------------------------------------------------------------------------------------------------------------
LRESULT CALLBACK DlgSettings(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND hWndComboBox;
	bool tile, scale;
	static COLORREF clrTmpBkgColor;
	static bool org_suspend_hooks;

	switch(Msg)
	{
	case WM_INITDIALOG:
		// Subclass the edit control
		org_suspend_hooks = gbSuspendHooks;
		gbSuspendHooks = true;
		wpOrigEditProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hWndDlg, ID_HOTKEY), GWLP_WNDPROC, (LONG_PTR)EditHotkeyProc);
		SendMessage(GetDlgItem(hWndDlg, ID_HOTKEY), WM_MY_SETKEYS, gbHotkeyModifiers, gbHotkeyVKey);

		// Subclass the Mouse Button Combobox
		wpOrigCmbProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hWndDlg, ID_MOUSE_BUTTON), GWLP_WNDPROC, (LONG_PTR)CmbMouseBtnProc);

		// Subclass the drop down list of the Mouse Button Combobox
		COMBOBOXINFO cbi;
		cbi.cbSize = sizeof(COMBOBOXINFO);
		GetComboBoxInfo(GetDlgItem(hWndDlg, ID_MOUSE_BUTTON), &cbi);
		wpOrigDropDownProc = (WNDPROC)SetWindowLongPtr(cbi.hwndList, GWLP_WNDPROC, (LONG_PTR)CmbDropDownProc);


		hWndComboBox = GetDlgItem(hWndDlg, ID_MOUSE_BUTTON);
		SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)_T("None"));
		SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)_T("Middle Mouse Button"));
		SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)_T("Extra Button 1"));
		SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)_T("Extra Button 2"));
		SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)_T("Extra Button 3"));
		SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)_T("Extra Button 4"));
		SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)_T("Extra Button 5"));
		SendMessage(hWndComboBox, CB_SETCURSEL, gnMouseButton, 0);

		SendMessage(GetDlgItem(hWndDlg, ID_MOUSE_CORNER), BM_SETCHECK, gnMouseCorner ? BST_CHECKED : BST_UNCHECKED, 0);
		SendMessage(GetDlgItem(hWndDlg, ID_SCROLL_JUMP), BM_SETCHECK, gSettings.m_bScrollWheelJumps ? BST_CHECKED : BST_UNCHECKED, 0);
		SendMessage(GetDlgItem(hWndDlg, ID_DISABLE_MAXIMIZED), BM_SETCHECK, gSettings.m_bDisableIfMaxWin ? BST_CHECKED : BST_UNCHECKED, 0);

		SendMessage(GetDlgItem(hWndDlg, ID_SHADER), EM_LIMITTEXT, EDIT_MAX_CHARS, 0);
		SetDlgItemText(hWndDlg, ID_SHADER, gSettings.m_strShaderFile.c_str());
		SendMessage(GetDlgItem(hWndDlg, ID_SHADER_SPEED), TBM_SETRANGE, TRUE, MAKELONG(1, 100));
		SendMessage(GetDlgItem(hWndDlg, ID_SHADER_SPEED), TBM_SETPOS, TRUE, gSettings.m_nShaderSpeed);
		_itot(gSettings.m_nShaderSpeed, szDlgExchange, 10);
		SetDlgItemText(hWndDlg, ID_SHADER_SPEED_DISPLAY, szDlgExchange);

		SendMessage(GetDlgItem(hWndDlg, ID_IMAGE), EM_LIMITTEXT, EDIT_MAX_CHARS, 0);
		SetDlgItemText(hWndDlg, ID_IMAGE, gSettings.m_strImageFile.c_str());
		SendMessage(GetDlgItem(hWndDlg, ID_TILE_IMAGE), BM_SETCHECK, (gSettings.m_nModifyImage == 1) ? BST_CHECKED : BST_UNCHECKED, 0);
		SendMessage(GetDlgItem(hWndDlg, ID_SCALE_IMAGE), BM_SETCHECK, (gSettings.m_nModifyImage == 2) ? BST_CHECKED : BST_UNCHECKED, 0);
		clrTmpBkgColor = gSettings.m_clrBkgColor;

		_itot(gSettings.m_nColumns, szDlgExchange, 10);
		SetDlgItemText(hWndDlg, ID_COLUMNS, szDlgExchange);
		_itot(gSettings.m_nRows, szDlgExchange, 10);
		SetDlgItemText(hWndDlg, ID_ROWS, szDlgExchange);
		_itot(gSettings.m_nIconFontSize, szDlgExchange, 10);
		SetDlgItemText(hWndDlg, ID_FONT_SIZE, szDlgExchange);

		SendMessage(GetDlgItem(hWndDlg, ID_PLAY_CLICK_SOUND), BM_SETCHECK, gSettings.m_bPlayClickSound ? BST_CHECKED : BST_UNCHECKED, 0);
		return TRUE;
	
	case WM_NCDESTROY:
		// Remove the subclass from the edit control.
		SetWindowLongPtr(GetDlgItem(hWndDlg, ID_HOTKEY), GWLP_WNDPROC, (LONG_PTR)wpOrigEditProc);
		gbSuspendHooks = org_suspend_hooks;
		break;

	case WM_HSCROLL:	// trackbar notification
		{
			WORD w = (WORD)SendMessage(GetDlgItem(hWndDlg, ID_SHADER_SPEED), TBM_GETPOS, 0, 0);
			_itot(w, szDlgExchange, 10);
			SetDlgItemText(hWndDlg, ID_SHADER_SPEED_DISPLAY, szDlgExchange);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_BTN_CLEAR_SHADER:
			SetDlgItemText(hWndDlg, ID_SHADER, _T(""));
			break;

		case ID_BROWSE_SHADER:
			{
				GetDlgItemText(hWndDlg, ID_SHADER, szDlgExchange, _tcschars(szDlgExchange));
				if (*szDlgExchange == '\0')
				{
					_tcscpy(szDlgExchange, gstrCommonAppData);
					_tcscat(szDlgExchange, SHADER_FOLDER);
					_tcscat(szDlgExchange, _T("*"));
				}
				CFileDialog file_dialog(szDlgExchange);
				if (file_dialog.GetOpenName(hWndDlg))
					SetDlgItemText(hWndDlg, ID_SHADER, file_dialog.GetPath().c_str());
			}
			break;

		case ID_BTN_CLEAR_IMAGE:
			SetDlgItemText(hWndDlg, ID_IMAGE, _T(""));
			break;

		case ID_BROWSE_IMAGE:
			{
				GetDlgItemText(hWndDlg, ID_IMAGE, szDlgExchange, _tcschars(szDlgExchange));
				if (*szDlgExchange == '\0')
				{
					_tcscpy(szDlgExchange, gstrCommonAppData);
					_tcscat(szDlgExchange, IMAGE_FOLDER);
					_tcscat(szDlgExchange, _T("*"));
				}
				CFileDialog file_dialog(szDlgExchange);
				if (file_dialog.GetOpenName(hWndDlg))
					SetDlgItemText(hWndDlg, ID_IMAGE, file_dialog.GetPath().c_str());
			}
			break;

		case ID_TILE_IMAGE:
		case ID_SCALE_IMAGE:
			tile = SendMessage(GetDlgItem(hWndDlg, ID_TILE_IMAGE), BM_GETCHECK, 0, 0) == BST_CHECKED;
			scale = SendMessage(GetDlgItem(hWndDlg, ID_SCALE_IMAGE), BM_GETCHECK, 0, 0) == BST_CHECKED;
			if (tile && scale)
			{
				if (LOWORD(wParam) == ID_TILE_IMAGE)
					SendMessage(GetDlgItem(hWndDlg, ID_SCALE_IMAGE), BM_SETCHECK, BST_UNCHECKED, 0);
				else
					SendMessage(GetDlgItem(hWndDlg, ID_TILE_IMAGE), BM_SETCHECK, BST_UNCHECKED, 0);
			}
			break;

		case ID_BROWSE_COLOR:
			if (PickColorDialog(hWndDlg, clrTmpBkgColor))
				InvalidateRect(hWndDlg, NULL, TRUE);
			break;

		case IDOK:
			gbHotkeyVKey = (BYTE)SendMessage(GetDlgItem(hWndDlg, ID_HOTKEY), WM_MY_GETVIRTKEY, 0, 0);
			gbHotkeyModifiers = (BYTE)SendMessage(GetDlgItem(hWndDlg, ID_HOTKEY), WM_MY_GETHOTKEYMODIFIERS, 0, 0);

			gnMouseButton = (int)SendMessage(GetDlgItem(hWndDlg, ID_MOUSE_BUTTON), CB_GETCURSEL, 0, 0);
			gnMouseCorner = SendMessage(GetDlgItem(hWndDlg, ID_MOUSE_CORNER), BM_GETCHECK, 0, 0) == BST_CHECKED;
			gSettings.m_bScrollWheelJumps = SendMessage(GetDlgItem(hWndDlg, ID_SCROLL_JUMP), BM_GETCHECK, 0, 0) == BST_CHECKED;
			gSettings.m_bDisableIfMaxWin = SendMessage(GetDlgItem(hWndDlg, ID_DISABLE_MAXIMIZED), BM_GETCHECK, 0, 0) == BST_CHECKED;
			gSettings.m_bPlayClickSound = SendMessage(GetDlgItem(hWndDlg, ID_PLAY_CLICK_SOUND), BM_GETCHECK, 0, 0) == BST_CHECKED;

			GetDlgItemText(hWndDlg, ID_SHADER, szDlgExchange, _tcschars(szDlgExchange));
			gSettings.m_strShaderFile = szDlgExchange;
			gSettings.m_nShaderSpeed = (int)SendMessage(GetDlgItem(hWndDlg, ID_SHADER_SPEED), TBM_GETPOS, 0, 0);

			GetDlgItemText(hWndDlg, ID_IMAGE, szDlgExchange, _tcschars(szDlgExchange));
			gSettings.m_strImageFile = szDlgExchange;
			tile = SendMessage(GetDlgItem(hWndDlg, ID_TILE_IMAGE), BM_GETCHECK, 0, 0) == BST_CHECKED;
			scale = SendMessage(GetDlgItem(hWndDlg, ID_SCALE_IMAGE), BM_GETCHECK, 0, 0) == BST_CHECKED;
			gSettings.m_nModifyImage = 0;
			if (tile)
				gSettings.m_nModifyImage = 1;
			else if (scale)
				gSettings.m_nModifyImage = 2;
			gSettings.m_clrBkgColor = clrTmpBkgColor;
			GetDlgItemText(hWndDlg, ID_COLUMNS, szDlgExchange, _tcschars(szDlgExchange));
			gSettings.m_nColumns = _ttoi(szDlgExchange);
			GetDlgItemText(hWndDlg, ID_ROWS, szDlgExchange, _tcschars(szDlgExchange));
			gSettings.m_nRows = _ttoi(szDlgExchange);
			GetDlgItemText(hWndDlg, ID_FONT_SIZE, szDlgExchange, _tcschars(szDlgExchange));
			gSettings.m_nIconFontSize = _ttoi(szDlgExchange);

			// check limits
			// min:  8 x  4
			// max: 48 x 24 (for 4K)
			if (gSettings.m_nColumns < 8)
				gSettings.m_nColumns = 8;
			else if (gSettings.m_nColumns > 48)
				gSettings.m_nColumns = 48;
			if (gSettings.m_nRows < 4)
				gSettings.m_nRows = 4;
			else if (gSettings.m_nRows >24)
				gSettings.m_nRows = 24;

			EndDialog(hWndDlg, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hWndDlg, 0);
			return TRUE;
		}
		break;

	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hWndDlg, ID_COLOR))
		{
			// Change text color and text-bkg-color
			//HDC hdcStatic = (HDC)wParam;
			//SetTextColor(hdcStatic, RGB(0,0,255));
			//SetBkColor(hdcStatic, RGB(250,250,0));
			return (INT_PTR)CreateSolidBrush(clrTmpBkgColor);	// control's bkg color
		}
		break;
	}

	return FALSE;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															DlgAbout()
// --------------------------------------------------------------------------------------------------------------------------------------------
LRESULT CALLBACK DlgAbout(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_WEBLINK:
			ShellExecute(hWndDlg, _T("open"), _T("http://www.idealsoftware.com"), NULL, NULL, SW_SHOWNORMAL);
			break;

		case IDOK:
			EndDialog(hWndDlg, 1);
			return TRUE;
		}
		break;
	}

	return FALSE;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														class CPageIndicators
// the circles at the bottom of the view
// --------------------------------------------------------------------------------------------------------------------------------------------
class CPageIndicators
{
protected:
	float					m_fRadius;
	sf::Color				m_clrSelected;
	sf::Color				m_clrDeselected;
	vector<sf::FloatRect>	m_arIndicators;

public:
	CPageIndicators()
		:	m_clrSelected(sf::Color(250, 250, 250, 255)),
			m_clrDeselected(sf::Color(0x47, 0x54, 0x6f, 0xff))
	{
	}

	void Compute(size_t nPages)
	{
		float window_width = (float)gConfig.m_nWindowWidth;
		float window_height = (float)gConfig.m_nWindowHeight;
		float bottom_space = (float)gConfig.m_nBottomSpace;

		m_arIndicators.clear();

		if (nPages > 1)
		{
			// we have a reserved space at the window bottom
			m_fRadius = bottom_space / 18.f;	// 14.f
			float pos_y = window_height - bottom_space * .33f - m_fRadius;
			float indicator_space = 8.f * m_fRadius;
			float total_width = (float)nPages * indicator_space;
			float pos_x = (window_width - total_width) / 2.f;

			float x = pos_x;
			for (size_t page = 0; page < nPages; page++)
			{
				sf::FloatRect rc(x, pos_y, 2 * m_fRadius, 2 * m_fRadius);
				m_arIndicators.push_back(rc);
				x += indicator_space;
			}
		}
	}

	void Draw(sf::RenderWindow &m_Window, int cur_page_num)
	{
		sf::CircleShape shape(m_fRadius);
		shape.setOutlineThickness(1);

		size_t nPages = m_arIndicators.size();
		for (size_t page = 0; page < nPages; page++)
		{
			if (page == cur_page_num)
			{
				shape.setOutlineColor(m_clrSelected);
				shape.setFillColor(m_clrSelected);
			}
			else
			{
				shape.setOutlineColor(m_clrDeselected);
				shape.setFillColor(m_clrDeselected);
			}

			shape.setPosition(m_arIndicators[page].left, m_arIndicators[page].top);
			m_Window.draw(shape);
		}
	}

	// tests, if the given screen coordinate hits one of the page indicators.
	// returns: the index of the page indicator (zero based), or -1 if no hit
	int HitTest(int screen_x, int screen_y)
	{
		float x = (float)screen_x;
		float y = (float)screen_y;

		size_t nPages = m_arIndicators.size();
		for (size_t page = 0; page < nPages; page++)
		{
			if (m_arIndicators[page].contains(x, y))
				return (int)page;
		}

		return -1;
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//															class CActivityIndicator
// --------------------------------------------------------------------------------------------------------------------------------------------
class CPosition
{
public:
	float x;
	float y;

public:
	CPosition(float xx, float yy)
		:	x(xx),
			y(yy)
	{
	}

	CPosition()
		:	x(0),
			y(0)
	{
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//															class CActivityIndicator
// --------------------------------------------------------------------------------------------------------------------------------------------
#define MIN_ALPHA	0
#define INDICATOR_RADIUS_SHRINK		0.7f

class CActivityIndicator
{
protected:
	CPosition	m_Position;
	float		m_fRadius;
	sf::Color	m_Color;
	bool		m_bVisible;
	bool		m_bWithUnderlay;	// if true, a black rectangle is drawn below the indicator
	RString		m_strTitle;
	int			m_nNumIndicators;	// number of indicators
	float		m_fIndicatorRadius;	// radius of one indicator
	int			m_nAnim;			// animation position (0 .. 1)
	vector<CPosition>	m_arIndicators;	// positions of indicators
	sf::Text	m_Text;
	CPosition	m_TextPos;

public:
	CActivityIndicator()
		:	m_bVisible(false),
			m_nAnim(255)
	{
		m_Text.setCharacterSize(15);
		m_Text.setStyle(sf::Text::Bold);
		m_Text.setColor(sf::Color::White);
	}

	const CPosition &GetPosition() const { return m_Position; }
	float GetRadius() const { return m_fRadius; }
	float GetExtent() const { return 30.f + m_fRadius + m_fIndicatorRadius; } // * INDICATOR_RADIUS_SHRINK; }

	bool GetVisible() const { return m_bVisible; }
	void SetVisible(bool val) { m_bVisible = val; }

	void SetColor(sf::Color color)
	{
		m_Color = color;
	}

	const RString &GetTitle() const { return m_strTitle; }

	void SetTitle(const RString &title)
	{
		m_strTitle = title;

		if (!m_Text.getFont())
			m_Text.setFont(gConfig.m_pFont->GetSfmlFont());

		m_Text.setString(m_strTitle.c_str());
		m_Text.setPosition(0, 0);
		sf::FloatRect rc = m_Text.getGlobalBounds();
		m_Text.setPosition(floor(m_Position.x - rc.width / 2.f), floor(m_Position.y + GetExtent() - rc.height - 5.f));
	}

	void Set(float x, float y, float radius, sf::Color color, bool visible = true, int num_indicators = 8, bool with_underlay = false)
	{
		m_fRadius = radius;
		m_Color = color;
		m_bVisible = visible;
		m_bWithUnderlay = with_underlay;
		m_nNumIndicators = num_indicators;
		m_nAnim = MIN_ALPHA;

		// compute the circumference
		float u = 2.f * (float)M_PI * radius;

		// divide by number of mini-circles
		float d_mini = u / (float)num_indicators;						// diameter per indicator
		m_fIndicatorRadius = d_mini / 2.f * INDICATOR_RADIUS_SHRINK;	// radius: shrink by factor 0.7

		// adjust position: circles in sfml are not drawn at their center, but at their left/top coord
		m_Position = CPosition(x + m_fIndicatorRadius, y + m_fIndicatorRadius);

		m_arIndicators.clear();
		float alpha = (float)M_PI;
		float alpha_add = 2.f * (float)M_PI / (float)num_indicators;
		for (int i = 0; i < num_indicators; i++)
		{
			float y_offset = sin(alpha) * radius;
			float x_offset = cos(alpha) * radius;
			m_arIndicators.push_back(CPosition(x + x_offset, y + y_offset));
			alpha += alpha_add;
		}
	}

	void Draw(sf::RenderWindow &window)
	{
		if (!m_bVisible)
			return;
		
		if (m_bWithUnderlay)
		{
			float extent = GetExtent();
			sf::RectangleShape bkg(sf::Vector2f(extent * 2, extent * 2));
			bkg.setPosition(m_Position.x - extent, m_Position.y - extent);
			bkg.setFillColor(sf::Color(0x20, 0x20, 0x20, 0xa0));
			bkg.setOutlineThickness(0);
			window.draw(bkg);
		}

		if (!m_strTitle.empty())
			window.draw(m_Text);

		sf::CircleShape shape(m_fIndicatorRadius);
		shape.setOutlineThickness(1);
		shape.setOutlineColor(m_Color);

		shape.setFillColor(sf::Color(0xe0, 0xe0, 0xe0));
		for (int i = 0; i < m_nNumIndicators; i++)
		{
			shape.setPosition(m_arIndicators[i].x, m_arIndicators[i].y);
			window.draw(shape);
		}

		sf::Color col = m_Color;
		int anim = m_nAnim;
		int anim_add = 255 / m_nNumIndicators;
		for (int i = m_nNumIndicators - 1; i >= 0 ; i--)
		{
			shape.setPosition(m_arIndicators[i].x, m_arIndicators[i].y);
			col.a = anim;
			shape.setFillColor(col);
			window.draw(shape);
			anim -= anim_add;
			if (anim < MIN_ALPHA)
				anim = 255;
		}

		m_nAnim -= 6;	// this controls the animation speed
		if (m_nAnim < MIN_ALPHA)
			m_nAnim = 255;
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//														TrayNotifyWndProc()
// receives notification messages from the tray icon
// --------------------------------------------------------------------------------------------------------------------------------------------
#define			WM_ICON_NOTIFY		(WM_APP + 10)
CSystemTray		gTrayIcon;
CSfmlApp		*gpNotifyApp;

LRESULT CALLBACK TrayNotifyWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ICON_NOTIFY:
		return gTrayIcon.OnTrayNotification(wParam, lParam);

	case WM_COMMAND:
		if (wParam == ID_MENU_SHOW)
		{
			gpNotifyApp->ShowMainWindow();
			gbEatSingleClick = false;
		}
		else if (wParam == ID_MENU_SUSPEND)
		{
			if (!gbSuspendHooks)
				gbSuspendHooks = true;
			else
				gbSuspendHooks = false;
			gTrayIcon.CheckMenuItem(ID_MENU_SUSPEND, gbSuspendHooks);
		}
		else if (wParam == ID_MENU_HELP)
			ShellExecute(hwnd, _T("open"), _T("http://idealsoftware.com/opensource/places.html"), NULL, NULL, SW_SHOWNORMAL);
		else if (wParam == ID_MENU_EXIT)
			gpNotifyApp->CloseMainWindow();
		break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CreateTrayNotifyWindow()
//
// Native WINAPI window, that receives notification messages from the tray icon.
// --------------------------------------------------------------------------------------------------------------------------------------------
HWND CreateTrayNotifyWindow(CSfmlApp *app)
{
	WNDCLASS    wndclass;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = TrayNotifyWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = ghInstance;
	wndclass.hIcon         = NULL;
	wndclass.hCursor       = NULL;
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = APP_CLASS_NAME;
	RegisterClass(&wndclass);

	HWND hwnd = CreateWindow(APP_CLASS_NAME, APP_CLASS_NAME,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		app->GetRenderWindow().getSystemHandle(), NULL, ghInstance, NULL);

	gpNotifyApp = app;

	return hwnd;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//																GetPath()
// --------------------------------------------------------------------------------------------------------------------------------------------
RString GetPath(const RString &path)
{
	int len = (int)path.length() - 1;
	int i;
	for (i = len; i >= 0 && path[i] != _T('\\'); i--)
		;	// NOP

	if (i >= 0)
		return path.left(i);

	return _T("");
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CMyApp
// --------------------------------------------------------------------------------------------------------------------------------------------
class CMyApp : public CSfmlApp
{
protected:
	enum ELassoMode
	{
		enLassoModeNone,
		enLassoModeSelect,
		enLassoModeDeselect,
	};

	enum EBkgType
	{
		enBkgNone,
		enBkgShader,
		enBkgWallpaper,
		enBkgSolidColor,
	};

	enum EFadeMode
	{
		enFmNoFade,
		enFmFadeIn,
		enFmFadeOut,
	};

public:				// need access by external thread function to avoid delays when calling ShellExecute()
	int				m_nDesktopWidth;
	int				m_nDesktopHeight;
	RString			m_strDatabasePath;
	RString			m_strSettingsPath;
	CPageIndicators	m_PageIndicators;
	CActivityIndicator	m_Activity;
	CActivityIndicator	m_ResyncActivity;
	sf::Vector2i	m_LastMousePos;			// last mouse position (received by MouseMove)
	sf::Vector2i	m_LastMouseOffsetPos;	// last mouse position with added page offset

	EBkgType		m_enBkgType;
	sf::Color		m_BkgColor;				// solid color
	sf::Texture		m_BkgTexture;			// image wallpaper
	sf::Sprite		m_BkgSprite;			// wallpaper sprite
	sf::Shader		m_Shader;				// shader
	float			m_fShaderTime;			// shader time parameter
	float			m_ShaderTimeInc;		// shader speed (0.001 - 0.1, typically 0.01)
	sf::RectangleShape m_BackgroundShape;	// shader shape

	sf::View		m_InitialView;
	sf::View		m_View;
	int				m_nScrollBy;

	CIconManager	m_IconManager;
	CIcon			*m_pHoverIcon;
	int				m_nMoveSpeed;

	bool			m_bIsDragging;
	bool			m_bIsDraggingIcons;
	bool			m_bHasDragged;
	sf::Vector2i	m_DragInitialMouseStart;	// stays constant while dragging
	sf::Vector2i	m_DragMouseStart;			// is modified while dragging
	sf::Clock		m_ClickClock;

	CIconPos		m_posKbdCursor;				// position of keyboard cursor
	CIcon			*m_pKbdCursorIcon;			// icon under keyboard cursor

	sf::Vector2i	m_HoverPos;
	sf::Clock		m_HoverClock;

	bool			m_bStartup;					// true, if the main window is created at startup of app
	bool			m_bFirstTime;				// true, if this app is run the very first time
	bool			m_bWasLeftButtonDown;		// for checks in button-up
	ELassoMode		m_enLassoMode;				// "Lasso" multi-selection box

	HCURSOR			m_hCursorArrow;
	HCURSOR			m_hCursorHand;
	HCURSOR			m_hCursorArrowSelectAdd;
	HCURSOR			m_hCursorArrowSelectSub;
	HCURSOR			m_hCursorArrowAddSub;
	HCURSOR			m_hCursorHandSelectAdd;
	HCURSOR			m_hCursorHandSelectSub;
	HCURSOR			m_hCursorHandAddSub;

	bool			m_bDrawActivityOnly;		// during layout-change and icon re-scaling, only the background and the Activity Indicator are drawn
	HANDLE			m_hActivityThread;
	HANDLE			m_hResyncThread;

	RString			m_strOpenLocPath;			// path retrieved from an icon, for OpenLocation() / ShellExecute()
	RString			m_strOpenLocParams;			// parameters retrieved from an icon, for OpenLocation() / ShellExecute()
	bool			m_bOpenAsAdmin;				// "run as admin" retrieved from an icon, for OpenLocation() / ShellExecute()

	bool			m_bDoFadeInOut;				// fades window in on show window and out on hide window
	EFadeMode		m_enFade;					// 0 no fade, 1 = fade-in, 2=fade-out
	float			m_fFadeValue;				// current alpha value of fade effect
	bool			m_bHideWithDelay;			// hide main window with delay (to make icon animation visible before closing)

	RString			m_strLicense;				// owner of license

	// system related
	HWND			m_hwndProgman;
	HWND			m_hwndDesktopListView;
	CTaskbar		m_Taskbar;
	int				m_nLeft;					// left position of app window
	int				m_nTop;						// top position of app window
	bool			m_bChangeWindowLayout;
	sf::Clock		m_Timer;

public:
	CMyApp()
		:	CSfmlApp(),
			m_enBkgType(enBkgNone),
			m_fShaderTime(0),
			m_IconManager(),
			m_pHoverIcon(nullptr),
			m_nMoveSpeed(8),
			m_bIsDragging(false),
			m_bIsDraggingIcons(false),
			m_bHasDragged(false),
			m_HoverPos(-1 , -1),
			m_nScrollBy(0),
			m_bStartup(true),
			m_bFirstTime(false),
			m_bWasLeftButtonDown(false),
			m_enLassoMode(enLassoModeNone),
			m_bDrawActivityOnly(false),
			m_bDoFadeInOut(true),
			m_enFade(enFmNoFade),
			m_bHideWithDelay(false),
			m_bChangeWindowLayout(false)
	{
		gbHotkeyVKey = 0;		// virtual key code for app activation
		gbHotkeyModifiers = 0;	// modifiers for app activation (shift, ctrl, alt)

		m_hCursorArrow = LoadCursor(NULL, IDC_ARROW);
		m_hCursorHand = LoadCursor(NULL, IDC_HAND);
		m_hCursorArrowSelectAdd = LoadCursor(ghInstance, MAKEINTRESOURCE(ARROW_SELECT_ADD));
		m_hCursorArrowSelectSub = LoadCursor(ghInstance, MAKEINTRESOURCE(ARROW_SELECT_SUB));
		m_hCursorArrowAddSub = LoadCursor(ghInstance, MAKEINTRESOURCE(ARROW_ADD_SUB));
		m_hCursorHandSelectAdd = LoadCursor(ghInstance, MAKEINTRESOURCE(HAND_SELECT_ADD));
		m_hCursorHandSelectSub = LoadCursor(ghInstance, MAKEINTRESOURCE(HAND_SELECT_SUB));
		m_hCursorHandAddSub = LoadCursor(ghInstance, MAKEINTRESOURCE(HAND_ADD_SUB));

		RunHookThread();
		InitIconShader();

		HICON hTrayIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_APP_ICON));
		gTrayIcon.Create(ghInstance, NULL, WM_ICON_NOTIFY, APP_NAME, hTrayIcon, IDR_TRAY_MENU);

		// check, if path exists. otherwise create it.
		m_strDatabasePath = gstrAppData;
		if (!TestWriteAccess(m_strDatabasePath))
			_tmkdir(m_strDatabasePath.c_str());

		m_strSettingsPath = m_strDatabasePath + SETTINGS_FILE;
		m_strDatabasePath += DATA_FILE;

		bool have_settings = TestWriteAccess(m_strSettingsPath);
		bool have_db = TestWriteAccess(m_strDatabasePath);

		if (have_settings && have_db)
		{
			try
			{
				gSettings.ReadFromFile(m_strSettingsPath);
				m_IconManager.ReadFromFile(m_strDatabasePath);
				m_IconManager.GotoPage(gSettings.m_nCurPageNum);
				if (gSettings.m_bFullscreen)
					m_enWindowStyle = enWsFullscreen;
				else
					m_enWindowStyle = enWsWindowed;
			}
			catch (const Exception &e)
			{
				MessageBox(NULL, e.GetExceptionMessage().c_str(), APP_NAME, MB_ICONERROR);
			}
		}
		else
		{
			// show settings dialog
			MessageBox(NULL,
				_T("This seems to be the first time you are running \"") APP_NAME _T("\".\n\n")
				_T("For a tutorial video, please search in Youtube for:\n")
				_T("\"Windows Launcher Places\"\n\n")
				_T("To configure the icon layout (number of columns and rows),\n")
				_T("activation keys and background wallpaper, please right-click\n")
				_T("onto the blank area and choose \"Settings\".")
				, APP_NAME, MB_ICONINFORMATION);

			m_bFirstTime = true;
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::~CMyApp()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	~CMyApp()
	{
		DeInitIconShader();
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::CreateDefaultIcons()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void CreateDefaultIcons()
	{
		int max_rows = min(7, gSettings.m_nRows);	// best logical structured layout, if 7 icons per column
		int col = 0;
		int row = 0;
		int i = 0;
		bool german = (GetUserDefaultUILanguage() & 0x1ff) == LANG_GERMAN;
		while (gDefaultIcons[i].TitleEn)
		{
			// depending on system locale, use German or English for the titles
			LPCTSTR title;
			if (german)
				title = gDefaultIcons[i].TitleDe;
			else
				title = gDefaultIcons[i].TitleEn;

			// create and add icon
			int x = (int)m_IconManager.Col2Screen(col);
			int y = (int)m_IconManager.Row2Screen(row);
			CIcon *icon = m_IconManager.CreateIcon(gDefaultIcons[i].Path, title, x, y);

			// set parameters
			icon->m_strParameters = gDefaultIcons[i].Params;

			// build full path for icon bitmap
			RString icon_path = gstrCommonAppData + _T("icons\\") + gDefaultIcons[i].Icon;
			icon->SetIconPath(m_IconManager.GetBitmapCache(), icon_path);
			icon->SetState(CIcon::enStateNormal);

			i++;
			row++;
			if (row >= max_rows)
			{
				row = 0;
				col++;
			}
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::DesktopResolutionChanged()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	bool DesktopResolutionChanged()
	{
		sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

		return m_nDesktopWidth != desktop.width || m_nDesktopHeight != desktop.height;
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::ComputeLayout()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void FinalizeLayout()
	{
		if (m_Window.getSystemHandle())
		{

			// position window
			if (m_enWindowStyle == enWsFullscreen)
				MoveWindow(m_Window.getSystemHandle(), m_nLeft, m_nTop, gConfig.m_nWindowWidth, gConfig.m_nWindowHeight, true);
		}
	}


	void ComputeLayout()
	{
		// setup global config
		sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

		m_nDesktopWidth = desktop.width;
		m_nDesktopHeight = desktop.height;

		int width = desktop.width;
		int height = desktop.height;
		m_nTop = 0;
		m_nLeft = 0;
		if (m_enWindowStyle == enWsWindowed)
		{
			width = width * 2 / 3;
			height = height * 2 / 3;
		}
		else
		{
			/* this flickers
			// because the taskbar can change location and dimensions (through the user dragging it)
			// in between calls to show the Places window, we prefer to use fullscreen mode
			style = sf::Style::Fullscreen; */

			// make window fit in conjuction with taskbar
			// if taskbar is not covering full height, it is docked at top or bottom of screen
			// NOTE: if the taskbar is hidden, the height of the bar is still as if unhidden, so we can not 
			//       use the width or height members of the CTaskbar class.
			if (height != m_Taskbar.m_nHeight)
			{
				// taskbar is top or bottom attached
				if (m_Taskbar.m_rcPosition.top <= 0)
				{
					m_nTop = m_Taskbar.m_rcPosition.bottom;
					height -= m_Taskbar.m_rcPosition.bottom;
				}
				else
					height -= height - m_Taskbar.m_rcPosition.top;
			}

			// if taskbar is not covering full width, it is docked at left or right of screen
			if (width != m_Taskbar.m_nWidth)
			{
				// taskbar is left or right attached
				if (m_Taskbar.m_rcPosition.left <= 0)
				{
					m_nLeft = m_Taskbar.m_rcPosition.right;
					width -= m_Taskbar.m_rcPosition.right;
				}
				else
					width -= width - m_Taskbar.m_rcPosition.left;
			}
		}

		// compute layout
		float old_w = (float)gConfig.m_nWindowWidth;
		int old_icon_size_x = gConfig.m_nIconSizeX;
		int old_icon_offset_x = gConfig.m_nIconOffsetX;
		int old_icon_offset_y = gConfig.m_nIconOffsetY;

		gConfig.Setup(width, height);	// sets gConfig.m_nWindowWidth / m_nWindowHeight
		gConfig.ComputeLayout(gSettings);

		if (m_posKbdCursor.m_RasterPos.m_nCol >= gSettings.m_nColumns)
			m_posKbdCursor.m_RasterPos.m_nCol = gSettings.m_nColumns - 1;

		if (m_posKbdCursor.m_RasterPos.m_nRow >= gSettings.m_nRows)
			m_posKbdCursor.m_RasterPos.m_nRow = gSettings.m_nRows - 1;

		// setup initial view, used for drawing the background and other static parts (page indicators, activity indicators)
		float w = (float)gConfig.m_nWindowWidth;	// m_Window.getSize().x;
		float h = (float)gConfig.m_nWindowHeight;	// m_Window.getSize().y;
		m_InitialView.setSize(w, h);
		m_InitialView.setCenter(w / 2.f, h / 2.f);
		m_Window.setView(m_InitialView);

		// this is the view used for scrolling
		m_View.setSize(w, h);
		sf::Vector2f center = m_View.getCenter();
		if (old_w != w || m_bStartup)
		{
			m_nScrollBy = 0;	// stop scrolling
			m_View.setCenter(m_IconManager.GetCurrentPageNum() * w + w / 2.f, h / 2.f);
		}
		else
		{
			// avoid reset of view's x-position, if width didn't change
			m_View.setCenter(center.x, h / 2.f);
		}

		// setup background
		SetupBackground();

		// create scaled font
		if (!gConfig.m_pFont)
		{
			gConfig.m_pFont = new CSfmlMemoryFont();
			gConfig.m_pFont->Create(m_Window, _T("Segoe UI"));		// will fallback to Arial, if not present
		}

		// Create a sf::Text object which uses our font
		if (!gConfig.m_pText)
		{
			gConfig.m_pText = new sf::Text();
			gConfig.m_pText->setFont(gConfig.m_pFont->GetSfmlFont());
			gConfig.m_pText->setColor(sf::Color::White);
			gConfig.m_pText->setStyle(sf::Text::Regular);		// sf::Text::Bold         sf::Text::Regular
		}

		int font_size = gConfig.m_nIconFontSize;
		if (font_size < 6)
			font_size = 6;
		gConfig.m_pText->setCharacterSize(font_size);

		m_IconManager.Init(gConfig, false);
		if (m_IconManager.GetPageCount() == 0)
		{
			// create 10 empty pages
			for (int i = 0; i < 10; i++)
				m_IconManager.AddNewPage();

			// create default icons
			CreateDefaultIcons();
		}

		if (old_icon_size_x != gConfig.m_nIconSizeX ||
			old_icon_offset_x != gConfig.m_nIconOffsetX ||
			old_icon_offset_y != gConfig.m_nIconOffsetY)
		{
			m_bDrawActivityOnly = true;		// triggers m_IconManager.RecomputeAllIcons();
		}

		// compute the page indicators (circles at the bottom)
		m_PageIndicators.Compute(m_IconManager.m_IconPages.size());

		// setup activity indicators
		m_Activity.Set((float)width / 2, (float)height / 3, 50.f, sf::Color(0xf0, 0x50, 0x01), false, 16, true);
		m_Activity.SetTitle(_T("Saving Database"));
		m_ResyncActivity.Set(30.f, (float)(height - 40), 10.f, sf::Color(2, 0x6a, 0x45), false);

		FinalizeLayout();
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::CreateMainWindow()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void CreateMainWindow() override
	{
		// system related
		m_hwndProgman = FindWindow(_T("Progman"), 0);
		m_hwndDesktopListView = GetDesktopListViewHWND();

		// Compute Layout, Create Font
		ComputeLayout();

		// setup main window
		unsigned int style = sf::Style::None;
		if (m_enWindowStyle == enWsWindowed)
			style = sf::Style::Titlebar; // +sf::Style::Close;
		sf::ContextSettings context;
		context.antialiasingLevel = 4;
		m_Window.create(sf::VideoMode(gConfig.m_nWindowWidth, gConfig.m_nWindowHeight), APP_NAME, style, context);

		m_Window.setIcon(app_icon.width, app_icon.height, app_icon.pixel_data); 
		SetHookAppWindow(m_Window.getSystemHandle());
		SetHookInstance(ghInstance);

		if (m_bStartup && !m_bFirstTime)
		{
			ShowWindow(m_Window.getSystemHandle(), SW_HIDE);
			m_bFirstTime = false;
		}

		DragAcceptFiles(m_Window.getSystemHandle(), TRUE);

		// Send all menu messages to MainWindow
		gTrayIcon.SetTargetWnd(CreateTrayNotifyWindow(this));

		FinalizeLayout();

		// HINT: removing the call to FramerateLimit, enabling vsync, adjusting the timing using Sleep() and setting 
		// "Threaded Optimization = Off" in the nVidia driver panel finally did the trick.
		// (whatever Threaded Optimization means... found it using google)
		//	m_Window.setFramerateLimit(60);
		m_Window.setFramerateLimit(0);
		m_Window.setVerticalSyncEnabled(true);

		m_bStartup = false;
		CheckNagScreen();
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::CheckNagScreen()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void CheckNagScreen()
	{
#if 0
		SYSTEMTIME time;
		
		GetSystemTime(&time);

		// don't make it too easy, scramble a bit (so year constant can not be found in hex editor)
		time.wYear -= 2000;

		// run until 2019/01/01
		if (time.wYear >= 19 && time.wMonth >= 1)
		{
			MessageBox(m_Window.getSystemHandle(),
				_T("The beta period for \"Places\" has expired.\n\n")
				_T("Please check at www.idealsoftware.com, if a newer version is available.\n\n"),
				APP_NAME, MB_ICONERROR);
			exit(1);
		}
#endif
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::SetTransparency()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void SetTransparency(float alpha)
	{
		BYTE balpha = (BYTE)(alpha * 255.f);
		HWND hWnd = m_Window.getSystemHandle();

		if (m_enFade == enFmFadeIn)
		{
			// fade-in
			if (alpha == 0.f)
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
			else if (alpha == 1.f)
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);

			SetLayeredWindowAttributes(hWnd, 0, balpha, LWA_ALPHA);
		}
		else if (m_enFade == enFmFadeOut)
		{
			// fade-out
			if (alpha == 0.f)
			{
				ShowWindow(m_Window.getSystemHandle(), SW_HIDE);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);

				if (m_pHoverIcon)
				{
					m_IconManager.SetIconState(m_pHoverIcon, CIcon::enStateNormal, true);
					m_pHoverIcon = nullptr;
					m_LastMousePos = { 0, 0 };
				}
			}
			else if (alpha == 1.f)
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);

			SetLayeredWindowAttributes(hWnd, 0, balpha, LWA_ALPHA);
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::ShowMainWindow()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void ShowMainWindow() override
	{
		if (m_Taskbar.ChangedPosition())
		{
			// the taskbar changed its position, we need to re-layout
			ComputeLayout();
		}

		// SetForegroundWindow(m_Window.getSystemHandle());

		if (m_bDoFadeInOut)
		{
			m_enFade = enFmFadeIn;
			m_fFadeValue = 0.f;
			SetTransparency(m_fFadeValue);
		}

		ShowWindow(m_Window.getSystemHandle(), SW_SHOW);
		SetForegroundWindowInternal(m_Window.getSystemHandle());

		// synthesize MouseMove event, so mouse cursor is set correctly
		sf::Event event;
		sf::Vector2i pos = sf::Mouse::getPosition(m_Window);
		event.mouseMove.x = pos.x;
		event.mouseMove.y = pos.y;
		OnMouseMove(event);
		CheckNagScreen();
	}


	void HideMainWindow()
	{
		if (m_bDoFadeInOut)
		{
			m_enFade = enFmFadeOut;
			m_fFadeValue = 1.f;
		}
		else
			ShowWindow(m_Window.getSystemHandle(), SW_HIDE);
	}

	void HideMainWindowWithDelay()
	{
		m_bHideWithDelay = true;
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::SetupBackground()
	// sets the desktop shader / wallpaper
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void SetupBackground()
	{
		m_enBkgType = enBkgNone;
		m_BkgColor = sf::Color(0);	// black

		if (!gSettings.m_strShaderFile.empty() && sf::Shader::isAvailable())
		{
			// create background shader
			m_enBkgType = enBkgShader;
			sf::String s(gSettings.m_strShaderFile.c_str());
			if (!m_Shader.loadFromFile(s, sf::Shader::Fragment))
			{
				// error...
				m_enBkgType = enBkgNone;
				MessageBox(m_Window.getSystemHandle(), _T("Error loading shader!"), APP_NAME, MB_ICONERROR);
			}
			m_ShaderTimeInc = (float)gSettings.m_nShaderSpeed / 1000.f;	// "animation speed" of shader, should be between 0.001 and 0.01
			m_Shader.setParameter("resolution", sf::Vector2f((float)gConfig.m_nWindowWidth, (float)gConfig.m_nWindowHeight));
			m_Shader.setParameter("time", m_fShaderTime);
			m_Shader.setParameter("mouse", sf::Vector2f((float)0, (float)0));
			m_BackgroundShape.setSize(sf::Vector2f((float)gConfig.m_nWindowWidth, (float)gConfig.m_nWindowHeight));
		}
		
		if (m_enBkgType == enBkgNone && !gSettings.m_strImageFile.empty())
		{
			// create background pattern
			m_enBkgType = enBkgWallpaper;

			sf::String s(gSettings.m_strImageFile.c_str());
			if (!m_BkgTexture.loadFromFile(s))
			{
				// error...
				m_enBkgType = enBkgNone;
				MessageBox(m_Window.getSystemHandle(), _T("Error loading image!"), APP_NAME, MB_ICONERROR);
			}
			else
			{
				m_BkgSprite.setTexture(m_BkgTexture, true);

				if (gSettings.m_nModifyImage == 2)	// scale image
				{
					float xfactor = (float)gConfig.m_nWindowWidth / m_BkgTexture.getSize().x;
					float yfactor = (float)gConfig.m_nWindowHeight/ m_BkgTexture.getSize().y;
					m_BkgSprite.setScale(xfactor, yfactor);
					m_BkgSprite.setPosition(0, 0);
				}
				else
				{
					m_BkgSprite.setScale(1.f, 1.f);
					m_BkgSprite.setPosition(((float)gConfig.m_nWindowWidth - m_BkgTexture.getSize().x) / 2.f, ((float)gConfig.m_nWindowHeight - m_BkgTexture.getSize().y) / 2.f);
				}
			}
		}
		
		if (m_enBkgType == enBkgNone)
		{
			m_enBkgType = enBkgSolidColor;
			BYTE r = GetRValue(gSettings.m_clrBkgColor);
			BYTE g = GetGValue(gSettings.m_clrBkgColor);
			BYTE b = GetBValue(gSettings.m_clrBkgColor);
			m_BkgColor = sf::Color(r, g, b);
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::ControlKeyPressed()
	// run thread to keep view alive
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void RunActivityThread(bool with_activity_indicator = true)
	{
		unsigned aID;
		gbTerminateActivityThread = false;
		m_Window.setActive(false);	// shift GL context to thread (calling m_Window.display() within the thread calls m_Window.setActive(true))
		m_Activity.SetVisible(with_activity_indicator);
		m_hActivityThread = (HANDLE)_beginthreadex(NULL, 0, ActivityThread, this, 0, &aID);
	}

	void TerminateActivityThread()
	{
		gbTerminateActivityThread = true;
		WaitForSingleObject(m_hActivityThread, INFINITE);
		m_Activity.SetVisible(false);
		m_Window.setActive(true);	// shift GL context back to this main thread
	}

	void TerminateResyncThread()
	{
		if (m_IconManager.m_bResyncRunning)
		{
			m_IconManager.m_bResyncRunning = false;
			WaitForSingleObject(m_hResyncThread, INFINITE);
			m_ResyncActivity.SetVisible(false);
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::SaveSettings()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void SaveSettings()
	{
		try
		{
			gSettings.WriteToFile(m_strSettingsPath, m_IconManager.GetCurrentPageNum(), m_enWindowStyle == enWsFullscreen);
		}
		catch (const Exception &e)
		{
			MessageBox(m_Window.getSystemHandle(), e.GetExceptionMessage().c_str(), APP_NAME, MB_ICONERROR);
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::SaveIconDatabase()
	//
	// this does intentionally NOT clear the undo-stack, so database files
	// written here may be larger than when they are written as the app terminates.
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void SaveIconDatabase()
	{
		// interrupt ResyncThread, if running
		TerminateResyncThread();

		// run thread to keep view alive, so activity indicator is shown
		RunActivityThread();

		try
		{
			m_IconManager.WriteToFile(m_strDatabasePath);
		}
		catch (const Exception &e)
		{
			m_Activity.SetVisible(false);
			gbTerminateActivityThread = true;
			MessageBox(m_Window.getSystemHandle(), e.GetExceptionMessage().c_str(), APP_NAME, MB_ICONERROR);
		}

		TerminateActivityThread();

		// reset flags
		m_IconManager.m_bChangedAnIcon = false;
		m_IconManager.m_bChangedLayout = false;
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::OpenLocation()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void RetrieveOpenLocation(CIcon *icon)
	{
		RString path;

		if (icon->m_strFilePath == _T("<explorer>"))
			path = gstrWindir + _T("explorer.exe");
		else if (icon->m_strFilePath == _T("<control>"))
			path = gstrWindir + _T("system32\\control.exe");
		else if (icon->m_strFilePath == _T("<mmc>"))
			path = gstrWindir + _T("system32\\mmc.exe");
		else
			path = icon->m_strFilePath;

		m_strOpenLocPath = path;
		m_strOpenLocParams = icon->m_strParameters;
		m_bOpenAsAdmin = icon->m_bOpenAsAdmin;
	}


	void OpenLocation()
	{
		if (!m_strOpenLocPath.empty())
		{
			// When calling ShellExecute(), Windows causes a huge delay, either because a virus scanner starts scanning the executable, or due
			// to network access by the domain controller. Therefore we call ShellExecute() from a thread.
			unsigned aID;
			_beginthreadex(NULL, 0, ShellExecuteThread, this, 0, &aID);
		}
	}


	void OpenIcon()
	{
		CIcon *icon = m_IconManager.GetSelectedIcons().front();
		m_IconManager.SetIconState(icon, CIcon::enStateOpen);
		if (gSettings.m_bPlayClickSound)
			PlaySound(_T("WAVE_CLICK"), ghInstance, SND_RESOURCE | SND_ASYNC); 
		HideMainWindowWithDelay();
		RetrieveOpenLocation(icon);		// OpenLocation() is called, when the icon animation has finished
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::OnClose()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	bool OnClose() override
	{
		TerminateResyncThread();

		SaveSettings();		// save always, so current page number is saved

		// write the database, if anything has been modified.
		// if CanUndo() is true, something was changed.
		if (m_IconManager.CanUndo() || m_IconManager.m_bChangedAnIcon || 
			m_IconManager.m_bChangedLayout || m_IconManager.m_nDbIconSize != gConfig.m_nIconSizeX)
		{
			// clear undo- / redo-stack, so unused bitmaps are de-referenced and not written to file
			m_IconManager.ClearUndoStack();
			SaveIconDatabase();
		}
		return true;
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::ControlKeyPressed()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	bool ControlKeyPressed() const
	{
		return sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::IsArrowKey()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	bool IsArrowKey(sf::Keyboard::Key code) const
	{
		return code == sf::Keyboard::Left || code == sf::Keyboard::Right || code == sf::Keyboard::Up || code == sf::Keyboard::Down;
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::GotoPage()
	// sets also the view
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void GotoPage(int page_num)
	{
		int old_page_num = m_IconManager.GetCurrentPageNum();

		if (page_num < 0)
			page_num = 0;
		else if (page_num >= m_IconManager.GetPageCount())
			page_num = m_IconManager.GetPageCount() - 1;

		if (page_num == old_page_num)
			return;
		m_IconManager.GotoPage(page_num);

		float w = (float)m_Window.getSize().x;
		float h = (float)m_Window.getSize().y;
		
		m_View.setCenter((float)page_num * w + w / 2.f, h / 2.f);

		// synthesize mouse move event, so *everything* is updated correctly, especially regarding currently dragged icons
		sf::Event event;
		event.mouseMove.x = (int)m_LastMousePos.x;
		event.mouseMove.y = (int)m_LastMousePos.y;
		OnMouseMove(event);
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::DeleteIcons()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void DeleteIcons(bool with_undo = true)
	{
		if (m_bIsDragging)		// do not delete icons which are currently dragged
			return;

		m_IconManager.CreateSelectedSnapshot();

		CIconUndoGroup *undo_group = nullptr;
		if (with_undo)
			undo_group = new CIconUndoGroup();

		for (auto it : m_IconManager.GetSelectedIcons())
		{
			if (with_undo)
				undo_group->Add(new CIconDeleteAction(it));
			m_IconManager.UnlinkIcon(it);
		}

		if (with_undo)
			m_IconManager.PushUndoAction(undo_group);

		m_IconManager.GetSelectedIcons().clear();	// we can not call ClearSelectedSnapshot() here, because the icons have been deleted
													// and ClearSelectedSnapshot() tries to access the icons (to set their state to "normal")
		m_pHoverIcon = nullptr;
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::DoMainContextMenu()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void DoMainContextMenu(int x, int y)
	{
		CPopupMenuItem *item;

		const TCHAR *menuFullscreenMode = _T("Fullscreen Mode");
		const TCHAR *menuWindowedMode = _T("Windowed Mode");
		const TCHAR *menuNewIcon = _T("New Icon");
		const TCHAR *menuUndo = _T("Undo");
		const TCHAR *menuRedo = _T("Redo");
		const TCHAR *menuSave = _T("Save");
		const TCHAR *menuReloadIcons = _T("Reload Icon Bitmaps");
		const TCHAR *menuSettings = _T("Settings");
		const TCHAR *menuHelp = _T("Help");
		const TCHAR *menuAbout = _T("About");
		const TCHAR *menuExit = _T("Exit");

		CPopupMenu menu(_T("menu"));

		if (m_enWindowStyle == enWsWindowed)
			menu.AddItem(menuFullscreenMode);
		else
			menu.AddItem(menuWindowedMode);

		// check, if it is possible to create a new icon at this location
		item = menu.AddItem(menuNewIcon);
		int virt_x = x + m_IconManager.GetCurrentPageNum() * gConfig.m_nWindowWidth;
		if (!m_IconManager.CanPlaceIconOnCurrentPage(virt_x, y))
			item->SetEnabled(false);

		menu.AddSeparator();

		item = menu.AddItem(menuUndo);
		if (!m_IconManager.CanUndo())
			item->SetEnabled(false);

		item = menu.AddItem(menuRedo);
		if (!m_IconManager.CanRedo())
			item->SetEnabled(false);
		menu.AddSeparator();

		menu.AddItem(menuSave);
		menu.AddItem(menuReloadIcons);
		menu.AddSeparator();
		menu.AddItem(menuSettings);
		menu.AddItem(menuHelp);
		menu.AddItem(menuAbout);
		menu.AddSeparator();
		menu.AddItem(menuExit);

		HWND hwnd = m_Window.getSystemHandle();
		if (menu.DoPopupMenu(hwnd, x, y))
		{
			if (menu.GetSelectedEntry() == menuWindowedMode)
			{
				m_bRecreateWindow = true;
				m_enWindowStyle = enWsWindowed;
				m_Window.close();
			}
			else if (menu.GetSelectedEntry() == menuFullscreenMode)
			{
				m_bRecreateWindow = true;
				m_enWindowStyle = enWsFullscreen;
				m_Window.close();
			}
			else if (menu.GetSelectedEntry() == menuNewIcon)
			{
				RString strEmpty;
				
				m_IconManager.ClearSelectedSnapshot();
				CIconUndoGroup *undo_group = new CIconUndoGroup();
				CIcon *icon = m_IconManager.CreateIcon(strEmpty, _T("New Icon"), virt_x, y);
				m_IconManager.CreateSelectedSnapshot();
				if (IconPropertyDialog(false))
				{
					undo_group->Add(new CIconInsertAction(icon));
					m_IconManager.PushUndoAction(undo_group);
				}
				else
					DeleteIcons(false);
			}
			else if (menu.GetSelectedEntry() == menuUndo)
			{
				m_IconManager.Undo();
			}
			else if (menu.GetSelectedEntry() == menuRedo)
			{
				m_IconManager.Redo();
			}
			else if (menu.GetSelectedEntry() == menuSave)
			{
				SaveSettings();
				SaveIconDatabase();
			}
			else if (menu.GetSelectedEntry() == menuReloadIcons)
			{
				unsigned aID;
				m_hResyncThread = (HANDLE)_beginthreadex(NULL, 0, ResyncThread, &m_IconManager, 0, &aID);
				m_ResyncActivity.SetVisible(true);
			}
			else if (menu.GetSelectedEntry() == menuSettings)
			{
				CSettings old(gSettings);
				if (DialogBox(ghInstance, MAKEINTRESOURCE(IDD_SETTINGS), m_Window.getSystemHandle(), reinterpret_cast<DLGPROC>(DlgSettings)) != 1)
					return;

				if (old.BkgChanged(gSettings))
					SetupBackground();

				if (old.LayoutChanged(gSettings))
				{
					if (!m_IconManager.CanChangeLayout(gSettings))
					{
						// restore old layout
						gSettings.CopyLayout(old);
						MessageBox(m_Window.getSystemHandle(),
							_T("Your edits of rows / columns were not applied.\n\n")
							_T("Cause:\n")
							_T("You tried to reduce the number of columns and / or rows, but ")
							_T("there are icons, which occupy positions that would fall outside ")
							_T("the new layout.\n\n")
							_T("Solution:\n")
							_T("Before changing the layout, please re-arrange the icons, so that ")
							_T("the desired layout can be applied without affecting any icons."),
							APP_NAME, MB_ICONWARNING);
					}
					else
					{
						m_bDrawActivityOnly = true;		//  redraw clashes with internal layout-change!
						RString old_title = m_Activity.GetTitle();
						m_Activity.SetTitle(_T("Scaling Icons"));
						RunActivityThread();

						// we cannot keep the undo stack, this would cause inconsistencies.
						// we either would need to implement an Undo Action, or we clear the stack.
						m_IconManager.ClearUndoStack();
						ComputeLayout();
						m_IconManager.ChangeLayout(gSettings);
						TerminateActivityThread();
						m_Activity.SetTitle(old_title);
						m_bDrawActivityOnly = false;
					}
				}
				else
				{
					// if above code was executed, titles where rendered, too.
					// so title rendering is only executed here, if the layout was not changed
					gConfig.SetFontSize(gSettings.m_nIconFontSize);
					m_IconManager.RenderTitles();
				}

				SaveSettings();
			}
			else if (menu.GetSelectedEntry() == menuHelp)
			{
				ShellExecute(m_Window.getSystemHandle(), _T("open"), _T("http://idealsoftware.com/opensource/places.html"), NULL, NULL, SW_SHOWNORMAL);
			}
			else if (menu.GetSelectedEntry() == menuAbout)
			{
				DialogBox(ghInstance, MAKEINTRESOURCE(IDD_ABOUT), m_Window.getSystemHandle(), reinterpret_cast<DLGPROC>(DlgAbout));
			}
			else if (menu.GetSelectedEntry() == menuExit)
			{
				CloseMainWindow();
			}
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::IconPropertyDialog()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	bool IconPropertyDialog(bool with_undo = true)
	{
		if (m_IconManager.GetSelectedIcons().size() == 1)
		{
			CIcon *icon = m_IconManager.GetSelectedIcons().front();
			gstrIconTitle = icon->GetFullTitle();
			gstrFilePath = icon->GetFilePath();
			gstrParameters = icon->m_strParameters;
			gstrIconPath = icon->GetIconPath();
			gbOpenAsAdmin = icon->m_bOpenAsAdmin;
			if (DialogBox(ghInstance, MAKEINTRESOURCE(IDD_ICON_PROPERTIES), m_Window.getSystemHandle(), reinterpret_cast<DLGPROC>(DlgIconProperties)) != 1)
				return false;

			// only for .exe "open as admin" is allowed
			if (gstrFilePath.right(4).CompareNoCase(_T(".exe")) != 0)
				gbOpenAsAdmin = false;

			// anything changed?
			if (gstrIconTitle == icon->GetFullTitle() &&
				gstrFilePath == icon->GetFilePath() &&
				gstrParameters == icon->m_strParameters &&
				gstrIconPath == icon->GetIconPath() &&
				gbOpenAsAdmin == icon->m_bOpenAsAdmin)
			{
				// no changes at all, do not create an undo action
				return false;
			}

			CIconPropertiesAction *undo;
			if (with_undo)
				undo = new CIconPropertiesAction(icon);
			
			icon->SetFullTitle(gstrIconTitle);
			icon->SetIconPath(m_IconManager.GetBitmapCache(), gstrIconPath);	// icon path always first! (see SetFilePath() comment)
			icon->SetFilePath(m_IconManager.GetBitmapCache(), gstrFilePath);
			icon->m_strParameters = gstrParameters;
			icon->m_bOpenAsAdmin = gbOpenAsAdmin;
	
			if (with_undo)
				m_IconManager.PushUndoAction(undo);

			return true;
		}

		return false;
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::DoIconContextMenu()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void DoIconContextMenu(int x, int y)
	{
		CPopupMenu menu(_T("Main"));

		const TCHAR *menuIconProperties = _T("Properties");
		const TCHAR *menuRunAsAdmin = _T("Run As Administrator");
		const TCHAR *menuRemove = _T("Remove");

		CIcon *icon = m_IconManager.GetSelectedIcons().front();

		if (m_IconManager.GetSelectedIcons().size() == 1)
		{
			menu.AddItem(menuIconProperties);

			// option "run as admin" only available for executables, not for locations
			if (icon->GetFilePath().right(4).CompareNoCase(_T(".exe")) == 0)
				menu.AddItem(menuRunAsAdmin);

			menu.AddSeparator();
		}
		menu.AddItem(menuRemove);

		HWND hwnd = m_Window.getSystemHandle();
		if (menu.DoPopupMenu(hwnd, x, y))
		{
			if (menu.GetSelectedEntry() == menuRemove)
			{
				DeleteIcons();
			}
			else if (menu.GetSelectedEntry() == menuIconProperties)
			{
				IconPropertyDialog();
			}
			else if (menu.GetSelectedEntry() == menuRunAsAdmin)
			{
				m_bOpenAsAdmin = true;
				HideMainWindowWithDelay();
				RetrieveOpenLocation(icon);		// OpenLocation() is called, when the icon animation has finished
			}
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::SetMouseCursor()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void SetMouseCursor()
	{
		// hovering above an icon?
		int mouse_x = m_LastMousePos.x + (int)m_View.getCenter().x - gConfig.m_nWindowWidth / 2;	// m_IconManager.GetCurrentPageNum() * gConfig.m_nWindowWidth;
		int mouse_y = m_LastMousePos.y;

		CIcon *icon = m_IconManager.FindIconUnderMouse(mouse_x, mouse_y);

		/*	This code does not work, when the window is made visible, because SMFL processes WM_SETCURSOR and sets the cursor to ARROW.

			With SMFL v2.5 we can do the following:

			sf::Cursor cursor;
			if (m_nScrollBy == 0 && (icon || m_bIsDraggingIcons))
				cursor.loadFromSystem(sf::Cursor::Hand);
			else
				cursor.loadFromSystem(sf::Cursor::Arrow);
			m_Window.setMouseCursor(cursor); */
		if (m_enLassoMode == enLassoModeSelect)
			SetCursor(icon ? m_hCursorHandSelectAdd : m_hCursorArrowSelectAdd);
		else if (m_enLassoMode == enLassoModeDeselect)
			SetCursor(icon ? m_hCursorHandSelectSub : m_hCursorArrowSelectSub);
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))
		{
			// alt-key = Lasso
			if (ControlKeyPressed())
				SetCursor(icon ? m_hCursorHandSelectSub : m_hCursorArrowSelectSub);	// alt + ctrl = Lasso in de-selection mode
			else
				SetCursor(icon ? m_hCursorHandSelectAdd : m_hCursorArrowSelectAdd);
		}
		else if (ControlKeyPressed())
			SetCursor(icon ? m_hCursorHandAddSub : m_hCursorArrowAddSub);
		else if (m_nScrollBy == 0 && (icon || m_bIsDraggingIcons))
			SetCursor(m_hCursorHand);
		else
			SetCursor(m_hCursorArrow);

		if (icon)
		{
			// we found an icon under the mouse
			// is the icon under the mouse different from the previously hovered icon?
			if (m_pHoverIcon && icon != m_pHoverIcon)
			{
				// yes, set the state of the previously hovered icon back to normal, if its
				// current state is enStateHover (could have been changed to another state while hovering)
				if (m_pHoverIcon->GetState() == CIcon::enStateHover)
					m_IconManager.SetIconState(m_pHoverIcon, CIcon::enStateNormal);
				m_pHoverIcon = nullptr;
			}

			if (icon->GetState() == CIcon::enStateNormal)
			{
				m_pHoverIcon = icon;
				m_IconManager.SetIconState(m_pHoverIcon, CIcon::enStateHover);
			}
		}
		else if (m_pHoverIcon)
		{
			// yes, set the state of the previously hovered icon back to normal, if its
			// current state is enStateHover (could have been changed to another state while hovering)
			if (m_pHoverIcon->GetState() == CIcon::enStateHover)
				m_IconManager.SetIconState(m_pHoverIcon, CIcon::enStateNormal);
			m_pHoverIcon = nullptr;
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::OnMouseDown()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void OnMouseDown(const sf::Event &event) override
	{
		m_bHasDragged = false;

		if (gbEatSingleClick)
			return;

		// do not handle mouse clicks while scrolling
		if (m_nScrollBy != 0)
			return;

		int mouse_x = event.mouseButton.x + m_IconManager.GetCurrentPageNum() * gConfig.m_nWindowWidth;
		int mouse_y = event.mouseButton.y;

		if (event.mouseButton.button == sf::Mouse::Left)
		{
			m_bWasLeftButtonDown = true;
			m_enLassoMode = enLassoModeNone;
			//SetFocus(m_Window.getSystemHandle());
			
			m_DragMouseStart.x = mouse_x;
			m_DragMouseStart.y = mouse_y;
			m_DragInitialMouseStart = m_DragMouseStart;

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))
			{
				// alt-key = Lasso
				if (ControlKeyPressed())
				{
					// alt + ctrl = Lasso in de-selection mode
					m_enLassoMode = enLassoModeDeselect;
				}
				else
				{
					/* too complicated to use
					if (!(sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)))
					{
						// not alt + shift: clear current selection
						m_IconManager.ClearSelectedSnapshot();
						m_IconManager.SelectAll(false);
					}*/

					m_enLassoMode = enLassoModeSelect;
				}

				m_IconManager.StartLassoMode(m_enLassoMode == enLassoModeSelect);
			}
			else if (ControlKeyPressed())
			{
				// ctrl = single icon select / deselect
				CIcon *icon = m_IconManager.FindIconUnderMouse(mouse_x, mouse_y);
				if (icon)
				{
					if (icon->GetState() != CIcon::enStateHighlight)
					{
						m_IconManager.SetIconState(icon, CIcon::enStateHighlight);
						m_IconManager.DeSelectAllOnOtherPages();	// calls CreateSelectedSnapshot()
					}
					else
						m_IconManager.SetIconState(icon, CIcon::enStateNormal);
				}
				else
				{
					// ctrl + click into free desktop space: deselect all selected icons
					m_IconManager.ClearSelectedSnapshot();
				}
			}
			else
			{
				// clicked on an icon?
				m_ClickClock.restart();
				m_bIsDragging = true;
				CIcon *icon = m_IconManager.FindIconUnderMouse(mouse_x, mouse_y);
				if (icon)
				{
					// yes
					m_bIsDraggingIcons = true;
					if (icon->GetState() != CIcon::enStateHighlight)
					{
						// an unselected icon has been clicked
						// de-select all other icons in this case
						m_IconManager.SelectAll(false);
						m_IconManager.DeSelectAllOnOtherPages();
						m_IconManager.ClearSelectedSnapshot();
						m_IconManager.SetIconState(icon, CIcon::enStateHighlight);
					}
					m_IconManager.CreateSelectedSnapshot();							// create snapshot of all selected icons
					m_IconManager.SetSelectedIconsState(CIcon::enStateDragging);	// change the state of all selected icons
					m_IconManager.UnlinkSelectedIcons();
					m_pHoverIcon = nullptr;

					// keyboard cursor
					m_pKbdCursorIcon = icon;
				}
			}
		}

		SetMouseCursor();
	}


	void ScrollPageLeft()
	{
		int width_2 = gConfig.m_nWindowWidth / 2;
		int pos_x = (int)m_View.getCenter().x;
		int current_page = pos_x / gConfig.m_nWindowWidth;

		// page left
		current_page--;
		if (current_page < 0)
			current_page = 0;
		m_nScrollBy = -(pos_x - (width_2 + current_page * gConfig.m_nWindowWidth));
	}


	void ScrollPageRight()
	{
		int width_2 = gConfig.m_nWindowWidth / 2;
		int pos_x = (int)m_View.getCenter().x;
		int current_page = pos_x / gConfig.m_nWindowWidth;

		// page right
		current_page++;
		if (current_page >= m_IconManager.GetPageCount())
			current_page = m_IconManager.GetPageCount() - 1;
		m_nScrollBy = width_2 + current_page * gConfig.m_nWindowWidth - pos_x;
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::OnMouseUp()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void OnMouseUp(const sf::Event &event) override
	{
		// do not handle mouse clicks while scrolling
		if (m_nScrollBy != 0)
			return;

		int mouse_x = event.mouseButton.x + m_IconManager.GetCurrentPageNum() * gConfig.m_nWindowWidth;
		int mouse_y = event.mouseButton.y;
		int dx = mouse_x - m_DragInitialMouseStart.x;

		if (event.mouseButton.button == sf::Mouse::Left)
		{
			m_bIsDragging = false;

			// ==> maybe we need m_bHasSelected here as new flag in if-statement
			if ((gbEatSingleClick || !m_bWasLeftButtonDown) && !m_bHasDragged && m_enLassoMode == enLassoModeNone)
			{
				m_bIsDraggingIcons = false;
				gbEatSingleClick = false;
				return;
			}
			gbEatSingleClick = false;

			m_bWasLeftButtonDown = false;

			if (m_bIsDraggingIcons && !m_bHasDragged)
			{
				// single short click ==> execute / open
				// first, settle the icons
				m_IconManager.PlaceSelectedIcons();
				m_IconManager.SetSelectedIconsState(CIcon::enStateHighlight);
				m_bIsDraggingIcons = false;
				m_bHasDragged = false;

				// set keyboard cursor
				m_posKbdCursor = m_pKbdCursorIcon->GetPosition();

				if (m_IconManager.GetSelectedIcons().size() == 1 && m_ClickClock.getElapsedTime().asMilliseconds() < 300)
					OpenIcon();
			}
			else if (m_bIsDraggingIcons)
			{
				// icon(s) were moved
				m_IconManager.PlaceSelectedIcons();
				m_IconManager.SetSelectedIconsState(CIcon::enStateHighlight);
				//m_IconManager.ClearSelectedIcons();
				m_bIsDraggingIcons = false;

				// set keyboard cursor
				m_posKbdCursor = m_pKbdCursorIcon->GetPosition();
			}
			else if (m_bHasDragged)		// there was a minimum required mouse movement, otherwise it is interpreted as a click
			{
				// desktop scrolling
				int width_2 = gConfig.m_nWindowWidth / 2;
				int pos_x = (int)m_View.getCenter().x;

				// check, whether mouse movement did reach a threshold
				if (abs(dx) > gConfig.ScaleCoord(128))
				{
					// threshold reached: finish scroll to next page
					if (pos_x < width_2)
					{
						// we are on the first page and user tries to scroll right, this is not possible, scroll back
						m_nScrollBy = width_2 - pos_x;
					}
					else
					{
						// normal scrolling
						int current_page = pos_x / gConfig.m_nWindowWidth;

						if (dx > 0)	// scroll left?
						{
							// page left
							current_page--;
							if (current_page < 0)
								current_page = 0;
							m_nScrollBy = -(pos_x - (width_2 + current_page * gConfig.m_nWindowWidth));
						}
						else
						{
							// page right
							current_page++;
							if (current_page >= m_IconManager.GetPageCount())
								current_page = m_IconManager.GetPageCount() - 1;
							m_nScrollBy = width_2 + current_page * gConfig.m_nWindowWidth - pos_x;
						}
						
						m_IconManager.GotoPage(current_page);
					}
				}
				else
				{
					// threshold not reached, scroll back to initial position of the page
					if (dx > 0)			// scroll left?
						m_nScrollBy = width_2 - pos_x % width_2;
					else
						m_nScrollBy = -pos_x % width_2;
				}
			}
			else if (m_enLassoMode != enLassoModeNone)
			{
				m_enLassoMode = enLassoModeNone;
				m_IconManager.CreateSelectedSnapshot();
			}
			else
			{
				// test, if any of the page indicators has been clicked
				int page_num = m_PageIndicators.HitTest(event.mouseButton.x, event.mouseButton.y);
				if (page_num >= 0)
				{
					GotoPage(page_num);
					return;
				}

				// no ctrl + click here?
				if (!ControlKeyPressed())
				{
					// clicked into free area of our app?
					CIcon *icon = m_IconManager.FindIconUnderMouse(mouse_x, mouse_y);
					if (icon == nullptr)
					{
						// yes
						if (m_IconManager.HasSelectedIcons())		// if there are any selected icons
						{
							m_IconManager.SelectAll(false);
							m_IconManager.ClearSelectedSnapshot();	// then deselect them
						}
						else
						{
							// otherwise hide window
							HideMainWindow();
						}
					}
				}
			}
		}
		else if (event.mouseButton.button == sf::Mouse::Right)
		{
			// clicked on an icon?
			CIcon *icon = m_IconManager.FindIconUnderMouse(mouse_x, mouse_y);
			if (icon)
			{
				if (icon->GetState() != CIcon::enStateHighlight)
				{
					// an unselected icon has been clicked
					// de-select all other icons in this case
					m_IconManager.SelectAll(false);
					m_IconManager.ClearSelectedSnapshot();
					m_IconManager.SetIconState(icon, CIcon::enStateHighlight);
				}
				m_IconManager.CreateSelectedSnapshot();
				RunActivityThread(false);	// update view so icon scaling is shown
				DoIconContextMenu(event.mouseButton.x, event.mouseButton.y);
				TerminateActivityThread();

				// set keyboard cursor
				m_posKbdCursor = icon->GetPosition();
			}
			else
				DoMainContextMenu(event.mouseButton.x, event.mouseButton.y);
		}

		SetMouseCursor();
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::OnMouseMove()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void OnMouseMove(const sf::Event &event) override
	{
		int mouse_x = event.mouseMove.x + m_IconManager.GetCurrentPageNum() * gConfig.m_nWindowWidth;
		int mouse_y = event.mouseMove.y;
		m_LastMousePos.x = event.mouseMove.x;
		m_LastMousePos.y = event.mouseMove.y;
		m_LastMouseOffsetPos.x = mouse_x;
		m_LastMouseOffsetPos.y = mouse_y;

		if (m_bIsDragging)
		{
			int dx = mouse_x - m_DragMouseStart.x;
			int dy = mouse_y - m_DragMouseStart.y;
			m_DragMouseStart.x = mouse_x;
			m_DragMouseStart.y = mouse_y;

			if (abs(dx) >= 3 || abs(dy) >= 3)
				m_bHasDragged = true;
			
			if (m_bIsDraggingIcons)
			{
				// dragging icon(s)
				if (m_nScrollBy == 0)
				{
					if (event.mouseMove.x < 16)
						ScrollPageLeft();
					else if (event.mouseMove.x >= gConfig.m_nWindowWidth - 16)
						ScrollPageRight();
				}

				m_IconManager.MoveSelectedIcons((float)dx, (float)dy);
			}
			else
			{
				// dragging on the desktop, i.e. the view is scrolled
				//if (abs(dx) > 5)
				{
					m_View.move((float)-dx, 0);

					// limit movement, if first page or last page
					sf::Vector2f pos = m_View.getCenter();
					int max_left = gConfig.m_nWindowWidth / 3;
					int max_right = (m_IconManager.GetPageCount() - 1) * gConfig.m_nWindowWidth + gConfig.m_nWindowWidth * 2 / 3;

					if (pos.x < max_left)
					{
						pos.x = (float)max_left;
						m_View.setCenter(pos);
					}
					else if (pos.x > max_right)
					{
						pos.x = (float)max_right;
						m_View.setCenter(pos);
					}
				}
			}
		}
		else if (m_enLassoMode != enLassoModeNone)
		{
			// when scrolling, disable lasso mode
			if (m_nScrollBy != 0)
			{
				m_enLassoMode = enLassoModeNone;
				m_IconManager.CreateSelectedSnapshot();
				return;
			}

			sf::FloatRect rc;

			if (m_DragInitialMouseStart.x <= mouse_x)
			{
				rc.left = (float)m_DragInitialMouseStart.x;
				rc.width = (float)(mouse_x - m_DragInitialMouseStart.x);
			}
			else
			{
				rc.left = (float)mouse_x;
				rc.width = (float)(m_DragInitialMouseStart.x - mouse_x);
			}

			if (m_DragInitialMouseStart.y <= mouse_y)
			{
				rc.top = (float)m_DragInitialMouseStart.y;
				rc.height = (float)(mouse_y - m_DragInitialMouseStart.y);
			}
			else
			{
				rc.top = (float)mouse_y;
				rc.height = (float)(m_DragInitialMouseStart.y - mouse_y);
			}

			// add selection or deselect
			m_IconManager.LassoSelect(m_enLassoMode == enLassoModeSelect, rc);
		}

		// hovering above an icon?
		SetMouseCursor();

		/* else
		{
			// Only hovering
			// check for blow-up, shrink-down, if mouse is over sprite
			CIcon *icon = m_IconManager.FindIconUnderMouse(mouse_x, mouse_y);
			if (icon)
			{
				// yet no mouse-hovering recorded for this sprite
				if (!m_pHoverIcon)
				{
					m_HoverPos.x = mouse_x;
					m_HoverPos.y = mouse_y;
					m_HoverClock.restart();
					m_pHoverIcon = icon;
				}
				else if (m_pHoverIcon->GetState() != CIcon::enStateBlowup)
				{
					if (ManhattanDistance(m_HoverPos, mouse_x, mouse_y) > HoverTolerance)
					{
						m_HoverPos.x = mouse_x;
						m_HoverPos.y = mouse_y;
						m_HoverClock.restart();
					}
				}
			}
			else
			{
				if (m_pHoverIcon)
				{
					m_IconManager.SetIconState(m_pHoverIcon, CIcon::enStateNormal);
					m_pHoverIcon = nullptr;
				}
			}
		}
		*/
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::OnMouseWheel()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void OnMouseWheel(const sf::Event &event) override
	{
		if (gSettings.m_bScrollWheelJumps)
		{
			if (event.mouseWheel.delta < 0)
				GotoPage(m_IconManager.GetCurrentPageNum() + 1);
			else
				GotoPage(m_IconManager.GetCurrentPageNum() - 1);
		}
		else
		{
			if (event.mouseWheel.delta < 0)
				ScrollPageRight();
			else
				ScrollPageLeft();
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::OnKeyPressed()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void OnKeyPressed(const sf::Event &event) override
	{
		if (event.key.code == sf::Keyboard::Escape)
		{
			// hide window
			HideMainWindow();
		}
		else if (event.key.code == sf::Keyboard::Delete)
		{
			// delete selected
			DeleteIcons();
		}
		else if (event.key.code == sf::Keyboard::Menu)
		{
			m_IconManager.CreateSelectedSnapshot();

			if (m_IconManager.GetSelectedIcons().size() == 1)
			{
				CIcon *icon = m_IconManager.GetSelectedIcons().front();
				DoIconContextMenu((int)icon->GetSpritePosition().x + gConfig.m_nIconSizeX_2, (int)icon->GetSpritePosition().y + gConfig.m_nIconSizeX_2);
			}
			else if (m_IconManager.GetSelectedIcons().size() == 0)
			{
				DoMainContextMenu(m_LastMousePos.x, m_LastMousePos.y);
			}
		}
		else if (event.key.code == sf::Keyboard::Home)
		{
			GotoPage(0);
		}
		else if (event.key.code == sf::Keyboard::End)
		{
			GotoPage(m_IconManager.GetPageCount() - 1);
		}
		else if (event.key.code == sf::Keyboard::PageUp)
		{
			int cur_page = m_IconManager.GetCurrentPageNum() - 1;
			if (cur_page < 0)
				cur_page = m_IconManager.GetPageCount() - 1;
			GotoPage(cur_page);
		}
		else if (event.key.code == sf::Keyboard::PageDown)
		{
			int cur_page = m_IconManager.GetCurrentPageNum() + 1;
			if (cur_page >= m_IconManager.GetPageCount())
				cur_page = 0;
			GotoPage(cur_page);
		}
		else if (event.key.code == sf::Keyboard::F2)
		{
			m_IconManager.CreateSelectedSnapshot();
			IconPropertyDialog();
		}
		else if (event.key.code >= sf::Keyboard::Num0 && event.key.code <= sf::Keyboard::Num9)
		{
			int page = event.key.code - sf::Keyboard::Num1;
			if (page < 0)
				page = 9;	// key 0 = page 10
			GotoPage(page);
		}
		else if (event.key.code >= sf::Keyboard::Numpad0 && event.key.code <= sf::Keyboard::Numpad9)
		{
			int page = event.key.code - sf::Keyboard::Numpad1;
			if (page < 0)
				page = 9;	// key 0 = page 10
			GotoPage(page);
		}
		else if (IsArrowKey(event.key.code))
		{
			// keyboard navigation
			// if a single icon is selected and the icon is on the currently visible page, we change the focus-position accordingly.
			CIcon *icon = nullptr;
			if (m_IconManager.GetSelectedIcons().size() == 1 && m_posKbdCursor.m_nPage == m_IconManager.GetCurrentPageNum())
			{
				do	// until an icon is found
				{
					if (event.key.code == sf::Keyboard::Left)
					{
						if (m_posKbdCursor.m_RasterPos.m_nCol > 0)
							m_posKbdCursor.m_RasterPos.m_nCol--;
						else
						{
							/* do not change the page, because on the next page there might not be any icons in the same row
							m_posKbdCursor.m_nPage--;
							if (m_posKbdCursor.m_nPage < 0)
								m_posKbdCursor.m_nPage = m_IconManager.GetPageCount() - 1;*/
							m_posKbdCursor.m_RasterPos.m_nCol = m_IconManager.m_nMaxCols - 1;
						}
					}
					else if (event.key.code == sf::Keyboard::Right)
					{
						if (m_posKbdCursor.m_RasterPos.m_nCol < m_IconManager.m_nMaxCols - 1)
							m_posKbdCursor.m_RasterPos.m_nCol++;
						else
						{
							/* do not change the page, because on the next page there might not be any icons in the same row
							m_posKbdCursor.m_nPage++;
							if (m_posKbdCursor.m_nPage >= m_IconManager.GetPageCount())
								m_posKbdCursor.m_nPage = 0;*/
							m_posKbdCursor.m_RasterPos.m_nCol = 0;
						}
					}
					else if (event.key.code == sf::Keyboard::Up)
					{
						if (m_posKbdCursor.m_RasterPos.m_nRow > 0)
							m_posKbdCursor.m_RasterPos.m_nRow--;
						else
							m_posKbdCursor.m_RasterPos.m_nRow = m_IconManager.m_nMaxRows - 1;
					}
					else if (event.key.code == sf::Keyboard::Down)
					{
						if (m_posKbdCursor.m_RasterPos.m_nRow < m_IconManager.m_nMaxRows - 1)
							m_posKbdCursor.m_RasterPos.m_nRow++;
						else
							m_posKbdCursor.m_RasterPos.m_nRow = 0;
					}

					icon = m_IconManager.GetIconFromPos(m_posKbdCursor);
				}
				while (!icon);
			}
			else if (m_posKbdCursor.m_nPage != m_IconManager.GetCurrentPageNum())	//m_IconManager.GetSelectedIcons().size() == 1)
			{
				// the icon is not on the currently visible page: select the (0, 0) icon on the currently visible page
				m_posKbdCursor.m_nPage = m_IconManager.GetCurrentPageNum();
				m_posKbdCursor.m_RasterPos = CRasterPos();
			}
			// else // no icon is selected, or multiple icons are selected: select the current keyboard-icon.

			icon = m_IconManager.GetIconFromPos(m_posKbdCursor);
			if (icon)
			{
				GotoPage(m_posKbdCursor.m_nPage);
				m_IconManager.ClearSelectedSnapshot();
				m_IconManager.SetIconState(icon, CIcon::enStateHighlight);
				m_IconManager.CreateSelectedSnapshot();
			}
		}
		else if (event.key.code == sf::Keyboard::Return)
		{
			if (m_IconManager.GetSelectedIcons().size() == 1)
				OpenIcon();
		}
		else if (event.key.control)	// ctrl-key is down
		{
			if (event.key.code == sf::Keyboard::A)
				m_IconManager.SelectAll(true);
			else if (event.key.code == sf::Keyboard::Z)
				m_IconManager.Undo();
			else if (event.key.code == sf::Keyboard::Y)
				m_IconManager.Redo();
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::OnDroppedFiles()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void OnDroppedFiles(const sf::Event &event) override
	{
		m_IconManager.ClearSelectedSnapshot();
		m_IconManager.SelectAll(false);

		CIconUndoGroup *undo_group = new CIconUndoGroup();

		for (list<TCHAR *>::iterator it = event.dropFiles.listDroppedFiles->begin(); it != event.dropFiles.listDroppedFiles->end(); it++)
		{
			RString strFilePath = *it;
			delete[] *it;
			if (strFilePath.length())
			{
				RString strShellDisplayName = GetShellItemName(strFilePath);
				CIcon *icon = m_IconManager.CreateIcon(strFilePath, strShellDisplayName, m_LastMouseOffsetPos.x, m_LastMouseOffsetPos.y);
				undo_group->Add(new CIconInsertAction(icon));
			}
		}

		// create undo action
		m_IconManager.PushUndoAction(undo_group);

		// After a drop-operation we want to gain the focus
		SetForegroundWindowInternal(m_Window.getSystemHandle());
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::Work()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void Work() override
	{
		HookTestMouseHover();

		if (gbHookEvent)
		{
			// we have any of the user-defined activation actions here, which was caught by our hook
			gbHookEvent = false;

			// We have a problem here: gbHookEvent is somehow true through the hook-function, though we set it false here.
			// Therefore we check m_enFade, which functions as a delay, during which gbHookEvent is set permanently to false.
			if (m_enFade == enFmNoFade)
			{
				HWND fg_win = GetForegroundWindow();
				//if (fg_win != m_Window.getSystemHandle())
				if (!IsWindowVisible(m_Window.getSystemHandle()))
				{
					if (!gSettings.m_bDisableIfMaxWin || !IsFullscreenAndMaximized(fg_win))
					{
						// show
						ShowMainWindow();
					}
				}
				else
				{
					// hide
					HideMainWindow();
				}
			}
		}

		if (gbHookDoubleClick)
		{
			// we have a left mouse button double-click here, which was caught by our hook
			gbHookDoubleClick = false;

			// was it on the desktop window?
			if (GetForegroundWindow() == m_hwndProgman)
			{
				// yes, is no icon on the desktop selected?
				// otherwise it was a double-click to execute a program / link
				if (ListView_GetSelectedCount(m_hwndDesktopListView) == 0)
				{
					// yes, activate ourselves
					ShowMainWindow();
				}
			}
		}

		/*
		// slowly resync icons
		static __int64 counter = 0;
		if (m_bResync)
		{
			counter++;
			if (counter == 20)	// 20 frames = about 0.33 seconds @ 60Hz screen refresh rate
			{
				counter = 0;
				if (!m_IconManager.ResyncOneIcon())
					m_bResync = false;
			}
		}*/

		if (m_bHideWithDelay)
		{
			if (m_IconManager.GetSelectedIcons().size() > 0)
			{
				CIcon *icon = m_IconManager.GetSelectedIcons().front();

				if (icon->m_fTargetScale == icon->m_Sprite.getScale().x)
				{
					m_bHideWithDelay = false;
					m_IconManager.SetIconState(icon, CIcon::enStateNormal);

					// animate shrinking icon
					for (int i = 0; i < 16; i++)
					{
						Draw();
						Sleep(10);
						m_Window.display();
					}
					m_IconManager.SetIconState(icon, CIcon::enStateNormal, true);
					HideMainWindow();
					OpenLocation();
				}
			}
#if 0	// I don't understand why I inserted this code
			else
			{
				m_bHideWithDelay = false;
				HideMainWindow();
				OpenLocation();
			}
#endif
		}

		if (m_enFade == enFmFadeIn)
		{
			if (m_fFadeValue > 1.f)
				m_fFadeValue = 1.f;
			SetTransparency(m_fFadeValue);
			if (m_fFadeValue >= 1.f)
			{
				m_enFade = enFmNoFade;
				SetForegroundWindowInternal(m_Window.getSystemHandle());	// sometimes the window is shown, but it is not in foreground, due to timing issues with Windows it seems, so we set it here again into the foreground
			}
			m_fFadeValue += 0.09f;	// 0.075f;
		}
		else if (m_enFade == enFmFadeOut)
		{
			if (m_fFadeValue < 0.f)
				m_fFadeValue = 0.f;
			SetTransparency(m_fFadeValue);
			if (m_fFadeValue <= 0.f)
				m_enFade = enFmNoFade;
			m_fFadeValue -= 0.09f;	// 0.075f;
		}

		if (m_bDrawActivityOnly)
		{
			RString old_title = m_Activity.GetTitle();
			m_Activity.SetTitle(_T("Scaling Icons"));
			RunActivityThread();

			m_IconManager.RecomputeAllIcons();

			TerminateActivityThread();
			m_Activity.SetTitle(old_title);
			m_bDrawActivityOnly = false;
		}
		else if (IsWindowVisible(m_Window.getSystemHandle()))	// this else is important! Otherwise calling ComputeLayout() might cause
		{														// redrawing, but this main-thread does not own the GL context!
			long elapsed = m_Timer.getElapsedTime().asMilliseconds();
			if (m_bChangeWindowLayout)
			{
				if (m_Taskbar.ChangedPosition())
					m_Timer.restart();
				else if (elapsed >= 1000)
				{
					m_bChangeWindowLayout = false;
					ComputeLayout();
					m_Timer.restart();
				}
			}
			else if (elapsed > 200)
			{
				m_Timer.restart();
				if (m_Taskbar.ChangedPosition() || DesktopResolutionChanged())
					m_bChangeWindowLayout = true;
			}
		}
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------
	//															CMyApp::Draw()
	// --------------------------------------------------------------------------------------------------------------------------------------------
	void Draw() override
	{
//		if (m_enBkgType == enBkgSolidColor)
			m_Window.clear(m_BkgColor);

		// compute animations
		// ------------------
#if 0	// BLOW-UP disabled for now
		// check mouse hovering
		if (m_pHoverIcon)
		{
			if (m_pHoverIcon->GetState() == CIcon::enStateNormal &&
				HoverClock.getElapsedTime().asMilliseconds() >= HoverTime)
			{
				m_IconManager.SetIconState(m_pHoverIcon, CIcon::enStateBlowup);
			}
		}
#endif

		if (!m_bDrawActivityOnly)
			m_IconManager.PerformAnimations();

		// draw background
		// ---------------
		if (m_enBkgType == enBkgWallpaper)
		{
			if (gSettings.m_nModifyImage == 1)
			{
				// draw tiled background image (könnte man auch über einen Fragment-Shader machen, indem das tiled image als textur übergeben wird)
				for (unsigned int y_count = 0; y_count <= gConfig.m_nWindowHeight / m_BkgTexture.getSize().y; y_count++)
				{
					for (unsigned int x_count = 0; x_count <= gConfig.m_nWindowWidth / m_BkgTexture.getSize().x; x_count++)
					{
						m_BkgSprite.setPosition((float)(x_count * m_BkgTexture.getSize().x), (float)(y_count * m_BkgTexture.getSize().y));
						m_Window.draw(m_BkgSprite);
					}
				}
			}
			else
			{
				// draw (scaled or unscaled) image to window dimensions
				m_Window.draw(m_BkgSprite);
			}
		}
		else if (m_enBkgType == enBkgShader)
		{
			// draw background with a shader
			m_Window.draw(m_BackgroundShape, &m_Shader);
			m_fShaderTime += m_ShaderTimeInc;
			m_Shader.setParameter("time", m_fShaderTime);
		}
		
		if (!m_bDrawActivityOnly)
		{
			// perform scrolling, if desired
			// -----------------------------
			if (m_nScrollBy != 0)
			{
				int scroll_val = (int)powf(((float)abs(m_nScrollBy) / 15.f), 2.f);		// smooth speed when reaching end-point
		
				if (scroll_val < 8)
					scroll_val = 8;
				else if (scroll_val > 64)
					scroll_val = 64;

				// scale scrolling speed on larger or smaller screens, reference is 1920 pixels
				double factor = (double)gConfig.m_nWindowWidth / 1920.0;
				scroll_val = (int)((double)scroll_val * factor);

				int to_scroll;
				if (m_nScrollBy > 0)
					to_scroll = std::min(m_nScrollBy, scroll_val);
				else if (m_nScrollBy < 0)
					to_scroll = std::max(m_nScrollBy, -scroll_val);

				m_nScrollBy -= to_scroll;
				m_View.move((float)to_scroll, 0);

				if (m_nScrollBy == 0)
				{
					// if scrolling has finished, activate the page we are on now
					int pos_x = (int)m_View.getCenter().x;
					int current_page = pos_x / gConfig.m_nWindowWidth;
					m_IconManager.GotoPage(current_page);
					m_DragMouseStart.x = current_page * gConfig.m_nWindowWidth + m_LastMousePos.x;
				}

				// hovering above an icon?
				SetMouseCursor();

				if (m_bIsDraggingIcons)
				{
					m_IconManager.MoveSelectedIcons((float)to_scroll, 0);
					//if (m_nScrollBy == 0)
					//	m_DragMouseStart.x = (int)m_pSelectedIcon->GetPosition().x;
				}
			}

			// draw icons
			// ----------
			m_Window.setView(m_View);
			int pos = (int)m_View.getCenter().x;
			pos -= gConfig.m_nWindowWidth / 2;
			if (pos < 0)
				pos = 0;
			int start_page = pos / gConfig.m_nWindowWidth;
			int end_page = (pos + gConfig.m_nWindowWidth) / gConfig.m_nWindowWidth;
			if (pos % gConfig.m_nWindowWidth >= gConfig.m_nHorizontalMargin)
				end_page++;
			if (end_page > m_IconManager.GetPageCount())
				end_page = m_IconManager.GetPageCount();
			int cur_page = m_IconManager.GetCurrentPageNum();
			for (int page = start_page; page < end_page; page++)
			{
				m_IconManager.GotoPage(page);
				m_IconManager.DrawIcons(m_Window, *gConfig.m_pText, pos, pos + gConfig.m_nWindowWidth);
			}
			m_IconManager.GotoPage(cur_page);

			// now draw the unlinked icons, these are the icons that are currently
			// moved with the mouse, and which don't belong to any page
			m_IconManager.DrawUnlinkedIcons(m_Window, *gConfig.m_pText, pos, pos + gConfig.m_nWindowWidth);

			// finally draw the icons from the animation list, which are not covered
			// by the page-range we had computed above
			m_IconManager.DrawAnimatedIcons(start_page, end_page, m_Window, *gConfig.m_pText, pos, pos + gConfig.m_nWindowWidth);

			// Draw Lasso
			// ----------
			if (m_enLassoMode)
			{
				sf::RectangleShape lasso(sf::Vector2f((float)(m_LastMouseOffsetPos.x - m_DragInitialMouseStart.x), (float)(m_LastMouseOffsetPos.y - m_DragInitialMouseStart.y)));
				lasso.setPosition((float)m_DragInitialMouseStart.x, (float)m_DragInitialMouseStart.y);
				lasso.setFillColor(sf::Color(0xff, 0xff, 0xff, 0x60));

				// set a 1-pixel wide blue outline
				lasso.setOutlineThickness(1);
				lasso.setOutlineColor(sf::Color(0x00, 0x60, 0xff));
				m_Window.draw(lasso);
			}

			// restore view (background may not be drawn with a scrolled view applied)
			m_Window.setView(m_InitialView);

			// draw page indicators
			m_PageIndicators.Draw(m_Window, m_IconManager.m_nCurPageNum);

			// draw license string
			if (!m_strLicense.empty())
			{
				float window_height = (float)gConfig.m_nWindowHeight;
				float bottom_space = (float)gConfig.m_nBottomSpace;

				float x = 20.f;
				float y = window_height - bottom_space * .33f - bottom_space * .09f;

				gConfig.m_pText->setString(m_strLicense.c_str());
				gConfig.m_pText->setPosition(x + 1, y + 1);
				gConfig.m_pText->setColor(sf::Color(0x40, 0x40, 0x40));
				m_Window.draw(*gConfig.m_pText);

				gConfig.m_pText->setPosition(x, y);
				gConfig.m_pText->setColor(sf::Color(255, 255, 255));
				m_Window.draw(*gConfig.m_pText);
			}
		}

		if (m_Activity.GetVisible())
			m_Activity.Draw(m_Window);

		if (m_IconManager.m_bResyncRunning)
			m_ResyncActivity.Draw(m_Window);

		SetMouseCursor();
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																DrawThread()
//
// This thread calls periodically CSfmlApp::Draw() to keep the view alive for the animated activity indicator.
// The activity indicator is active, if the icon database is saved. This performed in the main thread, which 
// makes things much simpler (no synchronization locks and the UI is disabled).
// --------------------------------------------------------------------------------------------------------------------------------------------
unsigned __stdcall ActivityThread(void *param)
{
	CMyApp *app = (CMyApp *)param;

	app->GetRenderWindow().setActive(true);
	while (!gbTerminateActivityThread)
	{
		app->Draw();
		app->GetRenderWindow().display();
		Sleep(15);
	}

	app->GetRenderWindow().setActive(false);	// shift GL away from this thread

	return 1;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//																ResyncThread()
// --------------------------------------------------------------------------------------------------------------------------------------------
unsigned __stdcall ResyncThread(void *param)
{
	CIconManager *icon_manager = (CIconManager *)param;

	icon_manager->ResyncIcons();

	return 1;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//																ShellExecuteThread()
//
// When calling ShellExecute(), Windows causes a huge delay, either because a virus scanner starts scanning the executable, or due
// to network access by the domain controller. Therefore we call ShellExecute() from a thread.
// --------------------------------------------------------------------------------------------------------------------------------------------
unsigned __stdcall ShellExecuteThread(void *param)
{
	CMyApp *app = (CMyApp *)param;

	const TCHAR *verb;
	if (app->m_bOpenAsAdmin)
		verb = _T("runas");
	else
		verb = _T("open");

	ShellExecute(app->GetRenderWindow().getSystemHandle(), verb, app->m_strOpenLocPath.c_str(), app->m_strOpenLocParams.c_str(), GetPath(app->m_strOpenLocPath).c_str(), SW_SHOWNORMAL);
	app->m_strOpenLocPath.clear();
	app->m_bOpenAsAdmin = false;

	return 1;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															WindowSearcher()
// --------------------------------------------------------------------------------------------------------------------------------------------
BOOL CALLBACK WindowSearcher(HWND hWnd, LPARAM lParam)
{
	TCHAR classname[32];
	GetClassName(hWnd, classname, _tcschars(classname));
	if (_tcscmp(classname, APP_CLASS_NAME) == 0)
	{
		HWND *target = (HWND *)lParam;
		*target = hWnd;
		return FALSE;	// stop search
	}

	return TRUE;		// continue search
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//																main
// --------------------------------------------------------------------------------------------------------------------------------------------
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	// activate memory heap checking at program exit
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif

	// Create a mutex with a unique name, so Inno Setup can terminate the program before update / reinstall / uninstall
	CreateMutex(NULL, FALSE, _T("IS$$Mutex$$") APP_NAME);

	// if an instance of our app is running, activate the instance and bail out
	HWND hwndChild = NULL;
	EnumWindows(WindowSearcher, (LPARAM)&hwndChild);	// FindWindow(APP_CLASS_NAME, NULL) does not work, because it is not a top-level window
	if (hwndChild)
	{
		// get the SFML owner window
		HWND hwndParent = GetWindow(hwndChild, GW_OWNER);
		if (hwndParent)
		{
			SetForegroundWindowInternal(hwndParent);
			ShowWindow(hwndParent, SW_SHOW);
		}
		return 0;
	}

	// set gstrAppData and gstrCommonAppData
	if (!GetAppPaths())
	{
		MessageBox(NULL,
			_T("Can not retrieve application data directory!\n\n")
			_T("SHGetFolderPath() failed.\n\n")
			_T("Please fix your system."), 
			APP_NAME, MB_ICONERROR);
		return 1;
	}

	gstrAppData += DATA_FOLDER;
	gstrCommonAppData += DATA_FOLDER;
	gstrWindir += _T('\\');
	ghInstance = hInstance;

	// init COM
	CoInitializeEx(NULL, COINIT_MULTITHREADED  | COINIT_DISABLE_OLE1DDE); // COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	CMyApp app;
	app.Run();

	// cleanup
	gConfig.Free();

	// Uninitialize the COM Library
	CoUninitialize();

	return 0;
}
