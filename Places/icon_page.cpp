
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

#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#include <Shlobj.h>

#include "memdebug.h"

#include <SFML/Graphics.hpp>

#include <vector>
#include <unordered_set>
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


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIconPage::CIconPage()
// --------------------------------------------------------------------------------------------------------------------------------------------
CIconPage::CIconPage(CIconPage &&page)
{
	m_nCols = page.m_nCols;
	m_nRows = page.m_nRows;

	m_arIcons = page.m_arIcons;
	page.m_arIcons = nullptr;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::CIconPage(Stream)
// --------------------------------------------------------------------------------------------------------------------------------------------
CIconPage::CIconPage(Stream &stream)
{
	// cols and rows
	m_nCols = stream.ReadInt();
	m_nRows = stream.ReadInt();

	// icons
	int count = m_nCols * m_nRows;
	m_arIcons = new CIcon*[count];
	for (int i = 0; i < count; i++)
	{
		BYTE has_data = stream.ReadByte();	// is icon data following?
		if (has_data)
			m_arIcons[i] = new CIcon(stream);
		else
			m_arIcons[i] = nullptr;
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::Write()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::Write(Stream &stream) const
{
	// cols and rows
	stream.WriteInt(m_nCols);
	stream.WriteInt(m_nRows);

	// icons
	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i])
		{
			stream.WriteByte(1);	// indicate that icon data is following
			m_arIcons[i]->Write(stream);
		}
		else
			stream.WriteByte(0);	// indicate that no icon data is following
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIconPage::Find()
// --------------------------------------------------------------------------------------------------------------------------------------------
CRasterPos CIconPage::Find(const CIcon *icon, bool &bFail)
{
	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i] == icon)
		{
			bFail = false;
			return ComputeRasterPos(i);
		}
	}

	bFail = true;
	return CRasterPos(0, 0);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIconPage::SetDimensions()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::SetDimensions(int cols, int rows)
{
	// todo: add code, to handle when m_arIcons is not nullptr here
	//       (ie move existing items to new array in a meaningful way regarding the layout)
	Free();

	m_nCols = cols;
	m_nRows = rows;

	int count = m_nCols * m_nRows;
	m_arIcons = new CIcon*[count];
	for (int i = 0; i < count; i++)
		m_arIcons[i] = nullptr;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIconPage::GetItem()
// --------------------------------------------------------------------------------------------------------------------------------------------
CIcon *CIconPage::GetItem(const CRasterPos &pos, bool &bFail)
{
	if (pos.m_nCol < 0 || pos.m_nCol >= m_nCols ||
		pos.m_nRow < 0 || pos.m_nRow >= m_nRows)
	{
		bFail = true;
		return nullptr;
	}

	bFail = false;
	return m_arIcons[pos.m_nRow * m_nCols + pos.m_nCol];
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIconPage::SetItem()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::SetItem(const CRasterPos &pos, CIcon *icon)
{
	if (pos.m_nCol < 0 || pos.m_nCol >= m_nCols)
		return;

	if (pos.m_nRow < 0 || pos.m_nRow >= m_nRows)
		return;

	m_arIcons[pos.m_nRow * m_nCols + pos.m_nCol] = icon;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::RecomputeAllIcons()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::RecomputeAllIcons(int pixels, int nPageNum, int nHorizontalMargin, int nVerticalMargin, int nIconRasterX, int nIconRasterY)
{
	float x_start = (float)(nPageNum * gConfig.m_nWindowWidth + nHorizontalMargin + nIconRasterX / 2);
	float y_pos = (float)nVerticalMargin + nIconRasterY / 2;

	int i = 0;
	for (int y = 0; y < m_nRows; y++)
	{
		float x_pos = x_start;

		for (int x = 0; x < m_nCols; x++)
		{
			if (m_arIcons[i])
			{
				m_arIcons[i]->CreateIcon(pixels, false);
				m_arIcons[i]->SetSpritePosition(x_pos, y_pos);
				m_arIcons[i]->RenderTitleTextLayout();
			}

			x_pos += nIconRasterX;
			i++;
		}

		y_pos += nIconRasterY;
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::CreateStateSnapshot()
//
// this is used during lasso-operation to backup the initial icon states before the lasso-op takes place
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::CreateStateSnapshot()
{
	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i])
			m_arIcons[i]->DoBackupState();
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::RestoreStateSnapshot()
//
// this is used when a move operation is canceled
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::RestoreStateSnapshot(CIconManager &icon_manager)
{
	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i])
		{
			if (m_arIcons[i]->SetState(m_arIcons[i]->GetBackupState()))
				icon_manager.AddToAnimationList(m_arIcons[i]);
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::CreateSelectedSnapshot()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::CreateSelectedSnapshot(TListIcons &listSelectedIcons)
{
	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i] && m_arIcons[i]->GetState() == CIcon::enStateHighlight)
			listSelectedIcons.push_back(m_arIcons[i]);
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::HasSelectedIcons()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIconPage::HasSelectedIcons() const
{
	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i] && m_arIcons[i]->GetState() == CIcon::enStateHighlight)
			return true;
	}

	return false;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::Select()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::Select(CIconManager &icon_manager, bool do_select, const sf::FloatRect &rc)
{
	CIcon::EIconState new_state = CIcon::enStateNormal;
	if (do_select)
		new_state = CIcon::enStateHighlight;

	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i] && rc.contains(m_arIcons[i]->GetSpritePosition()))
		{
			if (m_arIcons[i]->SetState(new_state))
				icon_manager.AddToAnimationList(m_arIcons[i]);
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::SelectAll()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::SelectAll(CIconManager &icon_manager, bool do_select)
{
	CIcon::EIconState new_state = CIcon::enStateNormal;
	if (do_select)
		new_state = CIcon::enStateHighlight;

	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i])
		{
			if (m_arIcons[i]->SetState(new_state))
				icon_manager.AddToAnimationList(m_arIcons[i]);
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::LassoSelect()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::LassoSelect(CIconManager &icon_manager, bool do_select, const sf::FloatRect &rc)
{
	CIcon::EIconState new_state = CIcon::enStateNormal;
	if (do_select)
		new_state = CIcon::enStateHighlight;

	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i])
		{
			if (rc.contains(m_arIcons[i]->GetSpritePosition()))
			{
				if (m_arIcons[i]->SetState(new_state))
					icon_manager.AddToAnimationList(m_arIcons[i]);
			}
			else
			{
				CIcon::EIconState backup_state = m_arIcons[i]->GetBackupState();
				if (m_arIcons[i]->SetState(backup_state))
					icon_manager.AddToAnimationList(m_arIcons[i]);
			}
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::SetAllIconStates()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::SetAllIconStates(CIconManager &icon_manager, CIcon::EIconState state)
{
	CreateStateSnapshot();	// backup states, so they can be restored when operation is canceled

	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i])
		{
			if (m_arIcons[i]->SetState(state))
				icon_manager.AddToAnimationList(m_arIcons[i]);
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::ResyncIcons()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIconPage::ResyncIcons(CBitmapCache &bitmap_cache, bool &resync_running, bool &changed_an_icon)
{
	bool did_resync_one = false;

	int count = m_nCols * m_nRows;
	for (int i = 0; i < count && resync_running; i++)
	{
		if (m_arIcons[i] && !m_arIcons[i]->IsResynced())
		{
			if (m_arIcons[i]->Resync(bitmap_cache))
				changed_an_icon = true;
			did_resync_one = true;
			Sleep(10);
		}
	}

	return did_resync_one;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::ClearIconsResyncFlag()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::ClearIconsResyncFlag()
{
	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i])
			m_arIcons[i]->m_bResynced = false;
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::CanChangeLayout()
//
// checks, if a new layout can be applied to each page, so that no icons
// would fall outside the new layout
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIconPage::CanChangeLayout(const CSettings &new_layout) const
{
	// check columns
	for (int i = new_layout.m_nColumns; i < m_nCols; i++)
	{
		for (int n = 0; n < m_nRows; n++)
		{
			if (m_arIcons[n * m_nCols + i] != nullptr)
				return false;
		}
	}

	// check rows
	for (int i = new_layout.m_nRows; i < m_nRows; i++)
	{
		for (int n = 0; n < m_nCols; n++)
		{
			if (m_arIcons[i * m_nCols + n] != nullptr)
				return false;
		}
	}

	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::ChangeLayout()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::ChangeLayout(const CSettings &new_layout)
{
	int count = new_layout.m_nColumns * new_layout.m_nRows;
	
	CIcon **arNew = new CIcon*[count];
	memset(arNew, 0, count * sizeof(CIcon *));

	int x_end = min(new_layout.m_nColumns, m_nCols);
	int y_end = min(new_layout.m_nRows, m_nRows);

	for (int x = 0; x < x_end; x++)
	{
		for (int y = 0; y < y_end; y++)
		{
			arNew[y * new_layout.m_nColumns + x] = m_arIcons[y * m_nCols + x];
		}
	}

	delete[] m_arIcons;
	m_arIcons = arNew;
	m_nCols = new_layout.m_nColumns;
	m_nRows = new_layout.m_nRows;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconPage::RenderTitles()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconPage::RenderTitles()
{
	int count = m_nCols * m_nRows;
	for (int i = 0; i < count; i++)
	{
		if (m_arIcons[i])
			m_arIcons[i]->RenderTitleTextLayout();
	}
}
