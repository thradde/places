
#include <Windows.h>
#include <Windowsx.h>
#include <tchar.h>

#define ASSERT
#include "RfwString.h"


inline int my_vsntprintf(LPTSTR str, size_t size, LPCTSTR format, va_list ap)
{
	return _vsntprintf_s(str, size, _TRUNCATE, format, ap);
}


// ========================================================================
//                              CreateFormatBuffer
//
// Allocates a buffer which is large enough to print a formatted string into it.
// The required buffer-size is returned in parameter "required_size".
//
// Note: required_size is in BYTES, not characters!
//
// Note: For Non-Windows platforms the allocated buffer may be larger than
//		 what is returned in "required_size".
//
// For sample code, see RString::Format().
//
// The caller must FREE the returned buffer!
// ========================================================================
LPTSTR CreateFormatBuffer(LPCTSTR fmt, va_list args, size_t &required_size)
{
#ifdef _WINDOWS

	required_size = (size_t)_vsctprintf(fmt, args) + 1;

	TCHAR *p = (TCHAR *)malloc(required_size * sizeof(TCHAR));
	return p;

#else

	// Guess we need no more than 100 bytes.
	required_size = 100;
	int n;
	char *p, *np;

	if ((p = malloc (size)) == NULL)
		return NULL;

	while (1)
	{
		// Try to print in the allocated space.
		n = vsnprintf(p, required_size, fmt, args);

		// If that worked, return the string.
		if (n > -1 && (size_t)n < required_size)
		{
			required_size = (size_t)n + 1;
			return p;
		}

		// Else try again with more space.
		if (n > -1)					// glibc 2.1
			required_size = (size_t)n + 1;	// precisely what is needed
		else						// glibc 2.0
			required_size *= 2;		// twice the old size

		if ((np = realloc (p, required_size)) == NULL)
		{
			free(p);
			return NULL;
		}
		else
		{
			p = np;
		}
	}
#endif
}


// ==========================================================================================
//										static EmptyStringData
//
// This is an optimization
// ==========================================================================================
class RStringDataEx
{
public:
	long	nRefs;		// reference count (-1 = not counted, see EmptyString)
	size_t	nLen;		// length of string (excluding terminator), in TCHAR's!
	size_t	nBufSize;	// length of allocation (excluding terminator), in TCHAR's!
	TCHAR	Data[1];
};

static RStringDataEx EmptyStringData =
{
	-1,
	0,
	0,
	0
};


// this function ensures that EmptyString also works during initialization
RStringData *GetEmptyStringData()
{
	return (RStringData *)&EmptyStringData;
}


// ==========================================================================================
//										RString::Release()
// ==========================================================================================
void RString::Release()
{
	if (GetStringData()->nRefs > 0)
	{
		GetStringData()->Release();
		if (GetStringData()->nRefs == 0)
		{
			free(GetStringData());
			Init();
		}
	}
}

	
// ==========================================================================================
//										RString::Alloc()
//
// always allocate one extra character for '\0' termination
// ==========================================================================================
void RString::Alloc(size_t nLen)
{
	ASSERT(nLen <= INT_MAX - 1);    // max size (enough room for 1 extra)

	if (nLen == 0)
		Init();
	else if (GetStringData()->nRefs > 1 || nLen > GetStringData()->nBufSize)
	{
		Release();

		RStringData *pData = (RStringData *)malloc(sizeof(RStringData) + (nLen + 1) * sizeof(TCHAR));

		pData->nRefs = 1;
		pData->GetStringPtr()[nLen] = '\0';
		pData->nLen = nLen;
		pData->nBufSize = nLen;

		m_pchString = pData->GetStringPtr();
	}
	else
	{
		// buffer is >= requested size, re-use it and adjust str-length
		m_pchString[nLen] = '\0';
		GetStringData()->nLen = nLen;
	}
}


