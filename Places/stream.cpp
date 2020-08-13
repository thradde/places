
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

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memdebug.h"

#define ASSERT
#include "RfwString.h"
#include "exceptions.h"
#include "Stream.h"


// ========================================================================
//                            TestWriteAccess
// ========================================================================
bool TestWriteAccess(const RString &fname)
{
#ifndef W_OK
	#define	W_OK	2	// MS Visual C
#endif
	return _taccess(fname.c_str(), W_OK) == 0;
}


// =========================================================================
//                            Stream - Constructor
// =========================================================================
Stream::Stream(const RString &fname, const RString &mode)
	:	m_pExtra(nullptr)
{
	fh = _tfopen(fname.c_str(), mode.c_str());
	if (!fh)
		throw Exception(Exception::EXCEPTION_FILE_OPEN, (RString)_T("File: ") + fname);

	m_FileName = fname;
}


// =========================================================================
//                          Stream - Destructor
// =========================================================================
Stream::~Stream()
{
	if (fh)
		fclose(fh);
}


// =========================================================================
//                          Stream - Eof
// =========================================================================
bool Stream::Eof()
{
	if (fh)
		return feof(fh) > 0;

	return true;
}



// =========================================================================
//                         Stream - Write
// =========================================================================
void Stream::Write(void const *buf, size_t count)
{
	if (fh)
	{
		if (fwrite(buf, 1, count, fh) != count)
			throw Exception(Exception::EXCEPTION_FILE_WRITE, (RString)_T("File: ") + m_FileName);
	}
}


// =========================================================================
//                     Stream - WriteByte
// =========================================================================
void Stream::WriteByte(BYTE buf)
{
	Write(&buf, sizeof(buf));
}


// =========================================================================
//                     Stream - WriteBool
// =========================================================================
void Stream::WriteBool(bool buf)
{
	if (buf)
		WriteByte(1);
	else
		WriteByte(0);
}


// =========================================================================
//                     Stream - WriteWord
// =========================================================================
void Stream::WriteWord(WORD buf)
{
	Write(&buf, sizeof(WORD));
}


// =========================================================================
//                     Stream - WriteInt
//
// Data in the stream is always organized in Little Endian Format!
// On Big Endian Machines, conversion is performed automatically.
// =========================================================================
void Stream::WriteInt(int buf)
{
	Write(&buf, sizeof(buf));
}


// =========================================================================
//                     Stream - WriteUInt
// =========================================================================
void Stream::WriteUInt(unsigned int buf)
{
	Write(&buf, sizeof(buf));
}


// =========================================================================
//                     Stream - WriteInt64
// =========================================================================
void Stream::WriteInt64(__int64 buf)
{
	Write(&buf, sizeof(buf));
}


// =========================================================================
//                     Stream - WriteString
// =========================================================================
void Stream::WriteString(const RString &s)
{
	unsigned int len = (unsigned int)s.length();
	Write(&len, sizeof(len));
	Write(s.c_str(), _tcssize(len));
}


// =========================================================================
//                         Stream - Read
// =========================================================================
void Stream::Read(void *buf, size_t count)
{
   if (fh)
   {
      if (fread(buf, 1, count, fh) != count)
		  throw Exception(Exception::EXCEPTION_FILE_READ, (RString)_T("File: ") + m_FileName);
   }
}


// =========================================================================
//                     Stream - ReadByte
// =========================================================================
BYTE Stream::ReadByte()
{
	BYTE buf;
	Read(&buf, sizeof(buf));
	return buf;
}


// =========================================================================
//                     Stream - ReadBool
// =========================================================================
bool Stream::ReadBool()
{
	BYTE b = ReadByte();
	return b == 1;
}


// =========================================================================
//                     Stream - ReadWord
// =========================================================================
WORD Stream::ReadWord()
{
	WORD buf;
	Read(&buf, sizeof(buf));
	return buf;
}


// =========================================================================
//                     Stream - ReadInt
// =========================================================================
int Stream::ReadInt()
{
	int buf;
	Read(&buf, sizeof(buf));
	return buf;
}


// =========================================================================
//                     Stream - ReadUInt
// =========================================================================
unsigned int Stream::ReadUInt()
{
	unsigned int buf;
	Read(&buf, sizeof(buf));
	return buf;
}


// =========================================================================
//                     Stream - ReadInt64
// =========================================================================
__int64 Stream::ReadInt64()
{
	__int64 buf;
	Read(&buf, sizeof(buf));
	return buf;
}


// =========================================================================
//                     Stream - ReadString
// =========================================================================
RString Stream::ReadString()
{
	unsigned int len;
	TCHAR *buf = nullptr;

	len = ReadUInt();
	if (len)
	{
		buf = (TCHAR *)malloc((size_t)_tcssize(len + 1));
		Read(buf, (size_t)_tcssize(len));
		buf[len] = _T('\0');
	}

	RString s(buf);
	free(buf);

  	return s;
}
