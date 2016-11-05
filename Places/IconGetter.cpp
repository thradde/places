
#include <Windows.h>
#include <Windowsx.h>

#include <tchar.h>
#include <shlobj.h>
#include <shlguid.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commoncontrols.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "GetIcon.h"


void MyAlphaBlend(DIBitmap source, const SIZE &size)
{
	BYTE *src = (BYTE *)source;
	BYTE bkg_color[] = { 0xff, 0xff, 0xff };
	int v;

	for (int x = 0; x < size.cx; x++)
	{
		for (int y = 0; y < size.cy; y++)
		{
			int alpha = src[3];
			v = MulDiv(src[0], alpha, 255) + MulDiv(bkg_color[0], (255 - alpha), 255);
			v += 0x18;
			if (v > 255)
				v = 255;
			src[0] = v;

			v = MulDiv(src[1], alpha, 255) + MulDiv(bkg_color[1], (255 - alpha), 255);
			v += 0x18;
			if (v > 255)
				v = 255;
			src[1] = v;

			v = MulDiv(src[2], alpha, 255) + MulDiv(bkg_color[2], (255 - alpha), 255);
			v += 0x18;
			if (v > 255)
				v = 255;
			src[2] = v;

			src += 4;
		}
	}
}


#if 1
DIBitmap BlurEdges(DIBitmap source, int size_x, int size_y)
{
#define GetPixel(x, y)		source[y * size_x + x]
#define SetPixel(x, y, v)	target[y * size_x + x] = v;

	const uint32 alpha = 0xff000000;
	size_t num_pixels = size_x * size_y;
	uint32 *target = new uint32[num_pixels];

	uint32 gauss_filter[] =
	{ 
		1, 2, 1, 
		2, 2, 2, 
		1, 2, 1 
	};

	for (int x = 0; x < size_x; x++)
	{
		for (int y = 0; y < size_y; y++)
		{
			// we blur selectively, i.e. on edges, where the alpha of the current pixel is non-fully-transparent,
			// and of a neighboring pixel is zero (i.e. fully transparent)
			uint32 pixel = GetPixel(x, y);
			SetPixel(x, y, pixel);
			if ((pixel & alpha) != 0)
			{
				bool has_fully_transparent_neighbour = false;
				for (int xx = x - 1; xx <= x + 1; xx++)
				{
					if (xx < 0)
						xx = 0;
					else if (xx >= size_x)
						break;
					for (int yy = y - 1; yy <= y + 1; yy++)
					{
						if (yy < 0)
							yy = 0;
						else if (yy >= size_y)
							break;
						// test neighboring pixel
						uint32 np = GetPixel(xx, yy);
						if ((np & alpha) == 0)
						{
							has_fully_transparent_neighbour = true;
							goto quit_loop;
						}
					}
				}

quit_loop:
				if (has_fully_transparent_neighbour)
				{
					int xx = x;
					//for (int xx = x - 1; xx < x + 1; xx++)
					{
						if (xx < 0)
							xx = 0;
						else if (xx >= size_x)
							break;
						int yy = y;
						//for (int yy = y - 1; yy < y + 1; yy++)
						{
							if (yy < 0)
								yy = 0;
							else if (yy >= size_y)
								break;

							uint32 np = GetPixel(xx, yy);
							// yes, it is fully transparent
							// read for this pixel all neighboring pixels into a matrix
							uint32 matrix[9];
							for (int i = 0; i < 9; i++)
									matrix[i] = 0; // 0xffffff00;

							if (yy > 0)
							{
								if (xx > 0)
									matrix[0] = source[(yy - 1) * size_x + xx - 1];
								matrix[1] = source[(yy - 1) * size_x + xx];
								if (xx + 1 < size_x)
									matrix[2] = source[(yy - 1) * size_x + xx + 1];
							}

							if (xx > 0)
								matrix[3] = source[yy * size_x + xx - 1];
							matrix[4] = source[yy * size_x + xx];
							if (xx + 1 < size_x)
								matrix[5] = source[yy * size_x + xx + 1];

							if (yy + 1 < size_y)
							{
								if (xx > 0)
									matrix[6] = source[(yy + 1) * size_x + xx - 1];
								matrix[7] = source[(yy + 1) * size_x + xx];
								if (xx + 1 < size_x)
									matrix[8] = source[(yy + 1) * size_x + xx + 1];
							}

							matrix[4] |= 0x00ffffff;
#define COPY_ALPHA_CHANNEL
							// schwarze Nachbarpixel ziehen das Smoothing in dunkle Bereiche, wir wollen
							// aber das Gegenteil, daher werden diese in weisse Pixel umgewandelt
							for (int i = 0; i < 9; i++)
								if ((matrix[i] & 0x00ffffff) == 0)
#ifdef COPY_ALPHA_CHANNEL
									matrix[i] |= 0x00ffffff;
#else
									matrix[i] |= 0xffffffff;
#endif

							int component_value;

#ifdef COPY_ALPHA_CHANNEL
							uint32 new_pixel = np & 0xff000000;		// copy alpha channel
							for (int component = 0; component < 24; component += 8)
#else
							uint32 new_pixel = 0;
							for (int component = 0; component < 32; component += 8)
#endif
							{
								float avg = 0;
								float divider = 0;
								for (int i = 0; i < 9; i++)
								{
									//if ((matrix[i] & 0x00ffffff) != 0)	// adaptives filtern, d.h. nur Pixel berücksichtigen, die nicht schwarz sind
									{
										component_value = (matrix[i] >> component) & 0xff;
										avg += component_value * gauss_filter[i];
										divider += gauss_filter[i];
									}
								}

								avg /= divider;
								if (component < 24)	// wenn nicht alpha channel, dann
									avg += 0x10;	// aufhellen
								else
									avg -= 0x10;	// alpha channel transparenter machen
#if 1
								if (avg < 0.f)
									avg = 0.f;
								else if (avg > 255.f)
									avg = 255.f;
								new_pixel |= ((int)avg << component);
#else
								avg /= 255.f;
								component_value = (np >> component) & 0xff;
								/*
								if (component_value == 0)
									component_value = (int)(64.f * avg);
								else
								*/
									component_value = (int)((float)component_value * avg);
								new_pixel |= (component_value << component);
#endif
							}

							SetPixel(xx, yy, new_pixel);
						}
					}
				}
			}
		}
	}

	return target;
}
#endif