// ==========================================================================================
//										RString::Grow()
//
// Extends the buffer, returns the old length.
// ==========================================================================================
size_t RString::Grow(size_t nNewLength)
{
	// we have to grow the buffer
	RStringData *org_data = GetStringData();
	LPTSTR org_str = m_pchString;
	size_t org_len = GetStringData()->nLen;

	ForceAlloc(nNewLength);
	memcpy(m_pchString, org_str, org_len * sizeof(TCHAR));

	org_data->Release();
	if (org_data->nRefs == 0)
		free(org_data);

	return org_len;
}

	
// ==========================================================================================
//										RString::ForceAlloc()
//
// always allocate one extra character for '\0' termination
// ==========================================================================================
void RString::ForceAlloc(size_t nLen)
{
	ASSERT(nLen <= INT_MAX - 1);    // max size (enough room for 1 extra)

	if (nLen == 0)
		Init();
	else
	{
		RStringData *pData = (RStringData *)malloc(sizeof(RStringData) + (nLen + 1) * sizeof(TCHAR));

		pData->nRefs = 1;
		pData->GetStringPtr()[nLen] = '\0';
		pData->nLen = nLen;
		pData->nBufSize = nLen;

		m_pchString = pData->GetStringPtr();
	}
}


// ==========================================================================================
//									RString::AllocCopy()
// ==========================================================================================
void RString::AllocCopy(RString& dest, size_t nCopyLen, size_t nCopyIndex, size_t nExtraLen) const
{
	size_t nNewLen = nCopyLen + nExtraLen;
	if (nNewLen == 0)
	{
		dest.Init();
	}
	else
	{
		dest.Alloc(nNewLen);
		memcpy(dest.m_pchString, m_pchString + nCopyIndex, nCopyLen * sizeof(TCHAR));
	}
}


// ==========================================================================================
//									RString::MakeCopy()
// ==========================================================================================
void RString::MakeCopy()
{
	if (GetStringData()->nRefs > 1)
	{
		RStringData *pData = GetStringData();
		//Release();	// release is safe, since nRefs > 1 ==> tr: NOT safe, because Alloc() below calls Release() a second time!!!
		Alloc(pData->nLen);
		memcpy(m_pchString, pData->GetStringPtr(), pData->nLen * sizeof(TCHAR));
	}
}


// ==========================================================================================
//										RString::~RString
// ==========================================================================================
RString::~RString()
{
	Release();
}


// ==========================================================================================
//										RString::RString
// ==========================================================================================
RString::RString(const RString& src)
{
	ASSERT(src.GetStringData()->nRefs != 0);

	if (src.GetStringData()->nRefs >= 0)
	{
		// copy reference
		m_pchString = src.m_pchString;
		GetStringData()->AddRef();
	}
	else
		Init();
}


// ==========================================================================================
//										RString::RString
// ==========================================================================================
RString::RString(TCHAR ch, size_t nRepeat)
{
	Init();
	if (nRepeat >= 1)
	{
		Alloc(nRepeat);
#ifdef _UNICODE
		for (size_t i = 0; i < nRepeat; i++)
			m_pchString[i] = ch;
#else
		memset(m_pchString, ch, nRepeat);
#endif
	}
}


// ==========================================================================================
//										RString::RString
// ==========================================================================================
RString::RString(LPCTSTR lpsz)
{
	Init();
	size_t len = StrLen(lpsz);
	Alloc(len);
	memcpy(m_pchString, lpsz, len * sizeof(TCHAR));
}


// ==========================================================================================
//										RString::RString
// ==========================================================================================
RString::RString(LPCTSTR lpch, size_t nLength)
{
	Init();
	if (nLength != 0)
	{
		Alloc(nLength);
		memcpy(m_pchString, lpch, nLength * sizeof(TCHAR));
	}
}


// ==========================================================================================
//										RString::operator=
// ==========================================================================================
const RString &RString::operator=(const RString &stringSrc)
{
	if (m_pchString != stringSrc.m_pchString)
	{
		// copy reference
		Release();
		m_pchString = stringSrc.m_pchString;
		GetStringData()->AddRef();
	}

	return *this;
}


