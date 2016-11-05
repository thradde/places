
#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>

#include <SFML/Graphics.hpp>

//#include <tchar.h>

#include <vector>
using namespace std;

#define ASSERT
#include "RfwString.h"
#include "platform.h"
#include "generic.h"
#include "popup_menu.h"


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CPopupMenu::GetItemMenu()
// --------------------------------------------------------------------------------------------------------------------------------------------
CPopupMenu *CPopupMenu::GetItemMenu(long idx, long &idxInMenu, long &offsetIdx, CPopupMenu *pMenu)
{
	long oldIDx = offsetIdx;
	offsetIdx += (long)pMenu->GetItemCount();

	if (idx < offsetIdx)
	{
		idxInMenu = idx - oldIDx;
		return pMenu;
	}

	CPopupMenu *menu = NULL;

	for (int i = 0; i < pMenu->GetItemCount() && menu == NULL; i++)
	{
		CPopupMenuItem *menuItem = pMenu->GetItem(i);
		if (menuItem->GetSubmenu())
			menu = GetItemMenu(idx, idxInMenu, offsetIdx, menuItem->GetSubmenu());
	}

	return menu;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CPopupMenu::CreateMenu()
// --------------------------------------------------------------------------------------------------------------------------------------------
HMENU CPopupMenu::CreateMenu(CPopupMenu *pMenu, long &offsetIdx)
{
	HMENU hMenu = ::CreatePopupMenu();

	int flags = 0;
	long idxSubmenu = 0;
	long offset = offsetIdx;
	long nItems = (long)pMenu->GetItemCount();
	offsetIdx += nItems;
	long inc = 0;

	for (int i = 0; i < nItems; i++)
	{
		CPopupMenuItem *menuItem = pMenu->GetItem(i);

		if (menuItem->GetIsSeparator())
		{
			AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
		}
		else
		{
			flags = MF_STRING;

			if (menuItem->GetSubmenu())
			{
				HMENU submenu = CreateMenu(menuItem->GetSubmenu(), offsetIdx);
				if (submenu)
					AppendMenu(hMenu, flags|MF_POPUP|MF_ENABLED, (UINT_PTR)submenu, (const TCHAR *)menuItem->GetText().c_str());
			}
			else
			{
				if (menuItem->GetEnabled())
					flags |= MF_ENABLED;
				else
					flags |= MF_GRAYED;

				if (menuItem->GetIsTitle())
					flags |= MF_DISABLED;

				if (menuItem->GetChecked())
					flags |= MF_CHECKED;
				else
					flags |= MF_UNCHECKED;

				AppendMenu(hMenu, flags, offset + inc, (const TCHAR *)menuItem->GetText().c_str());
			}
		}

		inc++;
	}

	return hMenu;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CPopupMenu::CreatePopupMenu()
// --------------------------------------------------------------------------------------------------------------------------------------------
CPopupMenu *CPopupMenu::CreatePopupMenu(HWND hwnd, int x, int y)
{
	long offsetIdx = 0;
	HMENU hMenu = CreateMenu(this, offsetIdx);
	CPopupMenu *result = 0;

	if (hMenu)
	{
		POINT cPos;
		cPos.x = x;
		cPos.y = y;
		ClientToScreen(hwnd, &cPos);

		if (TrackPopupMenu(hMenu,
			TPM_LEFTALIGN,
			cPos.x,
			cPos.y,
			0,
			hwnd,
			0))
		{
			MSG msg;

			// Eat WM_LBUTTONDOWN that might have been caused by a cancel-click into the app's client area, otherwise they
			// it'll be received by the main window and cause a hide-window action
			while (PeekMessage(&msg, hwnd, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
				; // NOP

			if (PeekMessage(&msg, hwnd, WM_COMMAND, WM_COMMAND, PM_REMOVE))
			{
				if (HIWORD(msg.wParam) == 0)
				{
					long res = LOWORD(msg.wParam);
					if (res != -1)
					{
						long idx = 0;
						offsetIdx = 0;
						CPopupMenu *resultMenu = GetItemMenu(res, idx, offsetIdx, this);
						if (resultMenu)
						{
							result = resultMenu;
							result->SetChosenItemIndex(idx);
						}
					}
				}
			}
		}

		DestroyMenu(hMenu);
	}

	return result;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CPopupMenu::DoPopupMenu()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CPopupMenu::DoPopupMenu(HWND hwnd, int x, int y)
{
	CPopupMenu *selectedMenu = CreatePopupMenu(hwnd, x, y);

	m_strSelectedMenu.erase();
	m_strSelectedEntry.erase();

	if (selectedMenu)
	{
		m_strSelectedMenu = selectedMenu->GetName();

		int chosenItemIndex = selectedMenu->GetChosenItemIndex();
		m_strSelectedEntry = selectedMenu->GetItemText(chosenItemIndex);
		return true;
	}

	return false;
}
