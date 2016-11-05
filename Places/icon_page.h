

// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CIconPage
//
// This array reflects the positions of the icons on a GUI page
// --------------------------------------------------------------------------------------------------------------------------------------------
class CIconManager;

class CIconPage
{
protected:
	CIcon	**m_arIcons;
	int		m_nCols;		// number of columns
	int		m_nRows;		// number of rows

	void Free()
	{
		if (m_arIcons)
		{
			int count = m_nCols * m_nRows;
			for (int i = 0; i < count; i++)
			{
				if (m_arIcons[i])
					delete m_arIcons[i];
			}
			delete[] m_arIcons;
		}
	}

public:
	CIconPage()
		:	m_arIcons(nullptr)
	{
	}

	CIconPage(CIconPage &&page);

	CIconPage(Stream &stream);
	void Write(Stream &stream) const;

	~CIconPage()
	{
		Free();
	}

	void SetDimensions(int cols, int rows);

	int	GetCols() const { return m_nCols; }
	int	GetRows() const { return m_nRows; }

	int GetCount() const { return m_nCols * m_nRows; }

	int ComputeIndex(const CRasterPos &pos) const { return pos.m_nRow * m_nCols + pos.m_nCol; }

	CRasterPos ComputeRasterPos(int index) const
	{
		return CRasterPos(index % m_nCols, index / m_nCols);
	}

	CRasterPos Find(const CIcon *icon, bool &bFail);

	CIcon *GetItem(int index)
	{
		return m_arIcons[index];
	}

	CIcon *GetItem(const CRasterPos &pos, bool &bFail);
	void SetItem(const CRasterPos &pos, CIcon *icon);

	// adapt all icons to new screen / window resolution
	void RecomputeAllIcons(int pixels, int nPageNum, int nHorizontalMargin, int nVerticalMargin, int nIconRasterX, int nIconRasterY);

	void CreateStateSnapshot();
	void RestoreStateSnapshot(CIconManager &icon_manager);
	void CreateSelectedSnapshot(TListIcons &listSelectedIcons);
	bool HasSelectedIcons() const;

	void Select(CIconManager &icon_manager, bool do_select, const sf::FloatRect &rc);
	void SelectAll(CIconManager &icon_manager, bool do_select);
	void LassoSelect(CIconManager &icon_manager, bool do_select, const sf::FloatRect &rc);
	void SetAllIconStates(CIconManager &icon_manager, CIcon::EIconState state);

	// iterates over all icons and re-syncs one icon that is not resynced yet
	// if no un-resynced icon is found, false is returned. Otherwise true.
	bool ResyncIcons(CBitmapCache &bitmap_cache, bool &resync_running, bool &changed_an_icon);
	void ClearIconsResyncFlag();

	// checks, if a new layout can be applied to each page, so that no icons
	// would fall outside the new layout
	bool CanChangeLayout(const CSettings &new_layout) const;

	void ChangeLayout(const CSettings &new_layout);
	void RenderTitles();
};


typedef vector<CIconPage> TIconPages;
typedef TIconPages::iterator TIconPagesIter;
