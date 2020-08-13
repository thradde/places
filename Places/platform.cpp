
// Copyright (c) 2020 IDEAL Software GmbH, Neuss, Germany.
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

// --------------------------------------------------------------------------------------------------------------------------------------------
//															Platform specific code
//
// When porting to another platform, the functions contained in here need to be rewritten.
//
// NOTE: This comment was written at the beginning of the project.
//		 Meanwhile it turned out that Windows specific code had to be injected at several different places.
//		 Gaining platform independence will require a bigger cleanup.
// --------------------------------------------------------------------------------------------------------------------------------------------

#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>

#include "memdebug.h"

#include <SFML/Graphics.hpp>

#include <tchar.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shlguid.h>
#include <shellapi.h>

#include <vector>
using namespace std;

#define ASSERT
#include "RfwString.h"
#include "platform.h"
#include "generic.h"
#include "hook.h"


// --------------------------------------------------------------------------------------------------------------------------------------------
//																Globals
// --------------------------------------------------------------------------------------------------------------------------------------------
RString gstrAppData;
RString gstrCommonAppData;
RString gstrWindir;


// --------------------------------------------------------------------------------------------------------------------------------------------
//															GetAppPaths()
//
// sets gstrAppData and gstrCommonAppData
// --------------------------------------------------------------------------------------------------------------------------------------------
bool GetAppPaths()
{
	TCHAR szPath[2048];
	
	// make sure, each SHGetFolderPath() is called, there the if-statement comes later
	bool r1 = SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szPath));
	if (r1)
		gstrAppData = szPath;

	bool r2 = SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szPath));
	if (r2)
		gstrCommonAppData = szPath;

	bool r3 = SUCCEEDED(SHGetFolderPath(NULL, CSIDL_WINDOWS, NULL, 0, szPath));
	if (r3)
		gstrWindir = szPath;

	return r1 && r2 && r3;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															GetShellItemName()
//
// Returns the name of a file, as shown in the Windows shell, for example the title given to an icon on the desktop.
// But works as well for normal file names.
//
// https://msdn.microsoft.com/en-us/library/windows/desktop/ff934858.aspx
// --------------------------------------------------------------------------------------------------------------------------------------------
RString GetShellItemName(const RString &strFilePath)
{
	RString item_name;
	IShellItem *pItem = nullptr;
	HRESULT hr = ::SHCreateItemFromParsingName(strFilePath.c_str(), nullptr, IID_PPV_ARGS(&pItem));
	if (SUCCEEDED(hr))
	{
		LPWSTR szName = nullptr;
		hr = pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &szName);
		if (SUCCEEDED(hr))
		{
			item_name = szName;
			::CoTaskMemFree(szName);
		}
		pItem->Release();
	}

	return item_name;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															ResolveLink()