// Selective Gaussian Blur - taken from gimp/plug-ins/blur-gauss-selective.c
static void
	init_matrix (
	double  radius,
	double *mat,
	int     num)
{
	int    dx;
	double sd, c1, c2;

	/* This formula isn't really correct, but it'll do */
	sd = radius / 3.329042969;
	c1 = 1.0 / sqrt (2.0 * M_PI * sd);
	c2 = -2.0 * (sd * sd);

	for (dx = 0; dx < num; dx++)
		mat[dx] = c1 * exp ((dx * dx)/ c2);
}


static void
	matrixmult_int(
	BYTE         *src,
	BYTE         *dest,
	int           width,
	int           height,
	const double *mat,
	int           numrad,
	int           bytes_per_pixel,
	bool          has_alpha,
	int           maxdelta)
{
	const int  nb        = bytes_per_pixel - (has_alpha ? 1 : 0);
	const int  rowstride = width * bytes_per_pixel;
	unsigned short *imat;
	double     fsum, fscale;
	int        i, j, b, x, y, d;

	imat = new unsigned short[2 * numrad];

	fsum = 0.0;
	for (y = 1 - numrad; y < numrad; y++)
		fsum += mat[abs(y)];

	/* Ensure that the sum fits in 32bits. */
	fscale = 0x1000 / fsum;
	for (y = 0; y < numrad; y++)
		imat[numrad - y] = imat[numrad + y] = (unsigned short)(mat[y] * fscale);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int dix = bytes_per_pixel * (width * y + x);

			if (has_alpha)	// copy over transparency value
				dest[dix + nb] = src[dix + nb];

			for (b = 0; b < nb; b++)
			{
				const BYTE *src_db = src + dix + b;
				uint32      sum    = 0;
				uint32      fact   = 0;
				int         offset;

				offset = rowstride * (y - numrad) + bytes_per_pixel * (x - numrad);

				for (j = 1 - numrad; j < numrad; j++)
				{
					const BYTE *src_b;
					uint32      rowsum  = 0;
					uint32      rowfact = 0;

					offset += rowstride;
					if (y + j < 0 || y + j >= height)
						continue;

					src_b = src + offset + b;

					for (i = 1 - numrad; i < numrad; i++)
					{
						int tmp;

						src_b += bytes_per_pixel;

						if (x + i < 0 || x + i >= width)
							continue;

						tmp = *src_db - *src_b;
						if (tmp > maxdelta || tmp < -maxdelta)
							continue;

						d = imat[numrad+i];
						if (has_alpha)
							d *= src_b[nb - b];

						rowsum += d * *src_b;
						rowfact += d;
					}

					d = imat[numrad+j];

					if (has_alpha)
					{
						rowsum >>= 8;
						rowfact >>= 8;
					}

					sum += d * rowsum;
					fact += d * rowfact;
				}

				if (fact == 0)
					dest[dix + b] = *src_db;
				else
					dest[dix + b] = sum / fact;
			}
		}
	}

	delete[] imat;
}