// ==========================================================================================
//										RString::operator=
// ==========================================================================================
const RString& RString::operator=(LPCTSTR lpsz)
{
	size_t len = StrLen(lpsz);
	Alloc(len);
	memcpy(m_pchString, lpsz, len * sizeof(TCHAR));
	return *this;
}


// ==========================================================================================
//										RString::operator=
// ==========================================================================================
const RString& RString::operator=(TCHAR ch)
{
	Alloc(1);
	*m_pchString = ch;
	return *this;
}


// ==========================================================================================
//										RString::ConcatInPlace()
// ==========================================================================================
void RString::ConcatInPlace(size_t nSrcLen, LPCTSTR lpszSrcData)
{
	if (nSrcLen == 0)
		return;

	size_t newLen = GetStringData()->nLen + nSrcLen;

	if (GetStringData()->nRefs > 1 || newLen > GetStringData()->nBufSize)
	{
		// we have to grow the buffer
		size_t org_len = Grow(newLen);
		memcpy(m_pchString + org_len, lpszSrcData, nSrcLen * sizeof(TCHAR));
	}
	else
	{
		// fast concatenation when buffer big enough
		memcpy(m_pchString + GetStringData()->nLen, lpszSrcData, nSrcLen * sizeof(TCHAR));
		GetStringData()->nLen += nSrcLen;
		m_pchString[GetStringData()->nLen] = '\0';
	}
}


// ==========================================================================================
//										RString::operator+
// ==========================================================================================
const RString& RString::operator+=(const RString& string)
{
	ConcatInPlace(string.GetStringData()->nLen, string.m_pchString);
	return *this;
}


// ==========================================================================================
//										RString::operator+
// ==========================================================================================
const RString& RString::operator+=(TCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}


// ==========================================================================================
//										RString::operator+
// ==========================================================================================
const RString& RString::operator+=(LPCTSTR lpsz)
{
	ConcatInPlace(StrLen(lpsz), lpsz);
	return *this;
}


// ==========================================================================================
//										RString::ConcatCopy
// ==========================================================================================
void RString::ConcatCopy(size_t nSrc1Len, LPCTSTR lpszSrc1Data, size_t nSrc2Len, LPCTSTR lpszSrc2Data)
{
	size_t nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		Alloc(nNewLen);
		memcpy(m_pchString, lpszSrc1Data, nSrc1Len * sizeof(TCHAR));
		memcpy(m_pchString + nSrc1Len, lpszSrc2Data, nSrc2Len * sizeof(TCHAR));
	}
}


// ==========================================================================================
//										RString::operator+
// ==========================================================================================
RString operator+(const RString& string1, const RString& string2)
{
	RString s;
	s.ConcatCopy(string1.GetStringData()->nLen, string1.m_pchString, string2.GetStringData()->nLen, string2.m_pchString);
	return s;
}


// ==========================================================================================
//										RString::operator+
// ==========================================================================================
RString operator+(const RString& string, LPCTSTR lpsz)
{
	RString s;
	s.ConcatCopy(string.GetStringData()->nLen, string.m_pchString, RString::StrLen(lpsz), lpsz);
	return s;
}


// ==========================================================================================
//										RString::operator+
// ==========================================================================================
RString operator+(LPCTSTR lpsz, const RString& string)
{
	RString s;
	s.ConcatCopy(RString::StrLen(lpsz), lpsz, string.GetStringData()->nLen, string.m_pchString);
	return s;
}


// ==========================================================================================
//										RString::operator+
// ==========================================================================================
RString operator+(const RString& string1, TCHAR ch)
{
	RString s;
	s.ConcatCopy(string1.GetStringData()->nLen, string1.m_pchString, 1, &ch);
	return s;
}


// ==========================================================================================
//										RString::operator+
// ==========================================================================================
RString operator+(TCHAR ch, const RString& string)
{
	RString s;
	s.ConcatCopy(1, &ch, string.GetStringData()->nLen, string.m_pchString);
	return s;
}


