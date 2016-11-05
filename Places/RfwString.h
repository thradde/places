
#define _tcschars(s)	(sizeof(s) / sizeof(TCHAR))
#define _tcssize(len)	((len) * sizeof(TCHAR))


// ==========================================================================================
//											RStringData
//
// Reference counted buffer
// ==========================================================================================
class RStringData
{
public:
	long	nRefs;		// reference count (-1 = not counted, see EmptyString)
	size_t	nLen;		// length of string (excluding terminator), in TCHAR's!
	size_t	nBufSize;	// length of allocation (excluding terminator), in TCHAR's!

	// TCHAR Data[nBufSize] follows here

	TCHAR	*GetStringPtr()	{ return (TCHAR *)(this + 1); }

	void	AddRef()	{ if (nRefs >= 0) InterlockedIncrement(&nRefs); }		// do not increment, if referencing EmptyString
	void	Release()	{ ASSERT(nRefs > 0); InterlockedDecrement(&nRefs); }
};


RStringData *GetEmptyStringData();


// ==========================================================================================
//											class RString
// ==========================================================================================
class RString
{
protected:
	LPTSTR	m_pchString;		// this is tricky: we store the pointer to the string, not to RStringData!

	// to get RStringData, we use this method:
	RStringData	*GetStringData() const { ASSERT(m_pchString != NULL); return ((RStringData *)m_pchString) - 1; }

	void	Init() { m_pchString = GetEmptyStringData()->GetStringPtr(); }

	void	Release();
	void	Alloc(size_t nLen);
	void	ForceAlloc(size_t nLen);
	size_t	Grow(size_t nNewLength);
	void	AllocCopy(RString& dest, size_t nCopyLen, size_t nCopyIndex, size_t nExtraLen) const;
	void	MakeCopy();
	void	ConcatInPlace(size_t nSrcLen, LPCTSTR lpszSrcData);
	void	ConcatCopy(size_t nSrc1Len, LPCTSTR lpszSrc1Data, size_t nSrc2Len, LPCTSTR lpszSrc2Data);

	static size_t StrLen(LPCTSTR lpsz) { return (lpsz == NULL) ? 0 : _tcslen(lpsz); }

public:
	// constructs empty RString
	RString() { Init(); }
	~RString();

	// copy constructor
	RString(const RString& src);

	// from a single character
	RString(TCHAR ch, size_t nRepeat = 1);

	// from a C-string
	RString(LPCTSTR lpsz);

	// subset of characters from a C-string
	RString(LPCTSTR lpch, size_t nLength);

	// from unsigned characters
	//RString(const unsigned char *lpsz) { Init(); *this = (LPCTSTR)lpsz; }

	operator LPCTSTR() const { return m_pchString; }

	LPCTSTR c_str() const { return m_pchString; }

	size_t	length() const { return GetStringData()->nLen; }
	size_t	GetLength() const { return GetStringData()->nLen; }

	bool	empty() const { return GetStringData()->nLen == 0; }

	void	erase()
	{
		if (!empty())
		{
			Release();
			Init();
		}
	}

	void clear()
	{
		erase();
	}

	TCHAR RString::GetAt(size_t nIndex) const
	{
		ASSERT(nIndex < GetStringData()->nLen);
		return m_pchString[nIndex];
	}

	// parameter is int instead of size_t, otherwise it clashes with a built-in operator
	TCHAR RString::operator[](int nIndex) const
	{
		// same as GetAt
		ASSERT(nIndex < (int)GetStringData()->nLen);
		return m_pchString[nIndex];
	}

	// ref-counted copy from another RString
	const RString& operator=(const RString& stringSrc);

	// copy string content from a C-string
	const RString& operator=(LPCTSTR lpsz);

	// set string content to single character
	const RString& operator=(TCHAR ch);

	// copy string content from unsigned chars
	const RString& operator=(const unsigned char* lpsz) { *this = (LPCTSTR)lpsz; return *this; }

	// concatenate from another RString
	const RString& operator+=(const RString& string);

	// concatenate a single character
	const RString& operator+=(TCHAR ch);

	// concatenate a C-string
	const RString& operator+=(LPCTSTR lpsz);

	friend RString operator+(const RString& string1, const RString& string2);
	friend RString operator+(const RString& string, LPCTSTR lpsz);
	friend RString operator+(LPCTSTR lpsz, const RString& string);
	friend RString operator+(const RString& string, TCHAR ch);
	friend RString operator+(TCHAR ch, const RString& string);

	// straight character comparison
	int Compare(LPCTSTR lpsz) const { return _tcscmp(m_pchString, lpsz); }    // MBCS/Unicode aware
	int Cmp(LPCTSTR lpsz) const { return _tcscmp(m_pchString, lpsz); }    // MBCS/Unicode aware

