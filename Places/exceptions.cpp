
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


#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#include <tchar.h>

#include "memdebug.h"

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