// ==========================================================================================
//										RString::find()
// ==========================================================================================
int RString::find(TCHAR ch, size_t nStart /* = 0 */) const
{
	size_t nLength = GetStringData()->nLen;
	if (nStart >= nLength)
		return -1;

	// find first single character
	LPTSTR lpsz = _tcschr(m_pchString + nStart, (_TUCHAR)ch);

	// return -1 if not found and index otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchString);
}


// ==========================================================================================
//										RString::FindOneOf()
// ==========================================================================================
int RString::FindOneOf(LPCTSTR lpszCharSet) const
{
	LPTSTR lpsz = _tcspbrk(m_pchString, lpszCharSet);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchString);
}


// ==========================================================================================
//										RString::reverse_find()
// ==========================================================================================
int RString::reverse_find(TCHAR ch) const
{
	// find last single character
	LPTSTR lpsz = _tcsrchr(m_pchString, (_TUCHAR) ch);

	// return -1 if not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchString);
}


// ==========================================================================================
//										RString::find()
// ==========================================================================================
int RString::find(LPCTSTR lpszSub, size_t nStart /* = 0 */) const
{
	size_t nLength = GetStringData()->nLen;
	if (nStart > nLength)
		return -1;

	// find first matching substring
	LPTSTR lpsz = _tcsstr(m_pchString + nStart, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchString);
}


// ==========================================================================================
//										RString::substr()
// ==========================================================================================
RString RString::substr(size_t nFirst, size_t nCount /* = 0 */) const
{
	if (nCount == 0)
		nCount = GetStringData()->nLen - nFirst;

	if (nFirst > GetStringData()->nLen)
		nCount = 0;
	else if (nFirst + nCount > GetStringData()->nLen)
		nCount = GetStringData()->nLen - nFirst;

	// optimize case of returning entire string
	if (nFirst == 0 && nFirst + nCount == GetStringData()->nLen)
		return *this;

	RString dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}


// ==========================================================================================
//										RString::left()
// ==========================================================================================
RString RString::left(size_t nCount) const
{
	if (nCount >= GetStringData()->nLen)
		return *this;

	RString dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}


// ==========================================================================================
//										RString::right()
// ==========================================================================================
RString RString::right(size_t nCount) const
{
	if (nCount >= GetStringData()->nLen)
		return *this;

	RString dest;
	AllocCopy(dest, nCount, GetStringData()->nLen-nCount, 0);
	return dest;
}


// ==========================================================================================
//										RString::MakeUpper()
// ==========================================================================================
void RString::MakeUpper()
{
	MakeCopy();
	_tcsupr(m_pchString);
}


// ==========================================================================================
//										RString::MakeLower()
// ==========================================================================================
void RString::MakeLower()
{
	MakeCopy();
	_tcslwr(m_pchString);
}


// ==========================================================================================
//										RString::MakeReverse()
// ==========================================================================================
void RString::MakeReverse()
{
	MakeCopy();
	_tcsrev(m_pchString);
}


// ==========================================================================================
//										RString::TrimLeft()
// ==========================================================================================
void RString::TrimLeft()
{
	// find first non-space character
	MakeCopy();
	LPCTSTR lpsz = m_pchString;

	while (_istspace(*lpsz))
		lpsz = _tcsinc(lpsz);

	if (lpsz != m_pchString)
	{
		// fix up data and length
		size_t nLen = GetStringData()->nLen - (lpsz - m_pchString);
		memmove(m_pchString, lpsz, (nLen + 1) * sizeof(TCHAR));
		GetStringData()->nLen = nLen;
	}
}


// ==========================================================================================
//										RString::TrimRight()
// ==========================================================================================
void RString::TrimRight()
{
	// find beginning of trailing spaces by starting at beginning (DBCS aware)
	MakeCopy();
	LPTSTR lpsz = m_pchString;
	LPTSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (_istspace(*lpsz))
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsinc(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at trailing space start
		*lpszLast = '\0';
		GetStringData()->nLen = lpszLast - m_pchString;
	}
}


// ==========================================================================================
//										RString::Trim()
// ==========================================================================================
void RString::Trim()
{
	TrimLeft();
	TrimRight();
}


