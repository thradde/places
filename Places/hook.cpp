

#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#include <Commctrl.h>
#include <tchar.h>
#include <process.h>

#include "memdebug.h"

#include "hook.h"


enum EMouseCorner		// (currently unused)
{
	enMcNone,
	enMcLeftTop,
	enMcRightTop,
	enMcLeftBottom,
	enMcRightBottom,
};

#define MIN_MOUSE_HOVER_TIME	1250	// 1250 ms


// --------------------------------------------------------------------------------------------------------------------------------------------
//															Globals
// --------------------------------------------------------------------------------------------------------------------------------------------
BYTE gbHotkeyVKey;				// virtual key code for app activation
BYTE gbHotkeyModifiers;			// modifiers for app activation (shift, ctrl, alt)

bool gbAltKeyDown;
bool gbCtrlKeyDown;
bool gbShiftKeyDown;
bool gbWinKeyDown;

int gnMouseButton;				// mouse button that activates: 0 = none, 1 = middle, 2 = xbutton1, 3 = xbutton2
int gnMouseCorner;				// if moving the mouse into the left-top corner will activate: 0 = no, 1 = yes
bool gbSuspendHooks;			// de-activate keyboard and mouse hooks, e.g. when gaming
EMouseCorner genMouseCorner;	// corner of screen that activates app when mouse is moved there (currently unused)

volatile bool gbHookEvent;			// true, if there was an event that did interest us.
									// This is either a mouse button event, a keyboard event or a progman event
									// After reading out "true", the consumer must set this flag = false
volatile bool gbEatSingleClick;		// same as above for mouse single click (used for checking if app was activated)
volatile bool gbHookDoubleClick;	// same as above for mouse double click (used for checking for double click on desktop)

static HWND		_hWndApp = NULL;
static HINSTANCE _hInstance = NULL;
static int		gnDoubleClickTime;	// user preferences value from registry
static __int64	gnDblClickStartTime;
static __int64	gnPerformanceFreq;

static HHOOK	_kbd_hook;
static HHOOK	_mouse_hook;

static RECT		_rcHoverArea;				// if mouse pointer hovers over this area, it will trigger the gbHookEvent
static __int64	_nMouseHoverStartTime = 0;	// start time for test of mouse hovering


inline bool IsAltKeyDown()
{
	return (GetKeyState(VK_MENU) & 0x80) != 0;
}

inline bool IsCtrlKeyDown()
{
	return (GetKeyState(VK_CONTROL) & 0x80) != 0;
}

inline bool IsShiftKeyDown()
{
	return (GetKeyState(VK_SHIFT) & 0x80) != 0;
}

