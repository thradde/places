
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


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CMD5Hash
// MD5 hash
// --------------------------------------------------------------------------------------------------------------------------------------------
class CMD5Hash
{
protected:
	enum { nHashSize = 16 };

	unsigned char m_HashValue[nHashSize];

public:
	CMD5Hash()
	{
		memset(m_HashValue, 0, nHashSize);		// if all zero, functions as file / bitmap not found symbol
	}

	void Read(Stream &stream)
	{
		stream.Read(m_HashValue, nHashSize);
	}

	void Write(Stream &stream) const
	{
		stream.Write(m_HashValue, nHashSize);
	}

	unsigned char *GetData() { return m_HashValue; }

	bool operator ==(const CMD5Hash &rhs) const
	{
		return memcmp(m_HashValue, rhs.m_HashValue, nHashSize) == 0;
	}

	bool operator < (const CMD5Hash &rhs) const
	{
		return memcmp(m_HashValue, rhs.m_HashValue, nHashSize) < 0;
	}
};


class CMD5
{
protected:
	MD5_CTX	m_Md5Context;

public:
	CMD5()
	{
	}

	void Init()
	{
		MD5_Init(&m_Md5Context);
	}

	void Update(const void *data, unsigned long size)
	{
		MD5_Update(&m_Md5Context, data, size);
	}

	void Final(CMD5Hash &hash_value)
	{
		MD5_Final(hash_value.GetData(), &m_Md5Context);
	}

	// creates an MD5 in one pass
	void OnePass(CMD5Hash &hash_value, const void *data, unsigned long size)
	{
		Init();
		Update(data, size);
		Final(hash_value);
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CSize
// --------------------------------------------------------------------------------------------------------------------------------------------
class CSize
{
public:
	int	x;
	int y;

public:
	CSize()
		:	x(0),
			y(0)
	{
	}

	CSize(int xx, int yy)
		:	x(xx),
			y(yy)
	{
	}
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CBitmap
// Storage format of bitmap data is always RGBA
// --------------------------------------------------------------------------------------------------------------------------------------------
class CBitmap
{
protected:
	CSize	m_Size;		// size in pixels
	BYTE	*m_pBitmap;	// bitmap data

public:
	CBitmap(const CSize &size, BYTE *bitmap)
		:	m_Size(size),
			m_pBitmap(bitmap)
	{
	}

	CBitmap()
		:	m_pBitmap(nullptr)
	{
	}

	~CBitmap()
	{
		FreeBitmap();
	}

	void FreeBitmap()
	{
		if (m_pBitmap)
		{
			delete[] m_pBitmap;
			m_pBitmap = nullptr;
		}
	}

	const CSize &GetSize() const { return m_Size; }

	void SetSize(const CSize &size)
	{
		m_Size = size;
	}

	BYTE *GetBitmapData() const
	{
		return m_pBitmap;
	}

	void SetBitmapData(BYTE *bitmap)
	{
		FreeBitmap();
		m_pBitmap = bitmap;
	}

	unsigned int GetDataSize() const
	{
		return m_Size.x * m_Size.y * 4;		// RGBA = 4 bytes per pixel
	}

	void Read(Stream &stream);
	void Write(Stream &stream) const;
};


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CBitmapCacheItem
// --------------------------------------------------------------------------------------------------------------------------------------------
const int nNumBitmapSizes = 4;

static const int arBitmapSizes[nNumBitmapSizes] =
{
	16, 32, 48, 256		// pixels
};

class CBitmapCacheItem
{
protected:
	CMD5Hash		m_Hash;						// hash value of bitmap data for biggest bitmap, this works as a unique ID
	CBitmap			m_Bitmaps[nNumBitmapSizes];	// available icon bitmaps in different sizes, nullptr if unavailable
	CBitmap			m_DrawBitmap;				// same bitmap as in m_DrawTexture. used for fast file streaming only.
	sf::Texture		m_DrawTexture;				// SFML-Texture, which is exactly scaled to the required resolution for GUI drawing
	unsigned long	m_nRefCount;				// reference counter
	bool			m_bHasError;				// true if the image could not be retrieved
	bool			m_bDeletable;				// the "file not found" bitmap can not be deleted
	CMutex			m_Mutex;

protected:
	bool RetrieveShellIcon(const RString &strFilePath);		// uses shell api
	bool LoadBitmapFile(const RString &strFilePath);		// loads bitmap from external bitmap graphics file

public:
	CBitmapCacheItem(const RString &strFilePath, bool use_shell_api = true);
	
	CBitmapCacheItem(Stream &stream);
	void Write(Stream &stream) const;
	
	CBitmapCacheItem(const CMD5Hash &md5hash)	// to be used as search-key
		:	m_Hash(md5hash)
	{
	}

	const CMD5Hash &GetHash() const { return m_Hash; }

	bool IsEqual(const CBitmapCacheItem *other) const
	{
		return m_Hash == other->m_Hash;
	}

	void CreateScaledDrawTexture(int pixels);

	const sf::Texture &GetDrawTexture() const
	{
		return m_DrawTexture;
	}

	void SetDeletable(bool yes_no)
	{
		m_bDeletable = yes_no;
	}

	bool IsDeletable()
	{
		return m_bDeletable;
	}

	bool HasError() const { return m_bHasError; }

	unsigned long GetRefCount() const { return m_nRefCount; }

	void AddRef()
	{
		InterlockedIncrement(&m_nRefCount);
	}

	unsigned long ReleaseRef()
	{
		if (m_nRefCount > 0)
			InterlockedDecrement(&m_nRefCount);

		return m_nRefCount;
	}

	// for stl::set sorting
	bool operator < (const CBitmapCacheItem &rhs) const
	{
		return m_Hash < rhs.m_Hash;
	}
};


// Comparison functor
class CCompareBitmapCacheItem
{
public:
	bool operator() (const CBitmapCacheItem *p1, const CBitmapCacheItem *p2) const
	{
		return *p1 < *p2;
	}
};


typedef set<CBitmapCacheItem *, CCompareBitmapCacheItem> TBitmapCacheSet;
typedef TBitmapCacheSet::iterator TBitmapCacheSetIter;


// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CBitmapCache
// --------------------------------------------------------------------------------------------------------------------------------------------
class CBitmapCache
{
protected:
	TBitmapCacheSet		m_setCache;
	CMutex				m_Mutex;

public:
	CBitmapCache();
	~CBitmapCache();

	// searches the cache, if found, returns the item.
	// otherwise the item is created and then returned.
	CBitmapCacheItem *GetCacheItem(const RString &strFilePath, bool use_shell_api = true);

	// used during stream-reading
	CBitmapCacheItem *GetCacheItem(const CMD5Hash &md5hash);

	// De-reference item. If refcount is zero, remove item from cache and delete it.
	void Unref(CBitmapCacheItem *cached_bitmap);

	void Read(Stream &stream);
	void Write(Stream &stream) const;
};
