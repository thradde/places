//
//	Exceptions.cpp
//	==============
//
//	11 / 2003 T. Radde
//


#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#include <tchar.h>

#define ASSERT
#include "RfwString.h"
#include "exceptions.h"


static const TCHAR *ExceptionMessage[] =
{
	_T(""),
	_T("General Failure"),
	_T("Out of Memory"),

	_T("Could not open or create the file"),
	_T("Error reading from file"),
	_T("Error writing to file"),

	_T("INI file corrupt"),

	_T("Structure of database file corrupted"),
	_T("Database version incompatible"),
};


const RString Exception::GetExceptionMessage() const
{
	if (m_enExceptionCode == EXCEPTION_PLAIN_TEXT)
		return m_strAdditionalInfo;
	else
		return (RString)ExceptionMessage[m_enExceptionCode] + _T("\n\n") + m_strAdditionalInfo;
}