inline bool IsWinKeyDown()
{
	return (GetKeyState(VK_LWIN) & 0x80) || (GetKeyState(VK_RWIN) & 0x80);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															KbdHookCallback()
// --------------------------------------------------------------------------------------------------------------------------------------------
LRESULT __stdcall KbdHookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (!gbSuspendHooks && gbHotkeyVKey != 0 && nCode >= 0)
    {
        if (wParam == WM_KEYDOWN)
        {
            KBDLLHOOKSTRUCT *kbdStruct = ((KBDLLHOOKSTRUCT *)lParam);
            if ((BYTE)(kbdStruct->vkCode & 0xff) == gbHotkeyVKey)
			{
				if ((gbHotkeyModifiers & HotkeyAlt) && !IsAltKeyDown())
					goto bailout;

				if ((gbHotkeyModifiers & HotkeyCtrl) && !IsCtrlKeyDown())
					goto bailout;

				if ((gbHotkeyModifiers & HotkeyShift) && !IsShiftKeyDown())
					goto bailout;

				if ((gbHotkeyModifiers & HotkeyWin) && !IsWinKeyDown())
					goto bailout;

				gbHookEvent = true;
				return 1;
			}
        }
    }
 
bailout:
    return CallNextHookEx(_kbd_hook, nCode, wParam, lParam);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															HookTestMouseHover()
//
// Must be called periodically by main program to check for mouse hovering in a screen corner.
// --------------------------------------------------------------------------------------------------------------------------------------------
void HookTestMouseHover()
{
	if (_nMouseHoverStartTime != 0)
	{
		__int64 now, elapsed;
		QueryPerformanceCounter((LARGE_INTEGER *)&now);
		elapsed = (now - _nMouseHoverStartTime) / gnPerformanceFreq;

		// min time hovered?
		if (elapsed >= MIN_MOUSE_HOVER_TIME)
		{
			gbHookEvent = true;
			_nMouseHoverStartTime = 0;
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															MouseHookCallback()
// --------------------------------------------------------------------------------------------------------------------------------------------
LRESULT __stdcall MouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (!gbSuspendHooks && nCode >= 0)
    {
		MSLLHOOKSTRUCT *mouseStruct = ((MSLLHOOKSTRUCT *)lParam);

		POINT pt;
		pt.x = mouseStruct->pt.x;
		pt.y = mouseStruct->pt.y;

		bool IsNotForegroundWindow = GetForegroundWindow() != _hWndApp;

		if (wParam == WM_MOUSEMOVE && gnMouseCorner != 0)
		{
            if (pt.x >= _rcHoverArea.left && pt.x <= _rcHoverArea.right &&
				pt.y >= _rcHoverArea.top  && pt.y <= _rcHoverArea.bottom &&
				IsNotForegroundWindow)
			{
				QueryPerformanceCounter((LARGE_INTEGER *)&_nMouseHoverStartTime);
			}
			else
				_nMouseHoverStartTime = 0;
		}
		else if ((wParam == WM_MBUTTONUP || wParam == WM_MBUTTONDOWN) && gnMouseButton == 1)
		{
			_nMouseHoverStartTime = 0;
			if (wParam == WM_MBUTTONUP)
				gbHookEvent = true;
			//return 1;	// we must eat BUTTONDOWN, otherwise the mouse input of this app hangs
		}
        else if ((wParam == WM_XBUTTONUP || wParam == WM_XBUTTONDOWN) && gnMouseButton >= 2 &&
				 (HIWORD(mouseStruct->mouseData) & (gnMouseButton - 1)))
        {
			_nMouseHoverStartTime = 0;
			if (wParam == WM_XBUTTONUP)
				gbHookEvent = true;
			//return 1;	// we must eat BUTTONDOWN, otherwise the mouse input of this app hangs
        }
		else if (wParam == WM_LBUTTONDOWN && IsNotForegroundWindow)	// if we are in foreground, bail out
		{
			_nMouseHoverStartTime = 0;

			// was it our own window?
			if (/* IsNotForegroundWindow && */ WindowFromPoint(pt) == _hWndApp)
				gbEatSingleClick = true;

			// Can not receive WM_LBUTTONDBLCLK through this low-level hook! So we synthesize it ourselves.
			if (gnDblClickStartTime == 0)
				QueryPerformanceCounter((LARGE_INTEGER *)&gnDblClickStartTime);
			else
			{
				__int64 now, elapsed;
				QueryPerformanceCounter((LARGE_INTEGER *)&now);
				elapsed = (now - gnDblClickStartTime) / gnPerformanceFreq;
				if (elapsed <= gnDoubleClickTime)
				{
					gbHookDoubleClick = true;
					gnDblClickStartTime = 0;
				}
				else
					QueryPerformanceCounter((LARGE_INTEGER *)&gnDblClickStartTime);
			}
		}
    }
 
    return CallNextHookEx(_mouse_hook, nCode, wParam, lParam);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															SetHookAppWindow()
// --------------------------------------------------------------------------------------------------------------------------------------------
void SetHookAppWindow(HWND hWndApp)
{
	_hWndApp = hWndApp;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															SetHookInstance()
// --------------------------------------------------------------------------------------------------------------------------------------------
void SetHookInstance(HINSTANCE hInstance)
{
	_hInstance = hInstance;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//																SetHooks()
// --------------------------------------------------------------------------------------------------------------------------------------------
static bool SetHooks()
{
	// setup variables
	_hWndApp = NULL;

	gbHotkeyVKey = 123;			// F12
	gbHotkeyModifiers = HotkeyCtrl;
	gnMouseButton = 1;			// middle mouse-button
	gnMouseCorner = 1;			// yes
	gbSuspendHooks = false;

	gbHookEvent = false;
	gbEatSingleClick = false;
	gbHookDoubleClick = false;

	gnDoubleClickTime = GetDoubleClickTime();
	QueryPerformanceFrequency((LARGE_INTEGER *)&gnPerformanceFreq);
	gnPerformanceFreq /= 1000;	// we want milliseconds later
	gnDblClickStartTime = 0;

	// GetSystemMetrics(SM_CXSCREEN / SM_CYSCREEN) return bogus values, if screen scaling is set.
	// Solution: Include manifest file, which tells Windows that this app is DPI aware.
	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);
	_rcHoverArea.left = width - 30;		// was -5
	_rcHoverArea.right = width + 10;
	_rcHoverArea.top = -10;
	_rcHoverArea.bottom = 30;			// was 5

	// create hooks
    if (!(_kbd_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KbdHookCallback, _hInstance, 0)))
		return false;

	if (!(_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookCallback, _hInstance, 0)))
		return false;

	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															ReleaseHooks()
// --------------------------------------------------------------------------------------------------------------------------------------------
static void ReleaseHooks()
{
    UnhookWindowsHookEx(_kbd_hook);
    UnhookWindowsHookEx(_mouse_hook);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															RunHookThread()
//
// The hooks are run in a separate thread, because they need a fast message loop.
// The SFML message loop is rather slow, because it paints a lot of stuff (and we have a Sleep() in  it).
// Using the SFML message loop, one can notice a significant slow-down, when AutoHotkey sends a group of single keystrokes.
// Using this thread, the slow-down is gone.
// There is no cleanup code for the thread, when the main app is terminated, the thread and hooks are terminated cleanly by Windows.
// --------------------------------------------------------------------------------------------------------------------------------------------
static HANDLE _hHookThread;

DWORD WINAPI HookThread(void *param)
{
	SetHooks();

#ifndef _DEBUG
   MSG msg;
   while (GetMessage(&msg, NULL, 0, 0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif

   ReleaseHooks();

   return 1;
}


void RunHookThread()
{
	DWORD aID;

	if (_hHookThread = CreateThread(NULL, 8 * 1024, HookThread, NULL, 0, &aID))
	{
		// boost thread priority, otherwise mouse lags under certain conditions.
		// especially when plugging-in USB devices, the mouse can lag so much that Windows stops this program!
		SetThreadPriority(_hHookThread, THREAD_PRIORITY_TIME_CRITICAL);
	}
}