static DIBitmap
sel_gauss (DIBitmap  drawable,
           int       width,			// width of bitmap
		   int       height,		// height of bitmap
           double    radius,		// radius of filter, 5.0 is a good value
           int       maxdelta,		// max range of filter, 80 is a good value
		   int       bytes_per_pixel = 4,
		   bool      has_alpha = true)
{
  BYTE      *dest;
  BYTE      *src;
  double    *mat;
  int        numrad;

  numrad = (int) (radius + 1.0);
  mat = new double[numrad];
  init_matrix (radius, mat, numrad);

  src = (BYTE *)drawable;
  dest = new BYTE[width * height * bytes_per_pixel];

  matrixmult_int (src, dest, width, height, mat, numrad,
              bytes_per_pixel, has_alpha, maxdelta);

  /* free up buffers */
  delete[] mat;

  return (DIBitmap)dest;
}


struct MyIconInfo
{
	int     nWidth;
	int     nHeight;
	int     nBitsPerPixel;
};


bool MyGetIconInfo(HICON hIcon, MyIconInfo &info)
{
	ICONINFO icon_info;
	BITMAP bmp;

	BOOL bRes = GetIconInfo(hIcon, &icon_info);
	if (!bRes)
		return false;

	if (icon_info.hbmColor)
	{
		const int nBytes = GetObject(icon_info.hbmColor, sizeof(bmp), &bmp);
		if (nBytes > 0)
		{
			info.nWidth = bmp.bmWidth;
			info.nHeight = bmp.bmHeight;
			info.nBitsPerPixel = bmp.bmBitsPixel;
		}
	}
	else if (icon_info.hbmMask)
	{
		// Icon has no color plane, image data stored in mask
		const int nBytes = GetObject(icon_info.hbmMask, sizeof(bmp), &bmp);
		if (nBytes > 0)
		{
			info.nWidth = bmp.bmWidth;
			info.nHeight = bmp.bmHeight / 2;	// monochrome bitmap has twice the hight, one mask with the
			info.nBitsPerPixel = 1;				// icon bits and the other with XOR mask in hbmMask
		}
	}

	if (icon_info.hbmColor)
		DeleteObject(icon_info.hbmColor);
	if (icon_info.hbmMask)
		DeleteObject(icon_info.hbmMask);

	return true;
}


HICON GetIcon(LPCTSTR filePath)
{
#if 1
	// Get the icon index using SHGetFileInfo
	SHFILEINFO sfi = { 0 };
	SHGetFileInfo(filePath, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX);

	// Retrieve the system image list.
	// To get the 48x48 icons, use SHIL_EXTRALARGE
	// To get the 256x256 icons (Vista only), use SHIL_JUMBO
	HIMAGELIST *imageList;
	HRESULT hResult = SHGetImageList(SHIL_JUMBO, IID_IImageList, (void**)&imageList);

	if (hResult == S_OK)
	{
		// Get the icon we need from the list. Note that the HIMAGELIST we retrieved
		// earlier needs to be casted to the IImageList interface before use.
		HICON hIcon;
		hResult = ((IImageList*)imageList)->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hIcon); // ILD_PRESERVEALPHA | ILD_SCALE | ILD_TRANSPARENT ==> ILD_SCALE seems to cause trouble sometimes (reported on internet)

		if (hResult == S_OK)
			return hIcon;
	}
#else
	// Get the icon index using SHGetFileInfo
	SHFILEINFO sfi = { 0 };
	if (SHGetFileInfo(filePath, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON))
	{
		return sfi.hIcon;
	}
