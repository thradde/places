
#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#include <Shlobj.h>

#include "memdebug.h"

#include <SFML/Graphics.hpp>

#include <vector>
#include <set>
using namespace std;

#define ASSERT
#include "RfwString.h"
#include "exceptions.h"
#include "Mutex.h"
#include "stream.h"
#include "platform.h"
#include "generic.h"
#include "configuration.h"
#include "md5.h"
#include "bitmap_cache.h"
#include "icon.h"
#include "icon_page.h"
#include "undo.h"
#include "icon_manager.h"


// --------------------------------------------------------- CIconInsertAction ---------------------------------------------------------
void CIconInsertAction::PerformUndo(CIconManager &icon_manager, int pass)
{
	icon_manager.UnlinkIcon(m_pIcon);
}


void CIconInsertAction::PerformRedo(CIconManager &icon_manager, int pass)
{
	icon_manager.PlaceIcon(m_pIcon, m_pIcon->GetPosition());
}

void CIconInsertAction::Release(CIconManager &icon_manager, bool is_undo_action)
{
	if (!is_undo_action)
	{
		icon_manager.UnrefIcon(m_pIcon);
		delete m_pIcon;
		m_pIcon = NULL;
	}
}


// --------------------------------------------------------- CIconDeleteAction ---------------------------------------------------------
void CIconDeleteAction::PerformUndo(CIconManager &icon_manager, int pass)
{
	icon_manager.PlaceIcon(m_pIcon, m_pIcon->GetPosition());
}


void CIconDeleteAction::PerformRedo(CIconManager &icon_manager, int pass)
{
	icon_manager.UnlinkIcon(m_pIcon);
}

void CIconDeleteAction::Release(CIconManager &icon_manager, bool is_undo_action)
{
	if (is_undo_action)
	{
		icon_manager.UnrefIcon(m_pIcon);
		delete m_pIcon;
		m_pIcon = NULL;
	}
}


// --------------------------------------------------------- CIconMoveAction ---------------------------------------------------------
void CIconMoveAction::PerformUndo(CIconManager &icon_manager, int pass)
{
	if (pass == 0)
	{
		// we must unlink the whole group in a first pass, otherwise some icons of the same
		// group might still occupy a position, to which other icons should be moved back
		icon_manager.UnlinkIcon(m_pIcon);
	}
	else
	{
		// after all icons have been unlinked, move the whole group in a second pass to the new position
		CIconPos cur_pos(m_pIcon->GetPosition());
		icon_manager.PlaceIcon(m_pIcon, m_OldPosition);
		m_OldPosition = cur_pos;	// for inverted redo action
	}
}


// --------------------------------------------------------- CIconPropertiesAction ---------------------------------------------------------
void CIconPropertiesAction::PerformUndo(CIconManager &icon_manager, int pass)
{
	// title
	RString tmp = m_pIcon->GetFullTitle();
	if (tmp != m_strOldTitle)
	{
		m_pIcon->SetFullTitle(m_strOldTitle);
		m_strOldTitle = tmp;		// for inverted redo action
	}

	/*
	// use shell api
	bool b = m_pIcon->m_bUseShellApi;
	if (b != m_bOldUseShellApi)
	{
		m_pIcon->m_bUseShellApi = m_bOldUseShellApi;
		m_bOldUseShellApi = b;
	}
	*/

	// icon path
	tmp = m_pIcon->GetIconPath();
	if (tmp != m_strOldIconPath)
	{
		m_pIcon->SetIconPath(icon_manager.GetBitmapCache(), m_strOldIconPath);
		m_strOldIconPath = tmp;		// for inverted redo action
	}

	// file path
	tmp = m_pIcon->GetFilePath();
	if (tmp != m_strOldFilePath)
	{
		m_pIcon->SetFilePath(icon_manager.GetBitmapCache(), m_strOldFilePath);
		m_strOldFilePath = tmp;		// for inverted redo action
	}

	// parameters
	tmp = m_pIcon->m_strParameters;
	if (tmp != m_strOldParameters)
	{
		m_pIcon->m_strParameters = m_strOldParameters;
		m_strOldParameters = tmp;	// for inverted redo action
	}

	// open as admin
	bool b = m_pIcon->m_bOpenAsAdmin;
	if (b != m_bOldOpenAsAdmin)
	{
		m_pIcon->m_bOpenAsAdmin = m_bOldOpenAsAdmin;
		m_bOldOpenAsAdmin = b;
	}
}