//
// Returns the path and file name of a given LNK-file.
//
// https://msdn.microsoft.com/en-us/library/windows/desktop/bb776891.aspx#Shellink_Resolving_Shortcut
// --------------------------------------------------------------------------------------------------------------------------------------------
HRESULT ResolveLink(HWND hwnd, const RString &strLinkFile, RString &path) 
{ 
	HRESULT hres; 
	IShellLink *psl; 
	WCHAR szGotPath[MAX_PATH]; 
	WIN32_FIND_DATA wfd; 

	path.clear();

	// Get a pointer to the IShellLink interface. It is assumed that CoInitialize
	// has already been called. 
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl); 
	if (SUCCEEDED(hres)) 
	{ 
		IPersistFile *ppf; 

		// Get a pointer to the IPersistFile interface. 
		hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 

		if (SUCCEEDED(hres)) 
		{
			hres = ppf->Load(strLinkFile.c_str(), STGM_READ); 
			if (SUCCEEDED(hres)) 
			{ 
				// Resolve the link. 
				hres = psl->Resolve(hwnd, 0); 

				if (SUCCEEDED(hres)) 
				{ 
					// Get the path to the link target. 
					hres = psl->GetPath(szGotPath, MAX_PATH, (WIN32_FIND_DATA*)&wfd, 0); //SLGP_SHORTPATH); 
					path = szGotPath;
				} 
			} 

			// Release the pointer to the IPersistFile interface. 
			ppf->Release(); 
		} 

		// Release the pointer to the IShellLink interface. 
		psl->Release(); 
	}

	return hres; 
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														GetDesktopListViewHWND()
// 
// Get handle of Desktop List View. Windows Progman manages the desktop icons internally in a SysListView.
//
// To catch a double-click onto the free Progman desktop area:
// - Retrieve window handle of desktop SysListView
// - Intercept WM_MOUSEDOUBLECLICK using a global mouse hook.
// - Send LVM_GETSELECTEDCOUNT to the ListView, if result = 0, then the user clicked onto an empty desktop space!
//
// Source code from: http://blog.syedgakbar.com/2013/01/windows-desktop-listview-handle/
// --------------------------------------------------------------------------------------------------------------------------------------------
HWND GetDesktopListViewHWND()
{
	HWND hDesktopListView = NULL;
	HWND hWorkerW = NULL;

	HWND hProgman = FindWindow(_T("Progman"), 0);
	HWND hDesktopWnd = GetDesktopWindow();

	// If the main Program Manager window is found
	if (hProgman)
	{
		// Get and load the main List view window containing the icons (found using Spy++).
		HWND hShellViewWin = FindWindowEx(hProgman, 0, _T("SHELLDLL_DefView"), 0);
		if (hShellViewWin)
			hDesktopListView = FindWindowEx(hShellViewWin, 0, _T("SysListView32"), 0);
		else
		{
			// When this fails (happens in Windows-7 when picture rotation is turned ON), then look for the WorkerW windows list to get the
			// correct desktop list handle.
			// As there can be multiple WorkerW windows, so iterate through all to get the correct one
			do
			{
				hWorkerW = FindWindowEx( hDesktopWnd, hWorkerW, _T("WorkerW"), NULL );
				hShellViewWin = FindWindowEx(hWorkerW, 0, _T("SHELLDLL_DefView"), 0);
			}
			while (hShellViewWin == NULL && hWorkerW != NULL);

			// Get the ListView control
			hDesktopListView = FindWindowEx(hShellViewWin, 0, _T("SysListView32"), 0);
		}
	}

	return hDesktopListView;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															BuildKeystateString()
// --------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
	BYTE	Keycode;
	LPCTSTR	Name;
} TKeyNames;