#endif

	return nullptr;
}


void InitializeBitmapHeader(BITMAPV5HEADER* header, int width, int height)
{
	memset(header, 0, sizeof(BITMAPV5HEADER));
	header->bV5Size = sizeof(BITMAPV5HEADER);

	// Note that icons are created using top-down DIBs so we must negate the
	// value used for the icon's height.
	header->bV5Width = width;
	header->bV5Height = -height;
	header->bV5Planes = 1;
	header->bV5Compression = BI_RGB;

	// Initializing the bitmap format to 32 bit ARGB.
	header->bV5BitCount = 32;
	header->bV5RedMask = 0x00FF0000;
	header->bV5GreenMask = 0x0000FF00;
	header->bV5BlueMask = 0x000000FF;
	header->bV5AlphaMask = 0xFF000000;

	// Use the system color space.  The default value is LCS_CALIBRATED_RGB, which
	// causes us to crash if we don't specify the appropriate gammas, etc.  See
	// <http://msdn.microsoft.com/en-us/library/ms536531(VS.85).aspx> and
	// <http://b/1283121>.
	header->bV5CSType = LCS_WINDOWS_COLOR_SPACE;

	// Use a valid value for bV5Intent as 0 is not a valid one.
	// <http://msdn.microsoft.com/en-us/library/dd183381(VS.85).aspx>
	header->bV5Intent = LCS_GM_IMAGES;
}


DIBitmap BGRA2RGBA(DIBitmap org_bitmap, const SIZE &size)
{
	// Copy the image over. It is in BGRA format, change it to RGBA
	size_t num_pixels = size.cx * size.cy;
	uint32 *bitmap = new uint32[num_pixels];
	register BYTE *source = (BYTE *)org_bitmap;
	register BYTE *target = (BYTE *)bitmap;
	for (register size_t i = 0; i < num_pixels; i++)
	{
		*(target+2) = *source++;
		*(target+1) = *source++;
		*target = *source++;
		*(target+3) = *source++;
		target += 4;
	}

	return bitmap;
}


DIBitmap MakeBmpTopDown(DIBitmap org_bitmap, const SIZE &size)
{
	size_t num_pixels = size.cx * size.cy;
	uint32 *bitmap = new uint32[num_pixels];
	for (int y = 0; y < size.cy; y++)
	{
		memcpy(	bitmap + y * size.cx,
				org_bitmap + (size.cy - 1 - y) * size.cx,
				size.cx * 4);
	}

	return bitmap;
}



bool IsScanlineTransparent(DIBitmap bmp, int y, const SIZE &size, int nFrameWidth)
{
	BYTE *scanline = (BYTE *)bmp + (y * size.cx * 4) + 3;		// alpha channel
	scanline += nFrameWidth * 4;
	int xend = size.cx - nFrameWidth;
	for (int x = nFrameWidth; x < xend; x++)
	{
		if (*scanline != 0)		// alpha channel not transparent
			return false;
		scanline += 4;
	}

	return true;
}


bool IsColumnTransparent(DIBitmap bmp, int x, const SIZE &size, int nFrameWidth)
{
	BYTE *scanline = (BYTE *)bmp + x * 4 + 3;		// alpha channel
	scanline += nFrameWidth * size.cx * 4;
	size_t offset = size.cx * 4;
	int yend = size.cy - nFrameWidth;
	for (int y = nFrameWidth; y < yend; y++)
	{
		if (*scanline != 0)		// alpha channel not transparent
			return false;
		scanline += offset;
	}

	return true;
}


