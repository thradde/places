
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
#include <Shlobj.h>

#include "memdebug.h"

#include <SFML/Graphics.hpp>

#include <set>
using namespace std;

#include "zlib/zlib.h"
#include "lodepng.h"

#define ASSERT
#include "RfwString.h"
#include "exceptions.h"
#include "Mutex.h"
#include "stream.h"
#include "platform.h"
#include "generic.h"
#include "configuration.h"
#include "GetIcon.h"
#include "my_resampler.h"
#include "md5.h"
#include "bitmap_cache.h"


// --------------------------------------------------------------------------------------------------------------------------------------------
//																Globals
// --------------------------------------------------------------------------------------------------------------------------------------------
static CResampler gResampler;
static CBitmapCacheItem	*gpNotFoundItem = NULL;


// --------------------------------------------------------------------------------------------------------------------------------------------
//															ZlibCompress
// --------------------------------------------------------------------------------------------------------------------------------------------
static void *ZlibCompress(const void *source, unsigned int source_size, unsigned int &compressed_size)
{
	// temporary buffer required by zlib must be 110% of source_size + 12 bytes
	uLongf comp_size = source_size + source_size / 10 + 12;
	void *tmp_buf = malloc(comp_size);

	int z_result = compress((Bytef *)tmp_buf, &comp_size, (const Bytef *)source, source_size);
	if (z_result != Z_OK)
	{
		free(tmp_buf);
		compressed_size = 0;
		return nullptr;
	}

	compressed_size = (unsigned int)comp_size;
	return tmp_buf;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															ZlibUncompress
// --------------------------------------------------------------------------------------------------------------------------------------------
static bool ZlibUncompress(const void *source, unsigned int source_size, const void *target, unsigned int uncompressed_size)
{
	uLongf uncomp_size = (uLongf)uncompressed_size;

	int z_result = uncompress((Bytef *)target, &uncomp_size, (const Bytef *)source, source_size);
	if (z_result != Z_OK || uncomp_size != (uLongf)uncompressed_size)
		return false;

	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															ReadBinaryData
//
// Reads a block of binary data in compressed form
// --------------------------------------------------------------------------------------------------------------------------------------------
static BYTE *ReadBinaryData(Stream &stream)
{
	BYTE *data;

	// compression type (0 = no bitmap data, 1 = uncompressed, 2 = zlib)
	int comp_type = stream.ReadInt();
	if (comp_type == 0)
	{
		// no bitmap data, bail out
		return nullptr;
	}

	// uncompressed size
	unsigned int uncomp_size = stream.ReadUInt();
	unsigned int comp_size = stream.ReadUInt();

	data = new BYTE[uncomp_size];

	if (comp_type == 1)
	{
		// uncompressed data
		stream.Read(data, uncomp_size);
	}
	else
	{
		// uncompressed data
		BYTE *tmp_buf = new BYTE[comp_size];
		stream.Read(tmp_buf, comp_size);
		if (!ZlibUncompress(tmp_buf, comp_size, data, uncomp_size))
		{
			// todo: use some placeholder bitmap
			delete data;
			data = nullptr;
		}
		delete[] tmp_buf;
	}

	return data;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															WriteBinaryData
//
// Writes a block of binary data in compressed form
// --------------------------------------------------------------------------------------------------------------------------------------------
static void WriteBinaryData(Stream &stream, const void *data, unsigned int size)
{
	if (!data)
	{
		// no data
		// write compression type = 0 and bail out
		stream.WriteInt(0);
		return;
	}

	// try to compress (bitmap) data
	unsigned int comp_size;
	void *comp_buf = ZlibCompress(data, size, comp_size);

	// compression type (0 = no bitmap data, 1 = uncompressed, 2 = zlib)
	if (comp_buf)
		stream.WriteInt(2);
	else
		stream.WriteInt(1);

	// uncompressed size
	stream.WriteUInt(size);

	// write bitmap
	if (comp_buf)
	{
		// size of compressed data
		stream.WriteUInt(comp_size);

		// compressed data
		stream.Write(comp_buf, comp_size);
		free(comp_buf);
	}
	else
	{
		// uncompressed data, write size 0 for compressed data size
		stream.WriteUInt(0);

		// write uncompressed bitmap
		stream.Write(data, size);
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//																CBitmap::Read()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CBitmap::Read(Stream &stream)
{
	// size pixels x and y
	m_Size.x = stream.ReadInt();
	m_Size.y = stream.ReadInt();

	// pixel format (always 0 = RGBA)
	stream.ReadInt();

	m_pBitmap = ReadBinaryData(stream);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//																CBitmap::Write()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CBitmap::Write(Stream &stream) const
{
	// size pixels x and y
	stream.WriteInt(m_Size.x);
	stream.WriteInt(m_Size.y);

	// pixel format (0 = RGBA)
	stream.WriteInt(0);

	WriteBinaryData(stream, m_pBitmap, GetDataSize());
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CBitmapCacheItem::RetrieveShellIcon()
// uses shell api
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CBitmapCacheItem::RetrieveShellIcon(const RString &strFilePath)
{
	// retrieve different icon sizes and store the bitmaps
	bool at_least_one_created = false;
	for (int i = 0; i < nNumBitmapSizes; i++)
	{
		int pixels = arBitmapSizes[i];
		m_Bitmaps[i].SetBitmapData((BYTE *)GetIconBitmap(strFilePath.c_str(), pixels, false));
		m_Bitmaps[i].SetSize(CSize(pixels, pixels));	// value of pixels can be modified by GetIconBitmap()

		if (m_Bitmaps[i].GetBitmapData())
			at_least_one_created = true;
	}

	return at_least_one_created;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CBitmapCacheItem::LoadBitmapFile()
// loads bitmap from external bitmap graphics file
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CBitmapCacheItem::LoadBitmapFile(const RString &strFilePath)
{
	// the file path points to a real bitmap file that shall be used as an icon
	// supports bmp, png, tga, jpg, gif, psd, hdr and pic (SFML 2.3)
	sf::Image image;
	sf::String s(strFilePath.c_str());		// use helper class to convert to ANSI string, which is required by loadFromFile() (?)
	unsigned char *pixel_data;
	sf::Vector2u size;
	bool used_lodepng = false;
	if (image.loadFromFile(s))				// returns always an RGBA bitmap!
	{
		pixel_data = (unsigned char *)image.getPixelsPtr();
		size = image.getSize();
	}
	else
	{
		// image could not be loaded
		// SFML can not load 16-bit per channel png, try lodepng lib

		// convert to ansi string first
		unsigned w, h;
		std::string str;
		str = s.toAnsiString();

		unsigned ret = lodepng_decode32_file(&pixel_data, &w, &h, str.c_str());
		if (ret != 0)
			return false;	// image could not be loaded
		size.x = w;
		size.y = h;
		used_lodepng = true;
	}

	// resample, if required
	int source_pixels = std::max(size.x, size.y);

	// compare against pre-configured sizes
	int best_fit = nNumBitmapSizes - 1;
	for (int i = 0; i < nNumBitmapSizes; i++)
	{
		if (source_pixels == arBitmapSizes[i])
		{
			best_fit = i;
			break;
		}
	}

	int target_size = arBitmapSizes[best_fit];
	unsigned char *resampled = gResampler.Resample(pixel_data, size.x, size.y, 4, target_size, target_size);
	
	if (used_lodepng)
		free(pixel_data);

	if (!resampled)
		return false;

	m_Bitmaps[best_fit].SetBitmapData(resampled);
	m_Bitmaps[best_fit].SetSize(CSize(target_size, target_size));

	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//														CBitmapCacheItem::CBitmapCacheItem()
// --------------------------------------------------------------------------------------------------------------------------------------------
CBitmapCacheItem::CBitmapCacheItem(const RString &strFilePath, bool use_shell_api)
	:	m_nRefCount(0),
		m_bHasError(false),
		m_bDeletable(true)
{
	bool success;

	if (strFilePath.empty())
	{
		m_bHasError = true;
		return;
	}

	if (use_shell_api)
		success = RetrieveShellIcon(strFilePath);
	else
		success = LoadBitmapFile(strFilePath);

	if (!success)
	{
		// no image could not be loaded
		m_bHasError = true;
		return;
	}

	// create MD5 hash for biggest available bitmap
	CMD5 md5;
	for (int i = nNumBitmapSizes - 1; i >= 0; i--)
	{
		if (m_Bitmaps[i].GetBitmapData())
		{
			md5.OnePass(m_Hash, m_Bitmaps[i].GetBitmapData(), m_Bitmaps[i].GetDataSize());
			break;
		}
	}

#if 0	// for debugging
		// create file from bitmap - use biggest bitmap
		for (int i = nNumBitmapSizes - 1; i >= 0; i--)
		{
			if (m_Bitmaps[i].GetBitmapData())
			{
				sf::Texture texture;
				texture.create(m_Bitmaps[i].m_Size.x, m_Bitmaps[i].m_Size.y);
				texture.update((sf::Uint8 *)m_Bitmaps[i].GetBitmapData());
				sf::Image image = texture.copyToImage();
				image.saveToFile("e:\\places debug img.png");
				break;
			}
		}
#endif
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//													CBitmapCacheItem::CBitmapCacheItem(Stream)
// --------------------------------------------------------------------------------------------------------------------------------------------
CBitmapCacheItem::CBitmapCacheItem(Stream &stream)
	:	m_nRefCount(0),
		m_bHasError(false),
		m_bDeletable(true)
{
	// md5 hash
	m_Hash.Read(stream);

	// number of elements in m_Bitmaps[]
	if (stream.ReadInt() != nNumBitmapSizes)	// must be nNumBitmapSizes
		throw Exception(Exception::EXCEPTION_DB_CORRUPT);

	// each element of m_Bitmaps[], zlib compressed
	for (int i = 0; i < nNumBitmapSizes; i++)
		m_Bitmaps[i].Read(stream);

#if 1
	// scaled bitmap for the texture
	m_DrawBitmap.Read(stream);

	// move draw bitmap into texture
	m_DrawTexture.create(m_DrawBitmap.GetSize().x, m_DrawBitmap.GetSize().y);		// todo: if new resolution is requested, does second create on same object work?
	m_DrawTexture.update((sf::Uint8 *)m_DrawBitmap.GetBitmapData());
	m_DrawTexture.setSmooth(true);
#endif
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CBitmapCacheItem::Write()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CBitmapCacheItem::Write(Stream &stream) const
{
	// md5 hash
	m_Hash.Write(stream);

	// number of elements in m_Bitmaps[]
	stream.WriteInt(nNumBitmapSizes);

	// each element of m_Bitmaps[], zlib compressed
	for (int i = 0; i < nNumBitmapSizes; i++)
		m_Bitmaps[i].Write(stream);

	// scaled bitmap for the texture
	m_DrawBitmap.Write(stream);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//													CBitmapCacheItem::CreateScaledDrawTexture()
//
// Creates a texture for GUI drawing which will be exactly scaled to the desired pixels.
// Chooses the best fitting bitmap as source for resampling.
// --------------------------------------------------------------------------------------------------------------------------------------------
void CBitmapCacheItem::CreateScaledDrawTexture(int pixels)
{
	CMutexLock lock(m_Mutex);

	if (m_DrawTexture.getSize().x == pixels)
		return;

	m_DrawTexture.create(pixels, pixels);		// todo: if new resolution is requested, does second create on same object work?

	// choose the best fitting source texture
	int best_fit;
	for (best_fit = 0; best_fit < nNumBitmapSizes; best_fit++)
	{
		if (m_Bitmaps[best_fit].GetSize().x >= pixels)
			break;
	}

	if (best_fit >= nNumBitmapSizes)
		best_fit = nNumBitmapSizes - 1;

	if (!m_Bitmaps[best_fit].GetBitmapData())
	{
		// best fit not available, search upwards for next bigger bitmap
		int closest_best_fit = best_fit + 1;
		while (closest_best_fit < nNumBitmapSizes)
		{
			if (m_Bitmaps[closest_best_fit].GetBitmapData())
				break;
			closest_best_fit++;
		}

		if (closest_best_fit < nNumBitmapSizes && m_Bitmaps[closest_best_fit].GetBitmapData())
			best_fit = closest_best_fit;
	}

	if (!m_Bitmaps[best_fit].GetBitmapData())
	{
		// best fit upwards not available, search downwards for next smaller bitmap
		int closest_best_fit = best_fit - 1;
		while (closest_best_fit >= 0)
		{
			if (m_Bitmaps[closest_best_fit].GetBitmapData())
				break;
			closest_best_fit--;
		}

		if (closest_best_fit >= 0 && m_Bitmaps[closest_best_fit].GetBitmapData())
			best_fit = closest_best_fit;
	}

	if (!m_Bitmaps[best_fit].GetBitmapData())
	{
		// todo: mark icon as undrawable ==> use a default bitmap!
		// this should never happen, because the image loader will then return an error and this code never gets
		// executed, as the "image not found" cache item is always used in that case
		return;
	}

	// setup sprite
	int i = best_fit;
	int tex_pixels = m_Bitmaps[i].GetSize().x;
	float scale = (float)pixels / (float)tex_pixels;
	unsigned char *resampled = gResampler.Resample(m_Bitmaps[i].GetBitmapData(), tex_pixels, tex_pixels, 4, pixels, pixels);
	if (!resampled)
	{
		// todo: mark icon as undrawable ==> use a default bitmap!
		// this can happen! at least theoretically, but then we are out of memory
		return;
	}

	m_DrawBitmap.SetSize(CSize(pixels, pixels));
	m_DrawBitmap.SetBitmapData(resampled);

	m_DrawTexture.update((sf::Uint8 *)resampled);
	m_DrawTexture.setSmooth(true);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CBitmapCache::CBitmapCache()
// --------------------------------------------------------------------------------------------------------------------------------------------
CBitmapCache::CBitmapCache()
{
	gpNotFoundItem = GetCacheItem(gstrCommonAppData + _T("icons\\img-not-found.png"), false);
	if (gpNotFoundItem)
		gpNotFoundItem->SetDeletable(false);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CBitmapCache::~CBitmapCache()
// --------------------------------------------------------------------------------------------------------------------------------------------
CBitmapCache::~CBitmapCache()
{
	for (auto it : m_setCache)
		delete it;				// this also deletes the gpNotFoundItem
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CBitmapCache::GetCacheItem()
//
// searches the cache, if found, returns the item.
// otherwise the item is created and then returned.
// if it can not be found nor created, the image-not-found icon is returned.
// --------------------------------------------------------------------------------------------------------------------------------------------
CBitmapCacheItem *CBitmapCache::GetCacheItem(const RString &strFilePath, bool use_shell_api)
{
	CMutexLock lock(m_Mutex);

	CBitmapCacheItem *item = new CBitmapCacheItem(strFilePath, use_shell_api);

	if (!item->HasError())
	{
		// search cache
		TBitmapCacheSetIter it = m_setCache.find(item);
		if (it != m_setCache.end())
		{
			delete item;
			(*it)->AddRef();
			return *it;
		}

		// not found, add to cache
		item->AddRef();
		m_setCache.insert(item);
	}
	else
	{
		delete item;
		item = gpNotFoundItem;
		if (item)
			item->AddRef();
	}

	return item;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CBitmapCache::GetCacheItem(by hash)
// used during stream-reading
// --------------------------------------------------------------------------------------------------------------------------------------------
CBitmapCacheItem *CBitmapCache::GetCacheItem(const CMD5Hash &md5hash)
{
	CMutexLock lock(m_Mutex);

	CBitmapCacheItem search_key(md5hash);

	TBitmapCacheSetIter it = m_setCache.find(&search_key);
	if (it != m_setCache.end())
	{
		(*it)->AddRef();
		return *it;
	}

	CBitmapCacheItem *item = gpNotFoundItem;
	if (item)
		item->AddRef();

	return item;
}



// --------------------------------------------------------------------------------------------------------------------------------------------
//															CBitmapCache::Unref()
//
// De-reference item. If refcount is zero, remove item from cache and delete it.
// --------------------------------------------------------------------------------------------------------------------------------------------
void CBitmapCache::Unref(CBitmapCacheItem *cached_bitmap)
{
	CMutexLock lock(m_Mutex);

	if (cached_bitmap)
	{
		// if refcount is zero, remove item from cache and delete it
		if (cached_bitmap->ReleaseRef() == 0 && cached_bitmap->IsDeletable())
		{
			// search item in cache
			TBitmapCacheSetIter it = m_setCache.find(cached_bitmap);
			if (it != m_setCache.end())
			{
				// remove from cache
				m_setCache.erase(it);
			}

			// delete the item itself
			delete cached_bitmap;
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CBitmapCache::Read()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CBitmapCache::Read(Stream &stream)
{
	// number of items
	unsigned int nCacheItems = stream.ReadUInt();

	for (unsigned int i = 0; i < nCacheItems; i++)
	{
		CBitmapCacheItem *item = new CBitmapCacheItem(stream);
		m_setCache.insert(item);
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CBitmapCache::Write()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CBitmapCache::Write(Stream &stream) const
{
	// compute number of items - unreferenced items and the gpNotFoundItem are not counted
	unsigned int nCacheItems = 0;
	for (auto it : m_setCache)
	{
		if (it->GetRefCount() > 0 && it->IsDeletable())
			nCacheItems++;
	}

	// write number of items
	stream.WriteUInt(nCacheItems);

	// write items
	for (auto it : m_setCache)
	{
		if (it->GetRefCount() > 0 && it->IsDeletable())
			it->Write(stream);
	}
}
