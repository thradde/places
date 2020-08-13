
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

// Hotkey Modifiers
enum
{
	HotkeyAlt = (1 << 0),
	HotkeyCtrl = (1 << 1),
	HotkeyShift = (1 << 2),
	HotkeyWin = (1 << 3),
};

extern BYTE gbHotkeyVKey;		// virtual key code for app activation
extern BYTE gbHotkeyModifiers;	// modifiers for app activation (shift, ctrl, alt, win)
extern int gnMouseButton;		// mouse button that activates: 0 = none, 1 = middle, 2 = xbutton1, 3 = xbutton2
extern int gnMouseCorner;		// if moving the mouse into the left-top corner will activate: 0 = no, 1 = yes

extern bool gbSuspendHooks;		// de-activate keyboard and mouse hooks, e.g. when gaming

extern volatile bool gbHookEvent;		// true, if there was an event that did interest us.
										// This is either a mouse button event, a keyboard event or a progman event
										// After reading out "true", the consumer must set this flag = false
extern volatile bool gbEatSingleClick;	// same as above for mouse single click (used for checking if app was activated)
extern volatile bool gbHookDoubleClick;	// same as above for mouse double click (used for checking for double click on desktop)


void SetHookAppWindow(HWND hWndApp);
void SetHookInstance(HINSTANCE hInstance);
bool SetHooks();
void ReleaseHooks();
void HookTestMouseHover();		// Must be called periodically by main program to check for mouse hovering in a screen corner.
void RunHookThread();
