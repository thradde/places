
// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CPopupMenuItem
// --------------------------------------------------------------------------------------------------------------------------------------------
class CPopupMenu;

class CPopupMenuItem
{
public:
	enum EFlags
	{
		kNoFlags  = 0,
		kDisabled = 1 << 0,     // item is gray and not selectable
		kTitle    = 1 << 1,     // item indicates a title and is not selectable
		kChecked  = 1 << 2,     // item has a checkmark
		kSeparator  = 1 << 3    // item is a separator
	};

protected:
	RString		m_strText;
	CPopupMenu	*m_pSubmenu;
	int			m_nFlags;

public:
	CPopupMenuItem(const RString &text, int flags = kNoFlags)
		:	m_strText(text),
			m_nFlags(flags),
			m_pSubmenu(nullptr)
	{
	}

	CPopupMenuItem(const RString &text, CPopupMenu *pSubMenu)
		:	m_strText(text),
			m_nFlags(kNoFlags),
			m_pSubmenu(pSubMenu)
	{
	}

	~CPopupMenuItem()
	{
		//don't need to delete submenu
	}

	void SetText(const RString &text) { m_strText = text; }
	RString GetText() const { return m_strText; }

	bool GetEnabled() const { return !(m_nFlags & kDisabled); }
	bool GetChecked() const { return (m_nFlags & kChecked) != 0; }
	bool GetIsTitle() const { return (m_nFlags & kTitle) != 0; }
	bool GetIsSeparator() const { return (m_nFlags & kSeparator) != 0; }

	CPopupMenu *GetSubmenu() const { return m_pSubmenu; }

	void SetEnabled(bool state)
	{
		if (state)
			m_nFlags &= ~kDisabled;
		else
			m_nFlags |= kDisabled;
	}

	void SetChecked(bool state)
	{
		if (state)
			m_nFlags |= kChecked;
		else
			m_nFlags &= ~kChecked;
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CPopupMenu
// --------------------------------------------------------------------------------------------------------------------------------------------
class CPopupMenu
{
protected:
	RString	m_strName;
	RString	m_strSelectedMenu;		// after user selection, this holds the name of the selected menu (so submenus can be identified)
	RString	m_strSelectedEntry;		// after user selection, this holds the string of the selected entry
	long	m_nChosenItemIndex;		// entry-index of selected menu / entry

	vector<CPopupMenuItem *>	m_arMenuItems;

protected:	// -------------- GUI handling ---------------

	CPopupMenu *GetItemMenu(long idx, long &idxInMenu, long &offsetIdx, CPopupMenu *pMenu);

	HMENU CreateMenu(CPopupMenu *pMenu, long &offsetIdx);

	CPopupMenu *CreatePopupMenu(HWND hwnd, int x, int y);


public:
	CPopupMenu(const RString &name)
		:	m_strName(name),
			m_nChosenItemIndex(-1)
	{
	}

	~CPopupMenu()
	{
		_foreach(it, m_arMenuItems)
			delete (*it);
	}

	const RString	&GetSelectedMenu() const { return m_strSelectedMenu; }

	const RString	&GetSelectedEntry() const { return m_strSelectedEntry; }

	const RString &GetName() const { return m_strName; }

	int GetItemCount() { return (int)m_arMenuItems.size(); }

	CPopupMenuItem *GetItem(int index)
	{
		if (index >= 0 && index < (int)m_arMenuItems.size())
			return m_arMenuItems[index];

		return nullptr;
	}

	long GetChosenItemIndex() const { return m_nChosenItemIndex; }

	void SetChosenItemIndex(long index)
	{
		m_nChosenItemIndex = index;
	}

	RString GetItemText(int index)
	{
		return GetItem(index)->GetText();
	}

	CPopupMenuItem *AddItem(const RString &text, CPopupMenu *submenu = nullptr)
	{
		CPopupMenuItem *item = new CPopupMenuItem(text, submenu);
		m_arMenuItems.push_back(item);
		return item;
	}

	CPopupMenuItem *AddSeparator()
	{
		CPopupMenuItem *item = new CPopupMenuItem(_T(""), CPopupMenuItem::kSeparator);
		m_arMenuItems.push_back(item);
		return item;
	}

	void CheckItem(int index, bool state)
	{
		CPopupMenuItem *item = m_arMenuItems[index];
		item->SetChecked(state);
	}

	void CheckItemExclusive(int index)
	{
		for (int i = 0; i < (int)m_arMenuItems.size(); i++)
			m_arMenuItems[i]->SetChecked(i == index);
	}

	bool IsItemChecked(int index) const
	{
		return m_arMenuItems[index]->GetChecked();
	}

	// returns false, if nothing was chosen
	bool DoPopupMenu(HWND hwnd, int x, int y);
};