	// compare ignoring case
	int CompareNoCase(LPCTSTR lpsz) const { return _tcsicmp(m_pchString, lpsz); }   // MBCS/Unicode aware
	
	// NLS aware comparison, case sensitive.
	// Collate is often slower than Compare but is MBSC/Unicode aware as well as locale-sensitive with respect to sort order.
	int Collate(LPCTSTR lpsz) const { return _tcscoll(m_pchString, lpsz); }   // locale sensitive
	
	// NLS aware comparison, case insensitive
	int CollateNoCase(LPCTSTR lpsz) const { return _tcsicoll(m_pchString, lpsz); }   // locale sensitive

	// find character starting at zero-based index and going right
	int find(TCHAR ch, size_t nStart = 0) const;

	// find first instance of any character in passed string
	int FindOneOf(LPCTSTR lpszCharSet) const;

	// find character starting at right
	int reverse_find(TCHAR ch) const;

	// find first instance of substring starting at zero-based index
	int find(LPCTSTR lpszSub, size_t nStart = 0) const;
	int Find(LPCTSTR lpszSub, size_t nStart = 0) const
	{
		return find(lpszSub, nStart);
	}

	// return nCount characters starting at zero-based nFirst
	RString substr(size_t nFirst, size_t nCount = 0) const;
	RString Mid(size_t nFirst, size_t nCount = 0) const
	{
		return substr(nFirst, nCount);
	}

	// return first nCount characters in string
	RString left(size_t nCount) const;
	RString Left(size_t nCount) const
	{
		return left(nCount);
	}

	// return nCount characters from end of string
	RString right(size_t nCount) const;
	RString Right(size_t nCount) const
	{
		return right(nCount);
	}

	// NLS aware conversion to uppercase
	void MakeUpper();

	// NLS aware conversion to lowercase
	void MakeLower();

	// reverse string right-to-left
	void MakeReverse();

	// remove whitespace starting from left side
	void TrimLeft();

	// remove whitespace starting from right edge
	void TrimRight();

	// remove whitespace left and right
	void Trim();

	// replace occurrences of chOld with chNew
	int replace(TCHAR chOld, TCHAR chNew);

	// replace occurrences of substring lpszOld with lpszNew;
	// empty lpszNew removes instances of lpszOld
	int replace(LPCTSTR lpszOld, LPCTSTR lpszNew);

	// delete nCount characters starting at zero-based index
	size_t remove(size_t nIndex, size_t nCount = 1);

	// remove occurrences of chRemove
	size_t remove(TCHAR chRemove);

	// insert character at zero-based index; concatenates
	// if index is past end of string
	size_t insert(size_t nIndex, TCHAR ch);

	// insert substring at zero-based index; concatenates
	// if index is past end of string
	size_t insert(size_t nIndex, LPCTSTR pstr);

	// format a string
	void Format(LPCTSTR lpszFormat, ...);

	// Tokenize a string
	RString Tokenize(LPCTSTR delimiters, int &nStart);
};


// Compare helpers
inline bool operator==(const RString& s1, const RString& s2)
	{ return s1.Compare(s2) == 0; }

inline bool operator==(const RString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) == 0; }

inline bool operator==(LPCTSTR s1, const RString& s2)
	{ return s2.Compare(s1) == 0; }

inline bool operator!=(const RString& s1, const RString& s2)
	{ return s1.Compare(s2) != 0; }

inline bool operator!=(const RString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) != 0; }

inline bool operator!=(LPCTSTR s1, const RString& s2)
	{ return s2.Compare(s1) != 0; }

inline bool operator<(const RString& s1, const RString& s2)
	{ return s1.Compare(s2) < 0; }

inline bool operator<(const RString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) < 0; }

inline bool operator<(LPCTSTR s1, const RString& s2)
	{ return s2.Compare(s1) > 0; }

inline bool operator>(const RString& s1, const RString& s2)
	{ return s1.Compare(s2) > 0; }

inline bool operator>(const RString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) > 0; }

inline bool operator>(LPCTSTR s1, const RString& s2)
	{ return s2.Compare(s1) < 0; }

inline bool operator<=(const RString& s1, const RString& s2)
	{ return s1.Compare(s2) <= 0; }

inline bool operator<=(const RString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) <= 0; }

inline bool operator<=(LPCTSTR s1, const RString& s2)
	{ return s2.Compare(s1) >= 0; }

inline bool operator>=(const RString& s1, const RString& s2)
	{ return s1.Compare(s2) >= 0; }

inline bool operator>=(const RString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) >= 0; }

inline bool operator>=(LPCTSTR s1, const RString& s2)
	{ return s2.Compare(s1) <= 0; }