// Often icons have not the 256 x 256 resolution.
// In this case Windows returns a 256x256 bitmap, but the original smaller icon is centered
// in the middle and it has a 3 pixel wide frame around it.
// Test, if the bitmap has been resized by Windows.
bool IsResized(DIBitmap org_bitmap, const SIZE &size)
{
	int nFrameWidth = 5;		// width of frame created by Windows (5 for 256x256 icons and 3 for 128x128 icons)

	if (size.cx <= 128)
		nFrameWidth = 3;
	else if (size.cx <= 64)
		nFrameWidth = 1;		// just guessed for safety

	// check, if smaller centered bitmap
	int x, y;
	int height = size.cy / 2 - nFrameWidth;	// skip 3 pixel wide frame created by Windows
	for (y = nFrameWidth; y < height; y++)
	{
		if (!IsScanlineTransparent(org_bitmap, y, size, nFrameWidth) || 
			!IsScanlineTransparent(org_bitmap, size.cy - nFrameWidth - 1 - y, size, nFrameWidth))
		{
			break;
		}
	}

	int width = size.cx / 2 - nFrameWidth;	// skip first and last column, because of frame created by Windows
	for (x = nFrameWidth; x < width; x++)
	{
		if (!IsScanlineTransparent(org_bitmap, x, size, nFrameWidth) || 
			!IsScanlineTransparent(org_bitmap, size.cx - nFrameWidth - 1 - x, size, nFrameWidth))
		{
			break;
		}
	}

	width = (width - x) * 2;
	height = (height - y) * 2;

	if (size.cx == 256 && width <= 128 && height <= 128)
		return true;

	if (size.cx == 128 && width <= 64 && height <= 64)
		return true;

	if (size.cx == 64 && width <= 32 && height <= 32)
		return true;

	if (size.cx == 48 && width <= 32 && height <= 32)
		return true;

	if (size.cx == 32 && width <= 16 && height <= 16)
		return true;

	return false;
}


// Returns a DIBitmap. The DIBitmap is allocated using new[] and must be deleted[] by the caller!
// Parameter filePath may also be "*.doc" to return a valid bitmap.
// If thumbnail is true, for images and other files a thumbnail is returned.
DIBitmap GetIconBitmap(LPCTSTR filePath, int &pixels, bool thumbnail)
{
#if 0
	// for testing / debugging
	if (pixels == 48)
	{
		size_t num_pixels = pixels * pixels;
		uint32 *bitmap = new uint32[num_pixels];
		memset(bitmap, 0xaa, num_pixels * 4);
		return bitmap;
	}
	return NULL;
#else
	// Getting the IShellItemImageFactory interface pointer for the file.
	IShellItemImageFactory *pImageFactory;
	HRESULT hr = SHCreateItemFromParsingName(filePath, NULL, IID_PPV_ARGS(&pImageFactory));
	if (FAILED(hr))
		return nullptr;

	// for size 256 x 256 , some programs (like irfan view) return only a 128 x 128 bitmap.
	// when the main program scales this down to 128 x 128, we get a small 64 x 64 version, this is bad.
	const int WantedSize = pixels;
	const SIZE size = { WantedSize, WantedSize };
	int width = WantedSize;
	int height = WantedSize;

	HBITMAP hBmp;
	SIIGBF flags = 0; //SIIGBF_RESIZETOFIT; // | SIIGBF_BIGGERSIZEOK	// SIIGBF_BIGGERSIZEOK - GetImage will stretch down the bitmap (preserving aspect ratio)
	if (!thumbnail)
		flags |= SIIGBF_ICONONLY;
	hr = pImageFactory->GetImage(size, flags, &hBmp);
	pImageFactory->Release();
	if (FAILED(hr))
		return nullptr;

	BITMAP bmp;
	if (GetObject(hBmp, sizeof(bmp), &bmp) > 0)
	{
		// test, if Windows did scale the image
		if (IsResized((DIBitmap)bmp.bmBits, size))
		{
			DeleteObject(hBmp);
			return nullptr;			// desired size not available
		}

		DIBitmap bitmap = BGRA2RGBA((DIBitmap)bmp.bmBits, size);
		DeleteObject(hBmp);
		
		DIBitmap top_down = MakeBmpTopDown(bitmap, size);
		delete[] bitmap;

//		MyAlphaBlend(top_down, new_size);
		return top_down;

		// blur
//		DIBitmap blurred = sel_gauss(top_down, width, height, 10, 80);
		DIBitmap blurred = BlurEdges(top_down, width, height);
		delete[] top_down;
#if 1
		return blurred;
#else
		DIBitmap b2 = BlurEdges(blurred, width, height);
		delete[] blurred;
		return b2;
#endif
	}

	DeleteObject(hBmp);
	return nullptr;

	/*
	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO)); 
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage     = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed       = 0;
	bmi.bmiHeader.biClrImportant  = 0;

	size_t num_pixels = size.cx * size.cy;
	uint32 *bits = new uint32[num_pixels];
	HDC hdc = ::GetDC(NULL);
	HDC sourceHdc = ::CreateCompatibleDC(hdc);
	::SelectObject(sourceHdc, hBmp);
	int ret = GetDIBits(
		sourceHdc,
		hBmp,
		0,
		256,
		(void **)&bits,
		(LPBITMAPINFO)&bmi,
		DIB_RGB_COLORS);
	*/

	DeleteObject(hBmp);
//	DeleteDC(sourceHdc);
//	::ReleaseDC(NULL, hdc);

#if 0
	DIBitmap bitmap = nullptr;
	if (ret)
		bitmap = BGRA2RGBA(bits, size);

	delete[] bits;
	return bitmap;
#endif
#endif
}


