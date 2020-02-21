

// --------------------------------------------------------------------------------------------------------------------------------------------
//															class CSettings
//
// This class is used to store global settings.
// The data herein is used for persistent storage in streams and it is also used for dialog exchange.
// --------------------------------------------------------------------------------------------------------------------------------------------
const int nColumns		= 16;		// default icon columns
const int nRows			= 7;		// default icon rows
const int nFontSize		= 13;		// default icon font size
const int nTitleLines	= 2;		// default icon title lines

class CSettings
{
public:
	int		m_nColumns;
	int		m_nRows;
	int		m_nIconFontSize;
	int		m_nIconTitleLines;

	RString		m_strShaderFile;
	int			m_nShaderSpeed;		// 1 - 100 = 0.001 - 0.10
	RString		m_strImageFile;
	int			m_nModifyImage;		// 0 = no action, 1 = tile, 2 = scale
	COLORREF	m_clrBkgColor;		// desktop solid color

	bool		m_bScrollWheelJumps;// if true, the scroll wheel jumps to the next page instead of scrolling
	bool		m_bDisableIfMaxWin;	// if true, the hooks are disabled if the foreground window is maximized (eg it is a game)

	int			m_nCurPageNum;		// this is set during file read and only used by the app-constructor: visible page when app was quit
	bool		m_bFullscreen;		// this is set during file read and only used by the app-constructor: if window was fullscreen when app was quit

public:
	CSettings()
		:	m_nColumns(nColumns),
			m_nRows(nRows),
			m_nIconFontSize(nFontSize),
			m_nIconTitleLines(nTitleLines),
			m_nShaderSpeed(5),
			m_nModifyImage(2),
			m_clrBkgColor(RGB(0x0a, 0x3b, 0x76)),
			m_bScrollWheelJumps(false),
			m_bDisableIfMaxWin(true),
			m_nCurPageNum(0),
			m_bFullscreen(true)
	{
		TCHAR buf[MAX_PATH + 1];
		GetModuleFileName(NULL, buf, MAX_PATH);
		RString module_name = buf;
		module_name = ExtractFileName(module_name);
		SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf);
		RString path = buf;
		m_strShaderFile = path + _T("\\") + module_name + _T("\\Shader\\ether.frag");
	}

	void Read(Stream &stream);
	void Write(Stream &stream) const;
	void ReadFromFile(const RString &file_name);
	void WriteToFile(const RString &file_name, int cur_page_num, bool fullscreen) const;

	void CopyLayout(const CSettings &other)
	{
		m_nColumns	= other.m_nColumns;
		m_nRows		= other.m_nRows;
	}

	bool LayoutChanged(const CSettings &rhs) const
	{
		return
			m_nColumns	!= rhs.m_nColumns ||
			m_nRows		!= rhs.m_nRows;
	}

	bool BkgChanged(const CSettings &rhs) const
	{
		return
			m_strShaderFile	!= rhs.m_strShaderFile ||
			m_nShaderSpeed	!= rhs.m_nShaderSpeed ||
			m_strImageFile	!= rhs.m_strImageFile ||
			m_nModifyImage	!= rhs.m_nModifyImage ||
			m_clrBkgColor	!= rhs.m_clrBkgColor;
	}

	bool operator !=(const CSettings &rhs) const
	{
		return
			LayoutChanged(rhs) ||
			m_nIconFontSize != rhs.m_nIconFontSize ||
			m_nIconTitleLines != rhs.m_nIconTitleLines ||
			BkgChanged(rhs) ||
			m_bScrollWheelJumps != rhs.m_bScrollWheelJumps ||
			m_bDisableIfMaxWin != rhs.m_bDisableIfMaxWin;
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CConfig
// --------------------------------------------------------------------------------------------------------------------------------------------
const float fRefResolutionX = 1920;		// reference pixel resolution
const float fRefResolutionY = 1080;
const float fHorizontalMargin = 0.5f;	// left and right screen margin in icon width-units
const float fVerticalMargin = 0.5f;		// top and bottom screen margin in icon width-units

const float fRefCols = 16.f;			// number of columns for 1920 resolution - random chosen value on which formulas rely... do not change!
const float fRefRows = 10.f;			// number of rows for 1920 resolution - random chosen value on which formulas rely... do not change!


class CConfig
{
public:
	// Der User gibt die Anzahl Spalten und Zeilen an, auf die die Icons je Seite verteilt werden sollen.
	// Es gibt feste Icon-Größen: 32x32, 48x48, 64x64, 80x80, 96x96, 112x112, 128x128
	// Das Programm zeigt dann an, welche Icon Größe es bei der aktuellen Bildschirmauflösung verwenden wird.
	int		m_nColumns;				// number of columns in Window
	int		m_nRows;				// number of rows in Window
	int		m_nIconFontSize;		// scaled size of the icon titles font
	int		m_nIconTitleLines;		// max. number of lines for icon titles

	// internal
	int		m_nWindowWidth;
	int		m_nWindowHeight;
	int		m_nBottomSpace;			// reserved scaled "pixel-units" at the window bottom to leave room for the page markers
	int		m_nIconSizeX;			// size in pixels
	int		m_nIconSizeX_2;			// half the size (for working with centered origin)
	int		m_nIconOffsetX;			// distance in pixels from one icon to another (in x and y direction)
	int		m_nIconTotalWidth;		// m_nIconSize + m_nIconOffset
	int		m_nIconTotalWidth_2;	// half of icon total width
	int		m_nTextOffset;			// number of pixels, text may exceed icon bounds to left and right
	int		m_nTextWidth;			// total horizontal space for text
	int		m_nIconSizeY;
	int		m_nIconOffsetY;
	int		m_nIconSizeY_2;
	int		m_nIconTotalHeight;
	int		m_nIconTotalHeight_2;
	float	m_fScaleFactor;			// to compute for example font size and distance of text from icon for the given resolution
	
	int		m_nHorizontalMargin;	// left and right screen margin
	int		m_nVerticalMargin;		// top and bottom screen margin

	CSfmlMemoryFont	*m_pFont;		// order of destruction is important, otherwise SFML crashes, therefore we create the objects dynamically
	sf::Text		*m_pText;

public:
	CConfig()
		:	m_pFont(nullptr),
			m_pText(nullptr)
	{
	}

	void Free()
	{
		if (m_pText)
		{
			delete m_pText;
			m_pText = nullptr;
		}

		if (m_pFont)
		{
			delete m_pFont;
			m_pFont = nullptr;
		}
	}

	~CConfig()
	{
		// Cannot call Free() here, somehow SFML is using the resources and crashes when the global CConfig
		// variable is auto-destroyed at app termination. We call Free() at the end of main().
		// Free();
	}

	// Compute icon sizes depending on available space and wanted columns
	void Setup(int window_width, int window_height);
	void ComputeLayout(const CSettings &settings);

	void SetFontSize(int size)
	{
		m_nIconFontSize = (int)(size * m_fScaleFactor);
		if (m_pText)
			m_pText->setCharacterSize(m_nIconFontSize);

	}

	// scale a constant value (eg a pixel distance) according to the scaling of the view
	int ScaleCoord(int coord) const
	{
		return (int)(m_fScaleFactor * (float)coord);
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																Globals
// --------------------------------------------------------------------------------------------------------------------------------------------
extern CConfig gConfig;