static TKeyNames KeyNames[] =
{
	VK_SHIFT          , _T("Shift + "),
	VK_CONTROL        , _T("Ctrl + "),
	VK_MENU           , _T("Alt + "),
	VK_LWIN           , _T("Win + "),
	VK_RWIN           , _T("Win + "),

	VK_BACK           , _T("Backspace"),
	VK_TAB            , _T("Tab"),

	VK_CLEAR          , _T("Clear"),
	VK_RETURN         , _T("Enter"),

	VK_PAUSE          , _T("Pause"),
	VK_CAPITAL        , _T("Caps"),

	VK_ESCAPE         , _T("Esc"),

	VK_SPACE          , _T("Space"),
	VK_PRIOR          , _T("Prior"),
	VK_NEXT           , _T("Next"),
	VK_END            , _T("End"),
	VK_HOME           , _T("Home"),
	VK_LEFT           , _T("Left"),
	VK_UP             , _T("Up"),
	VK_RIGHT          , _T("Right"),
	VK_DOWN           , _T("Down"),
	VK_SELECT         , _T("Select"),
	VK_PRINT          , _T("Print"),
	VK_EXECUTE        , _T("Execute"),
	VK_SNAPSHOT       , _T("Snapshot"),
	VK_INSERT         , _T("Insert"),
	VK_DELETE         , _T("Delete"),
	VK_HELP           , _T("Help"),

	/*
	* VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
	* 0x40 : unassigned
	* VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
	*/

	VK_APPS           , _T("Apps"),

	VK_SLEEP          , _T("Sleep"),

	VK_NUMPAD0        , _T("Numpad0"),
	VK_NUMPAD1        , _T("Numpad1"),
	VK_NUMPAD2        , _T("Numpad2"),
	VK_NUMPAD3        , _T("Numpad3"),
	VK_NUMPAD4        , _T("Numpad4"),
	VK_NUMPAD5        , _T("Numpad5"),
	VK_NUMPAD6        , _T("Numpad6"),
	VK_NUMPAD7        , _T("Numpad7"),
	VK_NUMPAD8        , _T("Numpad8"),
	VK_NUMPAD9        , _T("Numpad9"),
	VK_MULTIPLY       , _T("Multiply"),
	VK_ADD            , _T("Add"),
	VK_SEPARATOR      , _T("Separator"),
	VK_SUBTRACT       , _T("Subtract"),
	VK_DECIMAL        , _T("Decimal"),
	VK_DIVIDE         , _T("Divide"),
	VK_F1             , _T("F1"),
	VK_F2             , _T("F2"),
	VK_F3             , _T("F3"),
	VK_F4             , _T("F4"),
	VK_F5             , _T("F5"),
	VK_F6             , _T("F6"),
	VK_F7             , _T("F7"),
	VK_F8             , _T("F8"),
	VK_F9             , _T("F9"),
	VK_F10            , _T("F10"),
	VK_F11            , _T("F11"),
	VK_F12            , _T("F12"),
	VK_F13            , _T("F13"),
	VK_F14            , _T("F14"),
	VK_F15            , _T("F15"),
	VK_F16            , _T("F16"),
	VK_F17            , _T("F17"),
	VK_F18            , _T("F18"),
	VK_F19            , _T("F19"),
	VK_F20            , _T("F20"),
	VK_F21            , _T("F21"),
	VK_F22            , _T("F22"),
	VK_F23            , _T("F23"),
	VK_F24            , _T("F24"),

	VK_BROWSER_BACK        , _T("BrowserBack"),
	VK_BROWSER_FORWARD     , _T("BrowserForward"),
	VK_BROWSER_REFRESH     , _T("BrowserRefresh"),
	VK_BROWSER_STOP        , _T("BrowserStop"),
	VK_BROWSER_SEARCH      , _T("BrowserSearch"),
	VK_BROWSER_FAVORITES   , _T("BrowserFavorites"),
	VK_BROWSER_HOME        , _T("BrowserHome"),

	VK_VOLUME_MUTE         , _T("VolumeMute"),
	VK_VOLUME_DOWN         , _T("VolumeDown"),
	VK_VOLUME_UP           , _T("VolumeUp"),
	VK_MEDIA_NEXT_TRACK    , _T("NextTrack"),
	VK_MEDIA_PREV_TRACK    , _T("PrevTrack"),
	VK_MEDIA_STOP          , _T("Stop"),
	VK_MEDIA_PLAY_PAUSE    , _T("PlayPause"),
	VK_LAUNCH_MAIL         , _T("LauchMail"),
	VK_LAUNCH_MEDIA_SELECT , _T("LaunchMediaSelect"),
	VK_LAUNCH_APP1         , _T("App1"),
	VK_LAUNCH_APP2         , _T("App2"),
	0					   , NULL,
};


RString GetVirtKeyName(BYTE keycode)
{
	if (keycode >= '0' && keycode <= '9')
		return (TCHAR)keycode;

	if (keycode >= 'A' && keycode <= 'Z')
		return (TCHAR)keycode;

	int i = 0;
	while (KeyNames[i].Name)
	{
		if (KeyNames[i].Keycode == keycode)
			return KeyNames[i].Name;
		i++;
	}

	return _T("");
}


