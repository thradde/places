

// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CIconManager
// --------------------------------------------------------------------------------------------------------------------------------------------
class CIconManager
{
public:
	CBitmapCache	m_BitmapCache;
	TIconPages		m_IconPages;
	TIconPagesIter	m_itCurPage;
	int				m_nCurPageNum;
	int				m_nHorizontalMargin;
	int				m_nVerticalMargin;
	int				m_nIconRasterX;
	int				m_nIconRasterY;
	int				m_nMaxCols;
	int				m_nMaxRows;
	bool			m_bResyncRunning;		// true, if resync is currently running. set to false to terminate the resync
	bool			m_bChangedAnIcon;		// true, if any icon was changed by resync
	bool			m_bChangedLayout;		// true, if the icon layout (cols / rows) were changed
	int				m_nDbIconSize;			// size of draw-bitmap cached in DB (initialized in ReadFromFile())

	TListIcons		m_listSelectedIcons;
	TListIcons		m_listAnimatedIcons;

	CUndoStack		m_UndoStack;

protected:
//	void DrawIcons(sf::RenderWindow &window);
//	void DrawIconTitles(sf::RenderWindow &window, sf::Text &text);

public:
	CIconManager()
		:	m_nCurPageNum(0),
			m_bResyncRunning(false),
			m_bChangedAnIcon(false),
			m_bChangedLayout(false),
			m_nDbIconSize(0)
	{
	}

	~CIconManager()
	{
	}

	CBitmapCache &GetBitmapCache() { return m_BitmapCache; }

	void AddNewPage();

	void Init(const CConfig &config, bool create_first_empty_page);

	int GetPageCount() const { return (int)m_IconPages.size(); }

	int GetCurrentPageNum() const { return m_nCurPageNum; }

	// first page = 0
	void GotoPage(int page)
	{
		int n = 0;
		for (m_itCurPage = m_IconPages.begin(); m_itCurPage != m_IconPages.end() && n < page; m_itCurPage++)
			n++;

		m_nCurPageNum = n;
	}

	bool IsValidPos(int screen_x, int screen_y);
	CRasterPos ComputeRasterPos(int screen_x, int screen_y);

	float Col2Screen(int page_num, int col)
	{
		return (float)(page_num * gConfig.m_nWindowWidth + m_nHorizontalMargin + gConfig.m_nIconTotalWidth_2 + col * m_nIconRasterX);
	}

	// for current page
	float Col2Screen(int col)
	{
		return Col2Screen(m_nCurPageNum, col);
	}

	float Row2Screen(int row)
	{
		return (float)(m_nVerticalMargin + gConfig.m_nIconTotalHeight_2 + row * m_nIconRasterY);
	}

	bool FindFreePosition(CRasterPos &pos);
	
	// always using shell api to retrieve the icon
	CIcon *CreateIcon(const RString &strFilePath, const RString &strTitle, int screen_x, int screen_y)
	{
		RString strEmpty;
		CIcon *icon = new CIcon(m_BitmapCache, strTitle, strFilePath, strEmpty);
		icon->CreateIcon(gConfig.m_nIconSizeX);
		icon->RenderTitleTextLayout();
		icon->SetState(CIcon::enStateHighlight);
		Add(icon, screen_x, screen_y);
		return icon;
	}

	void Add(CIcon *icon, int screen_x, int screen_y);

	// unlink icon from the page's m_Icons array
	void UnlinkIcon(CIcon *icon);

	// de-reference icon from bitmap cache
	void UnrefIcon(CIcon *icon)
	{
		m_BitmapCache.Unref(icon->GetCachedBitmap());
	}

	// store icon at target position (target page may be any page)
	bool PlaceIcon(CIcon *icon, const CIconPos &pos);

	// store icon in current page's m_Icons array
	bool CanPlaceIconOnCurrentPage(int x, int y);
	bool PlaceIconOnCurrentPage(CIcon *icon, int x, int y);

	void DrawIcons(sf::RenderWindow &window, sf::Text &text, int view_left, int view_right);
	void DrawUnlinkedIcons(sf::RenderWindow &window, sf::Text &text, int view_left, int view_right);
	void DrawAnimatedIcons(int start_page, int end_page, sf::RenderWindow &window, sf::Text &text, int view_left, int view_right);

	// adapt all icons to new screen / window resolution
	void RecomputeAllIcons()
	{
		int page = 0;
		for (auto it_page = m_IconPages.begin(); it_page != m_IconPages.end(); it_page++)
			it_page->RecomputeAllIcons(gConfig.m_nIconSizeX, page++, m_nHorizontalMargin, m_nVerticalMargin, m_nIconRasterX, m_nIconRasterY);
	}

