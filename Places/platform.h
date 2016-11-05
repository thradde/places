
// --------------------------------------------------------------------------------------------------------------------------------------------
//															Platform specific code
//
// When porting to another platform, the functions contained in here need to be rewritten.
//
// NOTE: This comment was written at the beginning of the project.
//		 Meanwhile it turned out that Windows specific code had to be injected at several different places.
//		 Gaining platform independence will require a bigger cleanup.
// --------------------------------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------------------------------
//																Globals
// --------------------------------------------------------------------------------------------------------------------------------------------
extern RString gstrAppData;
extern RString gstrCommonAppData;
extern RString gstrWindir;


// --------------------------------------------------------------------------------------------------------------------------------------------
//																Prototypes
// --------------------------------------------------------------------------------------------------------------------------------------------
bool GetAppPaths();		// sets gstrAppData and gstrCommonAppData
RString GetShellItemName(const RString &strFilePath);
RString GetVirtKeyName(BYTE keycode);
RString GetKeyComboString(BYTE hotkey, BYTE virtkey);
HRESULT ResolveLink(HWND hwnd, const RString &strLinkFile, RString &path);
HWND GetDesktopListViewHWND();
void SetForegroundWindowInternal(HWND hWnd);
bool IsFullscreenAndMaximized(HWND hwnd);
bool ThereIsAFullscreenWin();
bool GetTaskbarRect(RECT &rc);


// --------------------------------------------------------------------------------------------------------------------------------------------
//															class CSfmlMemoryFont
//
// We retrieve the binary font data through Windows GDI.
// Unfortunately SFML does not create a copy of the binary font data, so we need to hold it in memory
// until the application terminates.
// --------------------------------------------------------------------------------------------------------------------------------------------
class CSfmlMemoryFont
{
protected:
	void		*m_pBinaryFontData;
	sf::Font	m_Font;

public:
	CSfmlMemoryFont()
		:	m_pBinaryFontData(nullptr)
	{
	}

	~CSfmlMemoryFont()
	{
		if (m_pBinaryFontData)
			free(m_pBinaryFontData);
	}

	void Create(const sf::Window &window, const RString &strFontName);

	sf::Font &GetSfmlFont()
	{
		return m_Font;
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//															class CFileDialog
// --------------------------------------------------------------------------------------------------------------------------------------------
#define MY_MAX_PATH		4096
#define MY_MAX_EXT		 256

class CFileDialog
{
protected:
	OPENFILENAME	m_Ofn;
	RString			m_strPath;
	TCHAR			m_szPath[MY_MAX_PATH + MY_MAX_EXT + 1];		// used for OFN-structure
	DWORD			m_dwFlags;
	LPCTSTR			m_pchFilter;
	int				m_nDefaultFilter;

protected:
	void InitOfn(HWND hwnd_owner);
	void ExitOfn();

public:
	CFileDialog(const RString &path = _T(""), DWORD flags = 0, LPCTSTR filter = NULL, int default_filter = 1);
	bool GetOpenName(HWND hwnd_owner);
	bool GetSaveName(HWND hwnd_owner);
	const RString &GetPath() const { return m_strPath; }
	void SetFilterIndexFromFileName();	// Analyses the file name extension and sets the DefaultFilterIndex accordingly
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//															class CTaskbar
// --------------------------------------------------------------------------------------------------------------------------------------------
class CTaskbar
{
public:
	RECT	m_rcPosition;
	int		m_nWidth;
	int		m_nHeight;

protected:
	void ComputeDimensions(const RECT &rc)
	{
		m_rcPosition = rc;
		m_nHeight = m_rcPosition.bottom - m_rcPosition.top;
		m_nWidth = m_rcPosition.right - m_rcPosition.left;
	}

public:
	CTaskbar()
	{
		GetDimensions();
	}


	void GetDimensions()
	{
		RECT rc;
		if (GetTaskbarRect(rc))
			ComputeDimensions(rc);
	}


	bool ChangedPosition()
	{
		RECT rc;
		if (GetTaskbarRect(rc))
		{
			if (m_rcPosition.left != rc.left ||
				m_rcPosition.top != rc.top ||
				m_rcPosition.right != rc.right ||
				m_rcPosition.bottom != rc.bottom)
			{
				ComputeDimensions(rc);
				return true;
			}
		}

		return false;
	}
};
