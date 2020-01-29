
// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CRasterPos
//
// Icon position in the raster units of CIconArray (which reflects the screen's icon layout)
// --------------------------------------------------------------------------------------------------------------------------------------------
class CRasterPos
{
public:
	int	m_nCol;
	int	m_nRow;

public:
	CRasterPos()
		:	m_nCol(0),
			m_nRow(0)
	{
	}

	CRasterPos(int col, int row)
		:	m_nCol(col),
			m_nRow(row)
	{
	}

	bool operator !=(const CRasterPos &rhs) const
	{
		return m_nCol != rhs.m_nCol || m_nRow != rhs.m_nRow;
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CIconPos
//
// Used to store the position of an icon, including page number
// --------------------------------------------------------------------------------------------------------------------------------------------
class CIconPos
{
public:
	CRasterPos	m_RasterPos;
	int			m_nPage;

public:
	CIconPos()
		:	m_nPage(0)
	{
	}

	CIconPos(const CRasterPos &pos, int page)
		:	m_RasterPos(pos),
			m_nPage(page)
	{
	}

	bool operator !=(const CIconPos &rhs) const
	{
		return m_RasterPos != rhs.m_RasterPos || m_nPage != rhs.m_nPage;
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CIconText
//
// Holds the layout of an icon's title text.
// --------------------------------------------------------------------------------------------------------------------------------------------

// One line of text
class CIconTextLine
{
public:
	RString	m_strText;
	int		m_nOffset;		// offset in pixels to the left of the icon

public:
	CIconTextLine(const RString &text, int offset)
		:	m_strText(text),
			m_nOffset(offset)
	{
	}
};


class CIconText
{
public:
	vector<CIconTextLine>	m_arTextLines;

public:
	CIconText()
	{
	}

	void AddLine(const CIconTextLine &line)
	{
		m_arTextLines.push_back(line);
	}

	void Reset()
	{
		m_arTextLines.clear();
	}

	int GetNumLines() const { return (int)m_arTextLines.size(); }
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CIcon
// --------------------------------------------------------------------------------------------------------------------------------------------
void InitIconShader();

class CIcon
{
public:
	// visual icon states
	enum EIconState
	{
		enStateNormal,
		enStateOpen,		// icon is clicked to open a program / link
		enStateHover,
		enStateBlowup,

		// for all following states, the title text is drawn inverted
		enStateHighlight,
		enStateDragging,
	};

public:
	RString			m_strIconPath;		// if set, specifies external icon file
	RString			m_strFilePath;		// the exe-path or folder-link or whatever shall be represented as an icon
	RString			m_strParameters;	// optional parameters that shall be passed to an executable
	//bool			m_bUseShellApi;		// if true, the icon bitmap shall be retrieved using the shell API.
										// otherwise m_strIconPath points to a real bitmap file that shall be used as an icon
	RString			m_strFullTitle;
	CIconText		m_Title;			// layout of title string

	CIconPos		m_Position;			// raster pos and page of icon
	CIconPos		m_BackupPosition;	// this is used during move-operation to backup the initial position before the move-op takes place

	EIconState		m_enState;			// the current state, i.e. normal, highlighted, etc.
	EIconState		m_enStateBackup;	// this is used during lasso-operation to backup the initial state before the lasso-op takes place
	float			m_fSpriteScaleInc;	// for scale animation
	float			m_fTargetScale;		// for scale animation
	sf::Vector2f	m_MovedBy;			// if scaling due to mouse-button down, this member holds dx, dy offsets (to position title text moved)

	// move animation
	bool			m_bIsAnimated;
	bool			m_bIsUnlinked;
	float			m_fStepsToAnimateMove;
	float			m_fAnimateMoveX;
	float			m_fAnimateMoveY;

	// internal
	bool				m_bResynced;
	CBitmapCacheItem	*m_pCachedBitmap;
	sf::Sprite			m_Sprite;			// This holds also the screen's pixel position of the icon

protected:
	void CreateOrAddCachedBitmap(CBitmapCache &bitmap_cache);

public:
	CIcon(CBitmapCache &bitmap_cache, const RString &strTitle, const RString &strFilePath, const RString &strIconPath);

	CIcon(Stream &stream);
	void Write(Stream &stream) const;

	~CIcon()
	{
	}

	bool GetIsAnimated() const { return m_bIsAnimated; }
	void SetIsAnimated(bool val) { m_bIsAnimated = val; }

	bool GetIsUnlinked() const { return m_bIsUnlinked; }
	void SetIsUnlinked(bool val) { m_bIsUnlinked = val; }

	const RString &GetIconPath() const { return m_strIconPath; }
	void SetIconPath(CBitmapCache &bitmap_cache, const RString &new_path);

	// if the file path shall be used to retrieve an icon via shell api,
	// call SetIconPath() with an empty string first.
	const RString &GetFilePath() const { return m_strFilePath; }
	void SetFilePath(CBitmapCache &bitmap_cache, const RString &new_path);

	RString &GetFullTitle() { return m_strFullTitle; }
	void SetFullTitle(const RString &title);

	bool IsResynced() const { return m_bResynced; }
	bool Resync(CBitmapCache &bitmap_cache);

	CBitmapCacheItem	*GetCachedBitmap() const { return m_pCachedBitmap; }
	void SetCachedBitmap(CBitmapCacheItem *bitmap)
	{
		m_pCachedBitmap = bitmap;
	}

	// Creates a sprite which will be exactly scaled to the desired pixels.
	// Chooses the best fitting texture.
	void CreateIcon(int pixels, bool force_create = true);
	void RenderTitleTextLayout();

	bool IsVisible(int view_left, int view_right);
	void Draw(sf::RenderWindow &window);
	void DrawTitle(sf::RenderWindow &window, sf::Text &text);

	bool HitTest(int x, int y) const
	{
		sf::FloatRect rc = m_Sprite.getGlobalBounds();
		return rc.contains((float)x, (float)y);
	}

	const CIconPos &GetPosition() const
	{
		return m_Position;
	}

	void SetPosition(const CIconPos &pos)
	{
		m_Position = pos;
	}

	const sf::Vector2f &GetSpritePosition() const
	{
		return m_Sprite.getPosition();
	}

	void SetSpritePosition(float x, float y)
	{
		m_Sprite.setPosition(x, y);
	}

	sf::Vector2f GetOrigin() const
	{
		return m_Sprite.getOrigin();
	}

	void DoBackupPosition()
	{
		m_BackupPosition = m_Position;
	}

	const CIconPos &GetBackupPosition() const
	{
		return m_BackupPosition;
	}

	void DoBackupState()
	{
		m_enStateBackup = m_enState;
	}

	EIconState GetBackupState() const
	{
		return m_enStateBackup;
	}

	// move icon relative to current position
	void Move(float dx, float dy)
	{
		m_Sprite.move(dx, dy);
	}

	EIconState GetState() const
	{
		return m_enState;
	}

	bool SetState(EIconState state, bool scale_now = false);

	bool IsScaling()
	{
		return m_Sprite.getScale().x != m_fTargetScale;
	}

	// returns false, if the animation sequence is finished
	bool PerformScalingAnimation();

	float CalcAnimationSteps(float distance) const
	{
		return floor(powf(distance, 0.525f)) + 5.f;		// flat curve
	}

	void InitMoveAnimation(float x, float y);

	// returns false, if the animation sequence is finished
	bool PerformMoveAnimation()
	{
		if (m_fStepsToAnimateMove <= 0)
			return false;

		m_fStepsToAnimateMove--;
		m_Sprite.move(m_fAnimateMoveX, m_fAnimateMoveY);
		return true;
	}
};


typedef list<CIcon *> TListIcons;
typedef TListIcons::iterator TListIconsIter;