bool PixelsHaveAlpha(const uint32* pixels, size_t num_pixels)
{
	for(const uint32* end=pixels+num_pixels; pixels!=end; ++pixels)
	{
		if ((*pixels & 0xff000000) != 0)
		{
			return true;
		}
	}

	return false;
}


// Returns a DIBitmap. The DIBitmap is allocated using new[] and must be deleted[] by the caller!
DIBitmap CreateDIBitmapFromHICON(HICON icon, int &size_x, int &size_y)
{
	// We start with validating parameters.
	ICONINFO icon_info;
	if (!icon || !(::GetIconInfo(icon, &icon_info)) ||
		!icon_info.fIcon)
	{
		return nullptr;
	}

	// Now we should create a DIB so that we can use ::DrawIconEx in order to
	// obtain the icon's image.
	BITMAPV5HEADER h;
	InitializeBitmapHeader(&h, size_x, size_y);
	HDC dc = ::GetDC(NULL);
	uint32 *bits;
	HBITMAP dib = ::CreateDIBSection(dc, reinterpret_cast<BITMAPINFO*>(&h),
		DIB_RGB_COLORS, reinterpret_cast<void**>(&bits), NULL, 0);
	HDC dib_dc = CreateCompatibleDC(dc);
	::SelectObject(dib_dc, dib);

	// Windows icons are defined using two different masks. The XOR mask, which
	// represents the icon image and an AND mask which is a monochrome bitmap
	// which indicates the transparency of each pixel.
	//
	// To make things more complex, the icon image itself can be an ARGB bitmap
	// and therefore contain an alpha channel which specifies the transparency
	// for each pixel. Unfortunately, there is no easy way to determine whether
	// or not a bitmap has an alpha channel and therefore constructing the bitmap
	// for the icon is nothing but straightforward.
	//
	// The idea is to read the AND mask but use it only if we know for sure that
	// the icon image does not have an alpha channel. The only way to tell if the
	// bitmap has an alpha channel is by looking through the pixels and checking
	// whether there are non-zero alpha bytes.
	//
	// We start by drawing the AND mask into our DIB.
	size_t num_pixels = size_x * size_y;
	memset(bits, 0, num_pixels * 4);
	BOOL ret = ::DrawIconEx(dib_dc, 0, 0, icon, size_x, size_y, 0, NULL, DI_MASK);

	// Capture boolean opacity. We may not use it if we find out the bitmap has
	// an alpha channel.
	bool *opaque = new bool[num_pixels];
	for (size_t i = 0; i < num_pixels; i++)
		opaque[i] = !bits[i];

	// Then draw the image itself which is really the XOR mask.
	memset(bits, 0, num_pixels * 4);
	ret = ::DrawIconEx(dib_dc, 0, 0, icon, size_x, size_y, 0, NULL, DI_NORMAL);

	// Finding out whether the bitmap has an alpha channel.
	bool bitmap_has_alpha_channel = PixelsHaveAlpha(bits, num_pixels);

	// If the bitmap does not have an alpha channel, we need to build it using
	// the previously captured AND mask. Otherwise, we are done.
	if (!bitmap_has_alpha_channel)
	{
		uint32* p = bits;
		for(size_t i=0; i<num_pixels; ++p,++i)
		{
			if(opaque[i])
				*p |= 0xff000000;
			else
				*p &= 0x00ffffff;
		}
	}

	// Copy the image over. It is in BGRA format, change it to RGBA
	uint32* bitmap = new uint32[num_pixels];
	register BYTE *source = (BYTE *)bits;
	register BYTE *target = (BYTE *)bitmap;
	for (register size_t i = 0; i < num_pixels; i++)
	{
		*(target+2) = *source++;
		*(target+1) = *source++;
		*target = *source++;
		*(target+3) = *source++;
		target += 4;
	}

	delete[] opaque;
	::DeleteDC(dib_dc);
	::DeleteObject(dib);
	::ReleaseDC(NULL, dc);

	// Stupid IImageList::GetIcon() might return jumbo icon (256 x 256) but draws
	// small 32 x 32  or 48 x 48 icon into upper left corner, leaving the rest of the image blank.
	// Find out, if this is a small icon by scanning through the pixels at both sides:
	// First, test from right to left all columns:
	int x, y;
	for (x = 255; x > 0; x--)
	{
		for (int yy = 0; yy < 256; yy++)
			if (bitmap[yy * 256 + x])
				goto found_x;
	}

found_x:
	// Second, test from bottom to top all rows:
	for (y = 255; y > 0; y--)
	{
		for (int xx = 0; xx < 256; xx++)
			if (bitmap[y * 256 + xx])
				goto found_y;
	}

found_y:
	if (x <= 48 && y <= 48)
	{
		// this is a small icon, so make a small image
		// make both dimensions a multiple of 8 of the bigger dimension
		int dim = max(x, y);
		int d8 = dim / 8;
		if (dim % 8)
			d8++;
		dim = d8 * 8;

		size_t small_num_pixels = dim * dim;
		uint32 *small_bitmap = new uint32[small_num_pixels];
		uint32 *p = small_bitmap;
		for (y = 0; y < dim; y++)
		{
			for (x = 0; x < dim; x++)
				*p++ = bitmap[y * 256 + x];
		}

		delete[] bitmap;
		size_x = dim;
		size_y = dim;
		return small_bitmap;
	}

	return bitmap;
}


