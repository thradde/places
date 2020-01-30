
#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>

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


#define PLACES_DB_MAGIC		_T("PlacesDB")
#define PLACES_DB_VERSION		1


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIconManager::AddNewPage()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::AddNewPage()
{
	CIconPage page;
	page.SetDimensions(m_nMaxCols, m_nMaxRows);
	m_IconPages.push_back(std::move(page));

	// for stl::vector any push / pop, insert, erase, ... will invalidate m_itCurPage
	GotoPage(m_nCurPageNum);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIconManager::Init()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::Init(const CConfig &config, bool create_first_empty_page)
{
	m_nHorizontalMargin = config.m_nHorizontalMargin;
	m_nVerticalMargin = config.m_nVerticalMargin;
	m_nIconRasterX = config.m_nIconTotalWidth;
	m_nIconRasterY = config.m_nIconTotalHeight;

	m_nMaxCols = config.m_nColumns;
	m_nMaxRows = config.m_nRows;

	if (create_first_empty_page && m_IconPages.empty())
	{
		AddNewPage();
		m_nCurPageNum = 0;
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::IsValidPos()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIconManager::IsValidPos(int screen_x, int screen_y)
{
	// take page number into account
	screen_x -= m_nCurPageNum * gConfig.m_nWindowWidth;

	int col = (screen_x - m_nHorizontalMargin) / m_nIconRasterX;
	int row = (screen_y - m_nVerticalMargin) / m_nIconRasterY;

	if (col < 0 || col >= m_nMaxCols)
		return false;

	if (row < 0 || row >= m_nMaxRows)
		return false;

	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::ComputeRasterPos()
// --------------------------------------------------------------------------------------------------------------------------------------------
CRasterPos CIconManager::ComputeRasterPos(int screen_x, int screen_y)
{
	// take page number into account
	screen_x -= m_nCurPageNum * gConfig.m_nWindowWidth;

	// Tricky:	The origin of the sprites is at xy + gConfig.m_nIconSize / 2
	//          AND we want to achieve some positional "rounding", i.e. the border between raster lines
	//          is set virtually to be *in the middle between* two raster lines (not at the top / bottom).
	//			So we need to add 2 * gConfig.m_nIconSize / 2 = gConfig.m_nIconSize
//	screen_x -= gConfig.m_nIconSizeX;
//	screen_y -= gConfig.m_nIconSizeY;
//	screen_x += gConfig.m_nIconTotalWidth_2;
//	screen_y += gConfig.m_nIconTotalHeight_2;

	int col = (screen_x - m_nHorizontalMargin) / m_nIconRasterX;
	int row = (screen_y - m_nVerticalMargin) / m_nIconRasterY;

	if (col < 0)
		col = 0;
	else if (col >= m_nMaxCols)
		col = m_nMaxCols - 1;

	if (row < 0)
		row = 0;
	else if (row >= m_nMaxRows)
		row = m_nMaxRows - 1;

	return CRasterPos(col, row);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::FindFreePosition()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIconManager::FindFreePosition(CRasterPos &pos)
{
	// todo: try next page, if this page is full

	int count = m_itCurPage->GetCount();
	int index = m_itCurPage->ComputeIndex(pos);
	while (index < count)
	{
		if (m_itCurPage->GetItem(index) == nullptr)
		{
			pos = m_itCurPage->ComputeRasterPos(index);
			return true;	// found free position
		}

		index++;
	}

	// not found, scan backwards
	index = m_itCurPage->ComputeIndex(pos) - 1;
	while (index >= 0)
	{
		if (m_itCurPage->GetItem(index) == nullptr)
		{
			pos = m_itCurPage->ComputeRasterPos(index);
			return true;	// found free position
		}

		index--;
	}

	return false;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::Add()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::Add(CIcon *icon, int screen_x, int screen_y)
{
	// compute nearest raster position
	CRasterPos pos = ComputeRasterPos(screen_x, screen_y);

	if (!FindFreePosition(pos))
	{
		// no free space anywhere
		delete icon;
		return;
	}

	m_itCurPage->SetItem(pos, icon);
	icon->SetPosition(CIconPos(pos, m_nCurPageNum));
	//icon->SetSpritePosition((float)(screen_x + gConfig.m_nIconSizeX_2), (float)(screen_y + gConfig.m_nIconSizeY_2));
	icon->SetSpritePosition((float)screen_x, (float)screen_y);
	InitIconMoveAnimation(icon, Col2Screen(pos.m_nCol), Row2Screen(pos.m_nRow));
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::UnlinkIcon()
//
// unlink icon from the page's m_Icons array
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::UnlinkIcon(CIcon *icon)
{
	m_IconPages[icon->GetPosition().m_nPage].SetItem(icon->GetPosition().m_RasterPos, nullptr);
	icon->SetIsUnlinked(true);

	/*bool bFail;
	CRasterPos pos = m_itCurPage->Find(icon, bFail);
	m_itCurPage->SetItem(pos, nullptr);
	icon->SetIsUnlinked(true);*/
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::PlaceIcon()
//
// store icon at target position (target page may be any page)
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIconManager::PlaceIcon(CIcon *icon, const CIconPos &pos)
{
	// fail, if new position is occupied by another icon
	/*bool bFail;
	CIcon *target = m_IconPages[pos.m_nPage].GetItem(pos.m_RasterPos, bFail);
	if (bFail || (target != nullptr && target != icon))
		return false;*/

	// move to new position
	m_IconPages[pos.m_nPage].SetItem(pos.m_RasterPos, icon);
	icon->SetIsUnlinked(false);
	icon->SetPosition(pos);
	InitIconMoveAnimation(icon, Col2Screen(pos.m_nPage, pos.m_RasterPos.m_nCol), Row2Screen(pos.m_RasterPos.m_nRow));
	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//													CIconManager::CanPlaceIconOnCurrentPage()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIconManager::CanPlaceIconOnCurrentPage(int x, int y)
{
	if (!IsValidPos(x, y))
		return false;

	CRasterPos new_pos = ComputeRasterPos(x, y);

	// fail, if new position is occupied by another icon
	bool bFail;
	CIcon *target = m_itCurPage->GetItem(new_pos, bFail);
	if (bFail || target != nullptr)
		return false;

	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//													CIconManager::PlaceIconOnCurrentPage()
//
// store icon in current page's m_Icons array
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIconManager::PlaceIconOnCurrentPage(CIcon *icon, int x, int y)
{
	if (!IsValidPos(x, y))
		return false;

	CRasterPos new_pos = ComputeRasterPos(x, y);

	// fail, if new position is occupied by another icon
	bool bFail;
	CIcon *target = m_itCurPage->GetItem(new_pos, bFail);
	if (bFail || (target != nullptr && target != icon))
		return false;

	// move to new position
	m_itCurPage->SetItem(new_pos, icon);
	icon->SetIsUnlinked(false);
	icon->SetPosition(CIconPos(new_pos, m_nCurPageNum));
	InitIconMoveAnimation(icon, Col2Screen(new_pos.m_nCol), Row2Screen(new_pos.m_nRow));
	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::DrawIcons()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::DrawIcons(sf::RenderWindow &window, sf::Text &text, int view_left, int view_right)
{
	int count = m_itCurPage->GetCount();
	for (int i = 0; i < count; i++)
	{
		CIcon *icon = m_itCurPage->GetItem(i);
		if (icon && icon->IsVisible(view_left, view_right))
		{
			icon->Draw(window);
			icon->DrawTitle(window, text);
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//													CIconManager::DrawUnlinkedIcons()
// draw unlinked selected icons
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::DrawUnlinkedIcons(sf::RenderWindow &window, sf::Text &text, int view_left, int view_right)
{
	//window.pushGLStates();

	_foreach(it, m_listSelectedIcons)
	{
		if ((*it)->GetIsUnlinked() && (*it)->IsVisible(view_left, view_right))
		{
			(*it)->Draw(window);
			(*it)->DrawTitle(window, text);
		}
	}

	//window.popGLStates();
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//													CIconManager::DrawAnimatedIcons()
//
// draw all icons of the animation list, which do not belong to the given page range
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::DrawAnimatedIcons(int start_page, int end_page, sf::RenderWindow &window, sf::Text &text, int view_left, int view_right)
{
	_foreach(it, m_listAnimatedIcons)
	{
		if ((*it)->GetPosition().m_nPage < start_page || (*it)->GetPosition().m_nPage >= end_page)
		{
			if ((*it)->IsVisible(view_left, view_right))
			{
				(*it)->Draw(window);
				(*it)->DrawTitle(window, text);
			}
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::FindIconUnderMouse()
// --------------------------------------------------------------------------------------------------------------------------------------------
CIcon *CIconManager::FindIconUnderMouse(int mouse_x, int mouse_y)
{
	// we do not use ComputeRasterPos() here, because we want to find out, if the user
	// clicks exactly onto an icon, or into the free desktop space
	int count = m_itCurPage->GetCount();
	for (int i = 0; i < count; i++)
	{
		CIcon *icon = m_itCurPage->GetItem(i);
		if (icon && icon->HitTest(mouse_x, mouse_y))
			return icon;
	}

	return nullptr;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::FindIconPage()
//
// Searches a given icon-pointer in all pages.
// Returns: the iterator that points to the page, or end()
// --------------------------------------------------------------------------------------------------------------------------------------------
TIconPagesIter CIconManager::FindIconPage(const CIcon *icon)
{
	bool bFail;

	_foreach(it, m_IconPages)
	{
		it->Find(icon, bFail);
		if (!bFail)
			return it;
	}

	return m_IconPages.end();
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::StartLassoMode()
//
//	==> deselects all icons on all other pages
//  ==> creates a snapshot of the current state of all icons
//  ==> so we can deselect what has been selected during the current operation, if the lasso is uncovering
//      previously covered icons (and vice versa, we can select, what has been deselected, if on de-selection mode)
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::StartLassoMode(bool do_select)
{
	//	deselect all icons on all other pages
	_foreach(it, m_IconPages)
	{
		if (it != m_itCurPage)
			it->SelectAll(*this, false);
	}

	m_itCurPage->CreateStateSnapshot();
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::PlaceSelectedIcons()
//
// Settles all selected icons in their current positions, if possible.
// Used during mouse-move operation.
// --------------------------------------------------------------------------------------------------------------------------------------------
#if 0
void CIconManager::PlaceSelectedIcons()
{
	sf::Vector2f icon_pos;

	// if no icon changed the position, do not create an undo action
	bool has_changed_pos = false;

	_foreach(it, m_listSelectedIcons)
	{
		CIconPos new_pos(ComputeRasterPos((int)(*it)->GetSpritePosition().x, (int)(*it)->GetSpritePosition().y), m_nCurPageNum);
		if (new_pos != (*it)->GetBackupPosition())
		{
			has_changed_pos = true;
			break;
		}
	}

	CIconUndoGroup *undo_group = new CIconUndoGroup();

	_foreach(it, m_listSelectedIcons)
	{
		if (has_changed_pos)
			undo_group->Add(new CIconMoveAction(*it, (*it)->GetBackupPosition()));

		icon_pos = (*it)->GetSpritePosition();

		if (!PlaceIconOnCurrentPage(*it, (int)icon_pos.x, (int)icon_pos.y))
		{
			// The new position is occupied by another icon, move ALL icons back to original position.
			// Unfortunately it is required for logical correctness to move ALL icons back, because
			// the original position of a moved-back icon might be occupied by another moved icon otherwise.
			int current_page = GetCurrentPageNum();
			CIconPos backup_pos;
			_foreach(it_moveback, m_listSelectedIcons)
			{
				if (!(*it_moveback)->GetIsUnlinked())
					UnlinkIcon(*it_moveback);
				backup_pos = (*it_moveback)->GetBackupPosition();
				PlaceIcon(*it_moveback, backup_pos);
			}

			delete undo_group;	// this is not used now
			return;				// quit
		}
	}

	if (has_changed_pos)
		PushUndoAction(undo_group);
	else
		delete undo_group;
}

#else
void CIconManager::PlaceSelectedIcons()
{
	sf::Vector2f icon_pos;

	// if no icon changed the position, do not create an undo action
	bool has_changed_pos = false;

	_foreach(it, m_listSelectedIcons)
	{
		icon_pos = (*it)->GetSpritePosition();
		int x = (int)icon_pos.x;
		int y = (int)icon_pos.y;
		CIconPos new_pos(ComputeRasterPos(x, y), m_nCurPageNum);
		if (new_pos != (*it)->GetBackupPosition() && IsValidPos(x, y))
		{
			has_changed_pos = true;
			break;
		}
	}

	if (!has_changed_pos)
	{
		// the icons have not changed their raster position, just place them without an undo-action:
		CIconPos backup_pos;
		_foreach(it, m_listSelectedIcons)
		{
			backup_pos = (*it)->GetBackupPosition();
			PlaceIcon(*it, backup_pos);
		}

		return;
	}

	// at least one icon has changed its raster position
	// Create undo group
	CIconUndoGroup *undo_group = new CIconUndoGroup();

	// try to place icons at new position
	_foreach(it, m_listSelectedIcons)
	{
		undo_group->Add(new CIconMoveAction(*it, (*it)->GetBackupPosition()));

		icon_pos = (*it)->GetSpritePosition();

		if (!PlaceIconOnCurrentPage(*it, (int)icon_pos.x, (int)icon_pos.y))
		{
			// The new position is occupied by another icon, move ALL icons back to original position.
			// Unfortunately it is required for logical correctness to move ALL icons back, because
			// the original position of a moved-back icon might be occupied by another moved icon otherwise.
			int current_page = GetCurrentPageNum();
			CIconPos backup_pos;
			_foreach(it_moveback, m_listSelectedIcons)
			{
				if (!(*it_moveback)->GetIsUnlinked())
					UnlinkIcon(*it_moveback);
				backup_pos = (*it_moveback)->GetBackupPosition();
				PlaceIcon(*it_moveback, backup_pos);
			}

			delete undo_group;	// this is not used now
			return;				// quit
		}
	}

	PushUndoAction(undo_group);
}
#endif


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::AddToAnimationList()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::AddToAnimationList(CIcon *icon)
{
	if (icon->GetIsAnimated())
		return;

	icon->SetIsAnimated(true);
	m_listAnimatedIcons.push_back(icon);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::InitMoveAnimation()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::InitIconMoveAnimation(CIcon *icon, float new_x, float new_y)
{
	icon->InitMoveAnimation(new_x, new_y);
	AddToAnimationList(icon);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::SetIconState()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::SetIconState(CIcon *icon, CIcon::EIconState new_state, bool scale_now)
{
	if (icon->SetState(new_state, scale_now))
		AddToAnimationList(icon);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::PerformAnimations()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::PerformAnimations()
{
	bool is_scaling, is_moving;

	TListIconsIter it = m_listAnimatedIcons.begin();
	while (it != m_listAnimatedIcons.end())
	{
		is_scaling = (*it)->PerformScalingAnimation();
		is_moving = (*it)->PerformMoveAnimation();

		if (!is_scaling && !is_moving)
		{
			// remove from animation-list
			(*it)->SetIsAnimated(false);
			TListIconsIter it_tmp = it;
			it++;
			m_listAnimatedIcons.erase(it_tmp);
		}
		else
			it++;
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::ResyncIcons()
//
// iterates over all icons and re-syncs all icons that are not resynced yet
// if no un-resynced icon is found, the loop is aborted
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::ResyncIcons()
{
	m_bResyncRunning = true;

	bool did_resync_one;
	do
	{
		did_resync_one = false;

		for (auto it = m_IconPages.begin(); it != m_IconPages.end() && m_bResyncRunning; it++)
		{
			if (it->ResyncIcons(m_BitmapCache, m_bResyncRunning, m_bChangedAnIcon))
				did_resync_one = true;
		}
	}
	while (did_resync_one && m_bResyncRunning);

	for (auto it = m_IconPages.begin(); it != m_IconPages.end() && m_bResyncRunning; it++)
		it->ClearIconsResyncFlag();

	m_bResyncRunning = false;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::ReadFromFile()
// --------------------------------------------------------------------------------------------------------------------------------------------
//extern CSettings gSettings;
void CIconManager::ReadFromFile(const RString &file_name)
{
	Stream stream(file_name, _T("rb"));

	// Magic
	RString magic = stream.ReadString();
	if (magic != PLACES_DB_MAGIC)
		throw Exception(Exception::EXCEPTION_DB_CORRUPT);

	// Version
	if (stream.ReadUInt() > PLACES_DB_VERSION)			// this implies we can always read older versions
		throw Exception(Exception::EXCEPTION_DB_VERSION);

	// read rows / columns configuration
	m_nMaxCols = stream.ReadInt();
	m_nMaxRows = stream.ReadInt();
	m_nDbIconSize = stream.ReadInt();
//	gSettings.Read(stream);
//	stream.ReadInt();

	// read bitmap cache
	m_BitmapCache.Read(stream);

	// read icon pages
	m_IconPages.clear();
	stream.SetExtra((void *)&m_BitmapCache);
	unsigned int nPages = stream.ReadUInt();

	for (unsigned int i = 0; i < nPages; i++)
		m_IconPages.push_back(CIconPage(stream));
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CIconManager::WriteToFile()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIconManager::WriteToFile(const RString &file_name) const
{
	_trename(file_name.c_str(), (file_name + _T(".bak")).c_str());
	Stream stream(file_name, _T("wb"));

	// Magic + Version
	stream.WriteString(PLACES_DB_MAGIC);
	stream.WriteUInt(PLACES_DB_VERSION);

	// write rows / columns configuration
	stream.WriteInt(m_nMaxCols);
	stream.WriteInt(m_nMaxRows);
	stream.WriteInt(gConfig.m_nIconSizeX);

	// Write bitmap cache
	m_BitmapCache.Write(stream);

	// Write icon pages
	stream.SetExtra((void *)&m_BitmapCache);
	stream.WriteUInt((unsigned int)m_IconPages.size());
	_foreach(it, m_IconPages)
		it->Write(stream);
}
