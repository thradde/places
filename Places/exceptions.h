
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


class Exception
{
public:
	enum ExceptionCode
	{
		EXCEPTION_PLAIN_TEXT,		// no standard message
		EXCEPTION_GENERAL_FAILURE,
		EXCEPTION_OUT_OF_MEMORY,

		EXCEPTION_FILE_OPEN,
		EXCEPTION_FILE_READ,
		EXCEPTION_FILE_WRITE,

		EXCEPTION_INI_FILE_CORRUPT,

		EXCEPTION_DB_CORRUPT,
		EXCEPTION_DB_VERSION,
	};

protected:
	ExceptionCode	m_enExceptionCode;
	RString			m_strAdditionalInfo;

public:
	Exception(ExceptionCode code, const RString &additional_info = _T("")) { m_enExceptionCode = code; m_strAdditionalInfo = additional_info; }
	Exception(const RString &plain_text = _T("")) { m_enExceptionCode = EXCEPTION_PLAIN_TEXT; m_strAdditionalInfo = plain_text; }

	ExceptionCode	GetExceptionCode() const { return m_enExceptionCode; }
	const RString	GetExceptionMessage() const;
};


class CriticalException : public Exception
{
public:
	CriticalException(ExceptionCode code)
		:	Exception(code)
	{
	}
};