// Returns a DIBitmap. The DIBitmap is allocated using new[] and must be deleted[] by the caller!
DIBitmap CreateDIBitmapFromHICON2(HICON hIcon, int &size_x, int &size_y)
{
	// We start with validating parameters.
	ICONINFO icon_info;
	if (!hIcon || !(::GetIconInfo(hIcon, &icon_info)) ||
		!icon_info.fIcon)
	{
		return nullptr;
	}

	if (icon_info.hbmColor)
	{
		BITMAP bmp;
		if (GetObject(icon_info.hbmColor, sizeof(bmp), &bmp) > 0)
		{
			SIZE size;
			size.cx = size_x;
			size.cy = size_y;
			DIBitmap bitmap = BGRA2RGBA((DIBitmap)bmp.bmBits, size);	// ==> Crashes, bmp.bmBits is nullptr
			DeleteObject(icon_info.hbmColor);
			if (icon_info.hbmMask)
				DeleteObject(icon_info.hbmMask);
			DIBitmap top_down = MakeBmpTopDown(bitmap, size);
			delete[] bitmap;
			return top_down;

			// blur
	//		DIBitmap blurred = sel_gauss(top_down, width, height, 10, 80);
			DIBitmap blurred = BlurEdges(top_down, size_x, size_y);
			delete[] top_down;
			return blurred;
			/*
			DIBitmap b2 = BlurEdges(blurred, width, height);
			delete[] blurred;
			return b2;
			*/
		}
	}

	if (icon_info.hbmColor)
		DeleteObject(icon_info.hbmColor);
	if (icon_info.hbmMask)
		DeleteObject(icon_info.hbmMask);

	return nullptr;
}



// Returns a DIBitmap. The DIBitmap is allocated using new[] and must be deleted[] by the caller!
DIBitmap GetIconBitmap(const LPCTSTR filePath, int &width, int &height)
{
	HICON hIcon = GetIcon(filePath);
	if (!hIcon)
		return nullptr;
	
#if 0	// use this, if the created bitmap shall be of the same size as the icon
	MyIconInfo info = { 0 };
	if (!MyGetIconInfo(hIcon, info))
	{
		info.nWidth = 32;
		info.nHeight = 32;
		info.nBitsPerPixel = 24;
	}
	width = info.nWidth;
	height = info.nHeight;
#else
	width = 256;
	height = 256;
#endif
//	DIBitmap bitmap = CreateDIBitmapFromHICON(hIcon, width, height);
	DIBitmap bitmap = CreateDIBitmapFromHICON2(hIcon, width, height);
	DestroyIcon(hIcon);
	return bitmap;
}
