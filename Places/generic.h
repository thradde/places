

// --------------------------------------------------------------------------------------------------------------------------------------------
//													Macros
// --------------------------------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------------------------------
//												ManhattanDistance
// --------------------------------------------------------------------------------------------------------------------------------------------
inline int ManhattanDistance(const sf::Vector2i pos, int dx, int dy)
{
	int ddx = dx - pos.x;
	int ddy = dy - pos.y;

	return (int)sqrtf((float)(ddx * ddx + ddy * ddy));
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//													Prototypes
// --------------------------------------------------------------------------------------------------------------------------------------------
RString ExtractFileName(const RString &path);


// --------------------------------------------------------------------------------------------------------------------------------------------
//													class CSfmlApp
//
// Generic SFML application class
// --------------------------------------------------------------------------------------------------------------------------------------------
class CSfmlApp
{
protected:
	enum EWindowStyle
	{
		enWsFullscreen,
		enWsWindowed,
	};

protected:
	sf::RenderWindow	m_Window;			// window must be created by caller using create()
	bool				m_bRequestClose;	// true if the window shall be closed
	EWindowStyle		m_enWindowStyle;	// either fullscreen or in a window
	bool				m_bRecreateWindow;	// if true, the window shall be recreated, considering m_enWindowStyle

public:
	CSfmlApp()
		:	m_bRequestClose(false),
			m_enWindowStyle(enWsFullscreen),
			m_bRecreateWindow(false)
	{
	}

	sf::RenderWindow &GetRenderWindow() { return m_Window; }

	virtual void ShowMainWindow()
	{
		ShowWindow(m_Window.getSystemHandle(), SW_SHOW);
	}

	virtual void CloseMainWindow()
	{
		m_bRequestClose = true;
	}

	virtual void CreateMainWindow() {}

	virtual void OnMouseDown(const sf::Event &event) {}
	virtual void OnMouseUp(const sf::Event &event) {}
	virtual void OnMouseMove(const sf::Event &event) {}
	virtual void OnMouseWheel(const sf::Event &event) {}
	virtual void OnKeyPressed(const sf::Event &event) {}

	virtual void OnDroppedFiles(const sf::Event &event) {}

	virtual bool OnClose() { return true; }		// return false to prevent closing

	virtual void HandleEvents();
	
	virtual void Work() {}		// is called after HandleEvents(), allows to perform any "work"
	
	virtual void Draw() {}		// is called after Work()

	virtual void Run();
};