RString GetKeyComboString(BYTE hotkey, BYTE virtkey)
{
	RString s;

	if (hotkey == 0)
		return _T("None");

	if (hotkey & HotkeyWin)
		s += _T("Win + ");

	if (hotkey & HotkeyAlt)
		s += _T("Alt + ");

	if (hotkey & HotkeyCtrl)
		s += _T("Ctrl + ");

	if (hotkey & HotkeyShift)
		s += _T("Shift + ");

	s += GetVirtKeyName(virtkey);

	return s;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															SetForegroundWindowInternal()
//
// http://www.codeproject.com/Tips/76427/How-to-bring-window-to-top-with-SetForegroundWindo
// --------------------------------------------------------------------------------------------------------------------------------------------
void SetForegroundWindowInternal(HWND hWnd)
{
	if (!hWnd || !::IsWindow(hWnd))
		return;

	// to unlock SetForegroundWindow we need to imitate pressing [Alt] key
	bool bPressed = false;
	if ((::GetAsyncKeyState(VK_MENU) & 0x8000) == 0)
	{
		bPressed = true;
		::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);
		Sleep(50);
	}

	::SetForegroundWindow(hWnd);

	if (bPressed)
	{
		Sleep(50);
		::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CreateFont()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CSfmlMemoryFont::Create(const sf::Window &window, const RString &strFontName)
{
	if (m_pBinaryFontData)
		return;

	HWND hwnd = window.getSystemHandle();

	LOGFONT lf;
	lf.lfHeight = -30;
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfUnderline = false;
	lf.lfStrikeOut = false;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	lf.lfItalic = false;
	lf.lfWeight = FW_NORMAL;
	_tcscpy(lf.lfFaceName, strFontName.c_str());

	HFONT hFont = CreateFontIndirect(&lf);
	HDC hdc = GetDC(hwnd);
	HFONT hFontOld = (HFONT)SelectObject(hdc, hFont);
	DWORD dwSize = GetFontData(hdc, 0, 0, NULL, 0);
	m_pBinaryFontData = malloc(dwSize);
	GetFontData(hdc, 0, 0, m_pBinaryFontData, dwSize);
	SelectObject(hdc, hFontOld);
	ReleaseDC(hwnd, hdc);

	m_Font.loadFromMemory(m_pBinaryFontData, dwSize);

	// we may NOT free the buffer, as it is used further by SFML !!!
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CFileDialog::CFileDialog()
// --------------------------------------------------------------------------------------------------------------------------------------------
CFileDialog::CFileDialog(const RString &path, DWORD flags, LPCTSTR filter, int default_filter)
	:	m_strPath(path),
		m_dwFlags(flags),
		m_pchFilter(filter),
		m_nDefaultFilter(default_filter)
{
	_tcsncpy(m_szPath, path.c_str(), MY_MAX_PATH);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CFileDialog::InitOfn()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CFileDialog::InitOfn(HWND hwnd_owner)
{
	memset(&m_Ofn, 0, sizeof(m_Ofn));
	m_Ofn.lStructSize	= sizeof(OPENFILENAME);
	m_Ofn.hwndOwner		= hwnd_owner;
	m_Ofn.lpstrFilter	= m_pchFilter ? m_pchFilter : NULL;
	m_Ofn.nFilterIndex	= m_nDefaultFilter;
	m_Ofn.lpstrFile		= m_szPath;			// Bei Öffnen bzw. Schließen gesetzt
	m_Ofn.nMaxFile		= MY_MAX_PATH;
	m_Ofn.Flags			= m_dwFlags;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CFileDialog::ExitOfn()
//
// Auswahlen des Benutzers merken.
// Falls kein Suffix angegeben worden war, diesen aus nFilterIndex generieren.
// --------------------------------------------------------------------------------------------------------------------------------------------
void CFileDialog::ExitOfn()
{
	TCHAR ext[MY_MAX_EXT];

	m_nDefaultFilter = (int)m_Ofn.nFilterIndex;
	_tsplitpath_s(m_szPath, NULL, 0, NULL, 0, NULL, 0, ext, _tcschars(ext));

	if (*ext == _T('\0') && m_pchFilter)
	{
		// Finde die EXT für nFilterIndex:
		int i = 0;
		for (DWORD n = 1; n < m_Ofn.nFilterIndex * 2; n++)
		{
			while (m_pchFilter[i++]);
		}

		while (m_pchFilter[i] == _T('*'))
			i++;
		_tcscat(m_szPath, &m_pchFilter[i]);
	}

	m_strPath = m_szPath;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CFileDialog::GetOpenName()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CFileDialog::GetOpenName(HWND hwnd_owner)
{
	BOOL rc;

	InitOfn(hwnd_owner);
	rc = GetOpenFileName(&m_Ofn);
	if (rc)
		ExitOfn();

	return rc != 0;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CFileDialog::GetSaveName()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CFileDialog::GetSaveName(HWND hwnd_owner)
{
	BOOL rc;

	InitOfn(hwnd_owner);
	rc = GetSaveFileName(&m_Ofn);
	if (rc)
		ExitOfn();

	return rc != 0;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														SplitExtension()
//
// Extract extension, including the '.'.
// Returns a malloc'ed string which must be freed by the caller.
// Supplied parameter is full path and file name.
// --------------------------------------------------------------------------------------------------------------------------------------------
static TCHAR *SplitExtension(LPCTSTR pfn)
{
	size_t i;

	if (!pfn)
		return NULL;

	size_t len = _tcslen(pfn);
	for (i = len; i > 0 && pfn[i] != _T('.'); i--);

	if (i == 0)
		return NULL;	// no extension found

	TCHAR *ext = (TCHAR *)malloc(_tcssize(len - i + 1));
	_tcsncpy(ext, pfn + i, len - i + 1);

	return ext;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//												CFileDialog::SetFilterIndexFromFileName()
//
// Analyses the file name extension and sets the DefaultFilterIndex accordingly
// --------------------------------------------------------------------------------------------------------------------------------------------
void CFileDialog::SetFilterIndexFromFileName()
{
	int i = 1;

	m_nDefaultFilter = 1;

	if (m_pchFilter && *m_pchFilter)
	{
		LPCTSTR p = m_pchFilter;
		TCHAR *ext = SplitExtension(m_szPath);

		if (ext)
		{
			// compare ext with extensions in Filter
			while (*p)
			{
				while (*p != 0)
					p++;

				p++;

				TCHAR *filter_ext = SplitExtension(p);

				if (filter_ext)
				{
					if (_tcsicmp(filter_ext, ext) == 0)
					{
						m_nDefaultFilter = i;
						free(filter_ext);
						break;
					}

					free(filter_ext);
				}

				i++;
				while (*p != 0)
					p++;

				p++;
			}

			free(ext);
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//																ThereIsAFullscreenWin()
// --------------------------------------------------------------------------------------------------------------------------------------------
static bool IsTopMostWindow(HWND hwnd)
{
	WINDOWINFO info;
	GetWindowInfo(hwnd, &info);
	return (info.dwExStyle & WS_EX_TOPMOST) ? true : false;
}

static bool IsFullScreenSize(HWND hwnd, const int cx, const int cy)
{
	RECT r;
	::GetWindowRect(hwnd, &r);
	return r.right - r.left == cx && r.bottom - r.top == cy;
}

bool IsFullscreenAndMaximized(HWND hwnd)
{
	if (IsWindowVisible(hwnd) && IsTopMostWindow(hwnd))
	{
		const int cx = GetSystemMetrics( SM_CXSCREEN );
		const int cy = GetSystemMetrics( SM_CYSCREEN );
		if (IsFullScreenSize(hwnd, cx, cy))
			return true;
	}
	return false;
}

BOOL CALLBACK CheckMaximized( HWND hwnd, LPARAM lParam )
{
	if (hwnd == GetDesktopWindow() || hwnd == GetShellWindow())
		return TRUE;

	if (IsFullscreenAndMaximized(hwnd))
	{
		*(bool *)lParam = true;
		return FALSE; //there can be only one so quit here
	}
	return TRUE;
}

bool ThereIsAFullscreenWin()
{
	bool bThereIsAFullscreenWin = false;
	EnumWindows( (WNDENUMPROC) CheckMaximized, (LPARAM) &bThereIsAFullscreenWin );

	return bThereIsAFullscreenWin;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//																GetTaskbarRect()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool GetTaskbarRect(RECT &rc)
{
	HWND hTaskbarWnd = FindWindow(_T("Shell_TrayWnd"), NULL);
	if (!hTaskbarWnd)
		return false;

	GetWindowRect(hTaskbarWnd, &rc);
	return true;
}