	CIcon *FindIconUnderMouse(int mouse_x, int mouse_y);
	
	CIcon *GetIconFromPos(const CIconPos &pos)
	{
		bool bFail;
		return m_IconPages[pos.m_nPage].GetItem(pos.m_RasterPos, bFail);
	}

	TIconPagesIter FindIconPage(const CIcon *icon);

	void RemoveIcon(const CIcon *icon)
	{
		m_IconPages[icon->GetPosition().m_nPage].SetItem(icon->GetPosition().m_RasterPos, nullptr);
	}

	void Select(bool do_select, const sf::FloatRect &rc)
	{
		m_itCurPage->Select(*this, do_select, rc);
	}

	void SelectAll(bool do_select)
	{
		m_itCurPage->SelectAll(*this, do_select);
	}

	void DeSelectAllOnOtherPages()
	{
		// deleselect all selected icons, which are not on the current page
		for (auto it : m_listSelectedIcons)
		{
			if (it->GetPosition().m_nPage != m_nCurPageNum)
				SetIconState(it, CIcon::enStateNormal);
		}
		
		// re-create the selected snapshot
		CreateSelectedSnapshot();
	}

	void StartLassoMode(bool do_select);
	void LassoSelect(bool do_select, const sf::FloatRect &rc)
	{
		m_itCurPage->LassoSelect(*this, do_select, rc);
	}

	void CreateSelectedSnapshot()
	{
		m_listSelectedIcons.clear();
		m_itCurPage->CreateSelectedSnapshot(m_listSelectedIcons);

		// backup position of each icon
		for (auto it : m_listSelectedIcons)
			it->DoBackupPosition();
	}

	TListIcons &GetSelectedIcons()
	{
		return m_listSelectedIcons;
	}

	void SetSelectedIconsState(CIcon::EIconState state)
	{
		for (auto it : m_listSelectedIcons)
			SetIconState(it, state);
	}

	void UnlinkSelectedIcons()
	{
		for (auto it : m_listSelectedIcons)
			UnlinkIcon(it);
	}

	// Settles all selected icons in their current positions, if possible.
	// Used during mouse-move operation.
	void PlaceSelectedIcons();

	void MoveSelectedIcons(float dx, float dy)
	{
		for (auto it : m_listSelectedIcons)
			it->Move(dx, dy);
	}

	bool HasSelectedIcons() const
	{
		// this method might be called, without having called CreateSelectedSnapshot() before
		return m_itCurPage->HasSelectedIcons();
	}

	void ClearSelectedSnapshot()
	{
		for (auto it : m_listSelectedIcons)
			SetIconState(it, CIcon::enStateNormal);
		m_listSelectedIcons.clear();
	}

	void PushUndoAction(CUndoAction *action)
	{
		m_UndoStack.PushUndoAction(*this, action);
	}

	bool CanUndo() const
	{
		return m_UndoStack.CanUndo();
	}

	void Undo()
	{
		ClearSelectedSnapshot();
		m_UndoStack.PerformUndo(*this);
	}

	bool CanRedo() const
	{
		return m_UndoStack.CanRedo();
	}

	void Redo()
	{
		ClearSelectedSnapshot();
		m_UndoStack.PerformRedo(*this);
	}

	void ClearUndoStack()
	{
		m_UndoStack.Clear(*this);
	}

	// iterates over all icons and re-syncs all icons that are not resynced yet
	// if no un-resynced icon is found, the loop is aborted
	void ResyncIcons();

	// checks, if a new layout can be applied to each page, so that no icons
	// would fall outside the new layout
	bool CanChangeLayout(const CSettings &new_layout) const
	{
		for (auto it = m_IconPages.begin(); it != m_IconPages.end(); it++)
		{
			if (!it->CanChangeLayout(new_layout))
				return false;
		}

		return true;
	}

	void ChangeLayout(const CSettings &new_layout)
	{
		for (auto it = m_IconPages.begin(); it != m_IconPages.end(); it++)
			it->ChangeLayout(new_layout);

		RecomputeAllIcons();
		m_bChangedLayout = true;
	}

	void RenderTitles()
	{
		for (auto it = m_IconPages.begin(); it != m_IconPages.end(); it++)
			it->RenderTitles();
	}

	void AddToAnimationList(CIcon *icon);
	void InitIconMoveAnimation(CIcon *icon, float new_x, float new_y);
	void SetIconState(CIcon *icon, CIcon::EIconState state, bool scale_now = false);
	void PerformAnimations();

	void ReadFromFile(const RString &file_name);
	void WriteToFile(const RString &file_name) const;
};