// ==========================================================================================
//										RString::replace()
// ==========================================================================================
int RString::replace(TCHAR chOld, TCHAR chNew)
{
	int nCount = 0;

	if (chOld != chNew)
	{
		// modify each character that matches in the string
		MakeCopy();
		LPTSTR psz = m_pchString;
		LPTSTR pszEnd = psz + GetStringData()->nLen;
		while (psz < pszEnd)
		{
			// replace instances of the specified character only
			if (*psz == chOld)
			{
				*psz = chNew;
				nCount++;
			}
			psz = _tcsinc(psz);
		}
	}
	return nCount;
}


// ==========================================================================================
//										RString::replace()
// ==========================================================================================
int RString::replace(LPCTSTR lpszOld, LPCTSTR lpszNew)
{
	size_t nSourceLen = StrLen(lpszOld);
	if (nSourceLen == 0)
		return 0;
	size_t nReplacementLen = StrLen(lpszNew);

	// loop once to figure out the size of the result string
	int nCount = 0;
	LPTSTR lpszStart = m_pchString;
	LPTSTR lpszEnd = m_pchString + GetStringData()->nLen;
	LPTSTR lpszTarget;
	while (lpszStart < lpszEnd)
	{
		while ((lpszTarget = _tcsstr(lpszStart, lpszOld)) != NULL)
		{
			nCount++;
			lpszStart = lpszTarget + nSourceLen;
		}
		lpszStart += lstrlen(lpszStart) + 1;
	}

	// if any changes were made, make them
	if (nCount > 0)
	{
		// if the buffer is too small, just allocate a new buffer (slow but sure)
		size_t nOldLength = GetStringData()->nLen;
		size_t nNewLength =  nOldLength + (nReplacementLen - nSourceLen) * nCount;
		if (GetStringData()->nBufSize < nNewLength)
		{
			// we have to grow the buffer
			Grow(nNewLength);
		}
		else
			MakeCopy();

		// else, we just do it in-place
		lpszStart = m_pchString;
		lpszEnd = m_pchString + GetStringData()->nLen;

		// loop again to actually do the work
		while (lpszStart < lpszEnd)
		{
			while ( (lpszTarget = _tcsstr(lpszStart, lpszOld)) != NULL)
			{
				size_t nBalance = nOldLength - (lpszTarget - m_pchString + nSourceLen);
				memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen,
					nBalance * sizeof(TCHAR));
				memcpy(lpszTarget, lpszNew, nReplacementLen*sizeof(TCHAR));
				lpszStart = lpszTarget + nReplacementLen;
				lpszStart[nBalance] = '\0';
				nOldLength += (nReplacementLen - nSourceLen);
			}
			lpszStart += lstrlen(lpszStart) + 1;
		}
		GetStringData()->nLen = nNewLength;
	}

	return nCount;
}


// ==========================================================================================
//										RString::remove()
// ==========================================================================================
size_t RString::remove(size_t nIndex, size_t nCount /* = 1 */)
{
	size_t nLength = GetStringData()->nLen;
	if (nCount > 0 && nIndex < nLength)
	{
		MakeCopy();

		if (nIndex + nCount > nLength)
			nCount = nLength - nIndex;

		size_t nCharsToCopy = nLength - (nIndex + nCount) + 1;

		memmove(m_pchString + nIndex, m_pchString + nIndex + nCount, nCharsToCopy * sizeof(TCHAR));

		nLength -= nCount;
		GetStringData()->nLen = nLength;
	}

	return nLength;
}


// ==========================================================================================
//										RString::remove()
// ==========================================================================================
size_t RString::remove(TCHAR chRemove)
{
	MakeCopy();

	LPTSTR pstrSource = m_pchString;
	LPTSTR pstrDest = m_pchString;
	LPTSTR pstrEnd = m_pchString + GetStringData()->nLen;

	while (pstrSource < pstrEnd)
	{
		if (*pstrSource != chRemove)
		{
			*pstrDest = *pstrSource;
			pstrDest = _tcsinc(pstrDest);
		}
		pstrSource = _tcsinc(pstrSource);
	}
	*pstrDest = '\0';
	size_t nCount = pstrSource - pstrDest;
	GetStringData()->nLen -= nCount;

	return nCount;
}


