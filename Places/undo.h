
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


class CIconManager;

// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CUndoAction
// Base class for all Undo / Redo actions
// --------------------------------------------------------------------------------------------------------------------------------------------
class CUndoAction
{
public:
	virtual ~CUndoAction()
	{
	}

	// if an undo action requires multiple passes (e.g. the Move Action), derived classes can overload this method
	virtual int	 GetRequiredPasses() const { return 1; }

	virtual void PerformUndo(CIconManager &icon_manager, int pass) = 0;
	virtual void PerformRedo(CIconManager &icon_manager, int pass) = 0;

	// if an undo- or redo-action requires cleanup before deletion, this method is used for it.
	// the parameter is_undo_action determines, whether the action is on the undo- or redo-stack.
	virtual void Release(CIconManager &icon_manager, bool is_undo_action) {}
};

typedef list<CUndoAction *> TListUndoActions;
typedef TListUndoActions::iterator TListUndoActionsIter;


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CIconAction
// Base class for all Undo / Redo actions related to icons
// --------------------------------------------------------------------------------------------------------------------------------------------
class CIconAction : public CUndoAction
{
public:
	CIcon	*m_pIcon;

public:
	CIconAction(CIcon *icon)
		:	m_pIcon(icon)
	{
	}

	virtual ~CIconAction()
	{
		// usually, m_pIcon is just a reference, so we do not delete it here
		// delete m_pIcon;
	}
};

typedef list<CIconAction *> TListIconActions;
typedef TListIconActions::iterator TListIconActionsIter;


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CIconUndoGroup
//
// Base class for actions related to groups of icons
// --------------------------------------------------------------------------------------------------------------------------------------------
class CIconUndoGroup : public CUndoAction
{
protected:
	TListIconActions	m_listIconActions;

public:
	CIconUndoGroup()
	{
	}

	virtual ~CIconUndoGroup()
	{
		for (auto it : m_listIconActions)
			delete it;
	}

	virtual void Add(CIconAction *action)
	{
		m_listIconActions.push_back(action);
	}

	// if an undo action requires multiple passes (e.g. the Move Action), derived classes can overload this method
	int	GetRequiredPasses() const override
	{
		if (!m_listIconActions.empty())
			return m_listIconActions.front()->GetRequiredPasses();

		return 0;		// nothing in list: zero actions / passes
	}

	void PerformUndo(CIconManager &icon_manager, int pass) override
	{
		for (auto it : m_listIconActions)
			it->PerformUndo(icon_manager, pass);
	}

	void PerformRedo(CIconManager &icon_manager, int pass) override
	{
		for (auto it : m_listIconActions)
			it->PerformRedo(icon_manager, pass);
	}

