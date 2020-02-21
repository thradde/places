
#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#include <Shlobj.h>

#include "memdebug.h"

#include <SFML/Graphics.hpp>

#include <vector>
using namespace std;

#define ASSERT
#include "hook.h"
#include "RfwString.h"
#include "exceptions.h"
#include "stream.h"
#include "platform.h"
#include "generic.h"
#include "configuration.h"


#define SETTINGS_MAGIC		_T("PlacesSettings")
#define SETTINGS_VERSION	1


// --------------------------------------------------------------------------------------------------------------------------------------------
//																Globals
// --------------------------------------------------------------------------------------------------------------------------------------------
CConfig gConfig;


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CSettings::Read()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CSettings::Read(Stream &stream)
{
	m_nColumns = stream.ReadInt();
	m_nRows = stream.ReadInt();
	m_nIconFontSize = stream.ReadInt();
	m_nIconTitleLines = stream.ReadInt();

	m_strShaderFile = stream.ReadString();
	m_nShaderSpeed = stream.ReadInt();
	m_strImageFile = stream.ReadString();
	m_nModifyImage = stream.ReadInt();
	m_clrBkgColor = stream.ReadUInt();

	m_bScrollWheelJumps = stream.ReadBool();
	m_bDisableIfMaxWin = stream.ReadBool();

	gbHotkeyVKey = stream.ReadByte();		// virtual key code for app activation
	gbHotkeyModifiers = stream.ReadByte();	// modifiers for app activation (shift, ctrl, alt, win)
	gnMouseButton = stream.ReadInt();
	gnMouseCorner = stream.ReadInt();
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CSettings::Read()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CSettings::Write(Stream &stream) const
{
	stream.WriteInt(m_nColumns);
	stream.WriteInt(m_nRows);
	stream.WriteInt(m_nIconFontSize);
	stream.WriteInt(m_nIconTitleLines);

	stream.WriteString(m_strShaderFile);
	stream.WriteInt(m_nShaderSpeed);
	stream.WriteString(m_strImageFile);
	stream.WriteInt(m_nModifyImage);
	stream.WriteUInt(m_clrBkgColor);

	stream.WriteBool(m_bScrollWheelJumps);
	stream.WriteBool(m_bDisableIfMaxWin);

	stream.WriteByte(gbHotkeyVKey);			// virtual key code for app activation
	stream.WriteByte(gbHotkeyModifiers);	// modifiers for app activation (shift, ctrl, alt)
	stream.WriteInt(gnMouseButton);
	stream.WriteInt(gnMouseCorner);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CSettings::Read()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CSettings::ReadFromFile(const RString &file_name)
{
	Stream stream(file_name, _T("rb"));

	// Magic
	RString magic = stream.ReadString();
	if (magic != SETTINGS_MAGIC)
		throw Exception(Exception::EXCEPTION_DB_CORRUPT);

	// Version
	if (stream.ReadUInt() > SETTINGS_VERSION)			// this implies we can always read older versions
		throw Exception(Exception::EXCEPTION_DB_VERSION);

	// read settings
	Read(stream);
	m_nCurPageNum = stream.ReadInt();
	m_bFullscreen = stream.ReadBool();
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CSettings::Read()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CSettings::WriteToFile(const RString &file_name, int cur_page_num, bool fullscreen) const
{
	_trename(file_name.c_str(), (file_name + _T(".bak")).c_str());
	Stream stream(file_name, _T("wb"));

	// Magic + Version
	stream.WriteString(SETTINGS_MAGIC);
	stream.WriteUInt(SETTINGS_VERSION);

	// write settings
	Write(stream);
	stream.WriteInt(cur_page_num);
	stream.WriteBool(fullscreen);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CConfig::Setup()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CConfig::Setup(int window_width, int window_height)
{
	// no need to free m_pFont here, it can be re-used
	if (m_pText)
	{
		delete m_pText;
		m_pText = nullptr;
	}

	m_nWindowWidth = window_width;
	m_nWindowHeight = window_height;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CConfig::ComputeLayout()
//
// Compute icon sizes depending on available space and wanted columns
// --------------------------------------------------------------------------------------------------------------------------------------------
void CConfig::ComputeLayout(const CSettings &settings)
{
	int cols = settings.m_nColumns;
	int max_cols = m_nWindowWidth / 40;	// max. number of columns, min. 24 pixels icon + 16 pixels offset to next icon
	if (cols > max_cols)
		cols = max_cols;

	int rows = settings.m_nRows;
	int max_rows = m_nWindowHeight / 40;	// max. number of rows
	if (rows > max_rows)
		rows = max_rows;

	/*
	float xfactor = ((float)m_nWindowWidth / (float)cols) / (fRefResolutionX / fRefCols);
	float yfactor = ((float)m_nWindowHeight / (float)rows) / (fRefResolutionY / fRefRows);
	m_fScaleFactor = min(xfactor, yfactor);
	*/

	// compute the scale factor (for font and other stuff)
	float horz_scale = (float)m_nWindowWidth / fRefResolutionX;
	float vert_scale = (float)m_nWindowHeight / fRefResolutionY;
	m_fScaleFactor = min(horz_scale, vert_scale);

	// faktor war 100
	m_nBottomSpace = (int)(50 * m_fScaleFactor);	// reserve 80 "pixel-units" at the bottom to leave room for the page markers
	int window_height = m_nWindowHeight - m_nBottomSpace;
	float pixel_width = (float)m_nWindowWidth / (2.f * ((float)cols + fHorizontalMargin));
	float pixel_height = (float)window_height / (2.f * ((float)rows + fVerticalMargin));
	int min_dim = (int)min(pixel_width, pixel_height);

	m_nIconSizeX = min_dim;
	m_nIconOffsetX = (int)(2 * pixel_width - m_nIconSizeX);
	m_nIconSizeX_2 = m_nIconSizeX / 2;
	m_nIconTotalWidth = m_nIconSizeX + m_nIconOffsetX;
	m_nIconTotalWidth_2 = m_nIconTotalWidth / 2;
	m_nTextOffset = m_nIconOffsetX / 3;					// number of pixels, text may exceed icon bounds to left and right
	m_nTextWidth = m_nIconSizeX + m_nTextOffset * 2;	// total horizontal space for text

	m_nIconSizeY = min_dim;
	m_nIconOffsetY = (int)(2 * pixel_height - m_nIconSizeY);
	m_nIconSizeY_2 = m_nIconSizeY / 2;
	m_nIconTotalHeight = m_nIconSizeY + m_nIconOffsetY;
	m_nIconTotalHeight_2 = m_nIconTotalHeight / 2;

	m_nBottomSpace += m_nIconOffsetY;

	//min_dim = min(m_nIconTotalWidth, m_nIconTotalHeight);
	//m_fScaleFactor = (float)min_dim / (fRefIconPixelsX + fRefIconOffset);

	// remainder is the window width / height minus the total space occupied by the icons
	m_nColumns = cols;
	int horz_remainder = m_nWindowWidth - cols * m_nIconTotalWidth;
	m_nHorizontalMargin = horz_remainder / 2;

	m_nRows = rows;
	int vert_remainder = window_height - m_nRows * m_nIconTotalHeight - (int)(fVerticalMargin * m_nIconTotalHeight);
	m_nVerticalMargin = vert_remainder / 2;

	SetFontSize(settings.m_nIconFontSize);
	m_nIconTitleLines = settings.m_nIconTitleLines;
}