// ==========================================================================================
//										RString::insert()
// ==========================================================================================
size_t RString::insert(size_t nIndex, TCHAR ch)
{
	size_t nNewLength = GetStringData()->nLen;
	if (nIndex > nNewLength)
		nIndex = nNewLength;
	nNewLength++;

	if (GetStringData()->nBufSize < nNewLength)
	{
		// we have to grow the buffer
		Grow(nNewLength);
	}
	else
		MakeCopy();

	// move existing bytes down
	memcpy(m_pchString + nIndex + 1, m_pchString + nIndex, (nNewLength - nIndex) * sizeof(TCHAR));
	m_pchString[nIndex] = ch;
	GetStringData()->nLen = nNewLength;

	return nNewLength;
}


// ==========================================================================================
//										RString::insert()
// ==========================================================================================
size_t RString::insert(size_t nIndex, LPCTSTR pstr)
{
	size_t nInsertLength = StrLen(pstr);
	size_t nNewLength = GetStringData()->nLen;
	if (nInsertLength > 0)
	{
		if (nIndex > nNewLength)
			nIndex = nNewLength;
		nNewLength += nInsertLength;

		if (GetStringData()->nBufSize < nNewLength)
		{
			Grow(nNewLength);
		}
		else
			MakeCopy();

		// move existing bytes down
		memcpy(m_pchString + nIndex + nInsertLength, m_pchString + nIndex,
			(nNewLength - nIndex - nInsertLength + 1) * sizeof(TCHAR));
		memcpy(m_pchString + nIndex, pstr, nInsertLength * sizeof(TCHAR));
		GetStringData()->nLen = nNewLength;
	}

	return nNewLength;
}


// ==========================================================================================
//										RString::Format()
// ==========================================================================================
void RString::Format(LPCTSTR lpszFormat, ...)
{
	va_list argList;
	size_t required_size;

	va_start(argList, lpszFormat);

	LPTSTR buffer = CreateFormatBuffer(lpszFormat, argList, required_size);
	if (buffer)
	{
		my_vsntprintf(buffer, required_size, lpszFormat, argList);

		required_size--;	// exclude terminating zero
		if (GetStringData()->nBufSize < required_size)
		{
			Grow(required_size);
		}
		else
			MakeCopy();

		// copy
		memcpy(m_pchString, buffer, required_size + 1);	// +1: include terminating zero
		GetStringData()->nLen = required_size;

		free(buffer);
	}
	else
		erase();

	va_end(argList);
}


// ==========================================================================================
//										RString::Tokenize()
// ==========================================================================================
static bool is_delimiter(TCHAR c, LPCTSTR delimiters)
{
	LPCTSTR p = delimiters;
	while (*p)
	{
		if (*p == c)
			return true;
		p++;
	}

	return false;
}


RString RString::Tokenize(LPCTSTR delimiters, int &nStart)
{
	size_t len = GetStringData()->nLen;

	if (nStart < 0 || (size_t)nStart >= len)
	{
		nStart = -1;
		return RString();
	}

	if (!delimiters || *delimiters == _T('\0'))
	{
		nStart = -1;
		return RString();
	}

	// skip leading delimiters
	LPTSTR	p = m_pchString + nStart;
	while ((size_t)nStart < len && is_delimiter(*p, delimiters))
	{
		nStart++;
		p++;
	}

	if ((size_t)nStart >= len)
	{
		nStart = -1;
		return RString();
	}

	LPTSTR start = p;
	while ((size_t)nStart < len && !is_delimiter(*p, delimiters))
	{
		nStart++;
		p++;
	}

	nStart++;

	return RString(start, p - start);
}

/*	TEST CODE:

	RString str = "bla;blub,fasel";
	int nStart = 0;
	RString token = str.Tokenize(";.,", nStart);
	while (nStart >= 0)
	{
		token = str.Tokenize(";.,", nStart);
	}
*/