	void Release(CIconManager &icon_manager, bool is_undo_action) override
	{
		for (auto it : m_listIconActions)
			it->Release(icon_manager, is_undo_action);
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CIconInsertAction
// --------------------------------------------------------------------------------------------------------------------------------------------
class CIconInsertAction : public CIconAction
{
public:
	CIconInsertAction(CIcon *icon)
		:	CIconAction(icon)
	{
	}

	void PerformUndo(CIconManager &icon_manager, int pass) override;
	void PerformRedo(CIconManager &icon_manager, int pass) override;
	void Release(CIconManager &icon_manager, bool is_undo_action) override;
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CIconDeleteAction
// --------------------------------------------------------------------------------------------------------------------------------------------
class CIconDeleteAction : public CIconAction
{
public:
	CIconDeleteAction(CIcon *icon)
		:	CIconAction(icon)
	{
	}

	void PerformUndo(CIconManager &icon_manager, int pass) override;
	void PerformRedo(CIconManager &icon_manager, int pass) override;
	void Release(CIconManager &icon_manager, bool is_undo_action) override;
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CIconMoveAction
// --------------------------------------------------------------------------------------------------------------------------------------------
class CIconMoveAction : public CIconAction
{
public:
	CIconPos	m_OldPosition;

public:
	CIconMoveAction(CIcon *icon, const CIconPos &position)
		:	CIconAction(icon),
			m_OldPosition(position)
	{
	}

	// if an undo action requires multiple passes (e.g. the Move Action), derived classes can overload this method
	virtual int	GetRequiredPasses() const { return 2; }

	void PerformUndo(CIconManager &icon_manager, int pass) override;
	void PerformRedo(CIconManager &icon_manager, int pass) override
	{
		PerformUndo(icon_manager, pass);		// inverse-identical for move actions
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//														class CIconPropertiesAction
// --------------------------------------------------------------------------------------------------------------------------------------------
class CIconPropertiesAction : public CIconAction
{
public:
	RString	m_strOldTitle;
	RString	m_strOldIconPath;
	RString	m_strOldFilePath;
	RString	m_strOldParameters;
	bool m_bOldOpenAsAdmin;

public:
	CIconPropertiesAction(CIcon *icon)
		:	CIconAction(icon),
			m_strOldTitle(icon->GetFullTitle()),
			m_strOldIconPath(icon->GetIconPath()),
			m_strOldFilePath(icon->GetFilePath()),
			m_strOldParameters(icon->m_strParameters),
			m_bOldOpenAsAdmin(icon->m_bOpenAsAdmin)
	{
	}

	void PerformUndo(CIconManager &icon_manager, int pass) override;
	void PerformRedo(CIconManager &icon_manager, int pass) override
	{
		PerformUndo(icon_manager, pass);		// inverse-identical for property change actions
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CUndoStack
// --------------------------------------------------------------------------------------------------------------------------------------------
class CUndoStack
{
protected:
	TListUndoActions	m_listUndoActions;
	TListUndoActions	m_listRedoActions;

public:
	CUndoStack()
	{
	}

	// Before the destructor is executed, *always* call Clear().
	// Clear() has a parameter CIconManager&, which can not be provided to the destructor here.
	~CUndoStack()
	{
		for (auto it : m_listUndoActions)
			delete it;

		for (auto it : m_listRedoActions)
			delete it;
	}

	void PushUndoAction(CIconManager &icon_manager, CUndoAction *action)
	{
		m_listUndoActions.push_back(action);

		// when a new undo action is pushed onto the stack, the redo-stack is invalidated
		for (auto it : m_listRedoActions)
		{
			it->Release(icon_manager, false);
			delete it;
		}
		m_listRedoActions.clear();
	}

	bool CanUndo() const
	{
		return m_listUndoActions.size() > 0;
	}

	void PerformUndo(CIconManager &icon_manager)
	{
		if (m_listUndoActions.empty())
			return;

		// pop the most recent undo action from the undo-stack
		CUndoAction *action = m_listUndoActions.back();
		m_listUndoActions.pop_back();

		// perform it in the number of required passes
		int pass_count = action->GetRequiredPasses();
		for (int pass = 0; pass < pass_count; pass++)
			action->PerformUndo(icon_manager, pass);

		// move the action to the redo-stack
		m_listRedoActions.push_back(action);
	}

	bool CanRedo() const
	{
		return m_listRedoActions.size() > 0;
	}

	void PerformRedo(CIconManager &icon_manager)
	{
		if (m_listRedoActions.empty())
			return;

		// pop the most recent redo action from the redo-stack
		CUndoAction *action = m_listRedoActions.back();
		m_listRedoActions.pop_back();

		// perform it in the number of required passes
		int pass_count = action->GetRequiredPasses();
		for (int pass = 0; pass < pass_count; pass++)
			action->PerformRedo(icon_manager, pass);

		// move the action to the undo-stack
		m_listUndoActions.push_back(action);
	}

	void Clear(CIconManager &icon_manager)
	{
		// clear undo stack, so unused bitmaps are de-referenced and not written to file
		for (auto it : m_listUndoActions)
		{
			it->Release(icon_manager, true);
			delete it;
		}
		m_listUndoActions.clear();

		for (auto it : m_listRedoActions)
		{
			it->Release(icon_manager, false);
			delete it;
		}
		m_listRedoActions.clear();
	}
};
