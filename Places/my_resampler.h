
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <vector>
#include <algorithm>

// Resampler Code: https://code.google.com/p/imageresampler/
#include "resampler.h"


#define RSMP_MIN(a, b)		((a < b) ? a : b)
#define RSMP_MAX(a, b)		((a > b) ? a : b)

const int srgb_to_linear_table_size = 256;
const int linear_to_srgb_table_size = 4096;

class CResampler
{
protected:
	float m_fSourceGamma;		// Partial gamma correction looks better on mips. Set to 1.0 to disable gamma correction.
	float m_fFilterScale;		// Filter scale - values < 1.0 cause aliasing, but create sharper looking mips.

	float			m_SRGB_To_Linear[srgb_to_linear_table_size];
	unsigned char	m_Linear_to_SRGB[linear_to_srgb_table_size];

public:
	CResampler()
		: m_fFilterScale(1.0f)	//.75f;
	{
		SetSourceGamma(1.75f);
	}


	void SetSourceGamma(float gamma)
	{
		m_fSourceGamma = gamma;

		for (int i = 0; i < srgb_to_linear_table_size; ++i)
			m_SRGB_To_Linear[i] = (float)pow(i * 1.0f / 255.0f, m_fSourceGamma);

		const float inv_linear_to_srgb_table_size = 1.0f / linear_to_srgb_table_size;
		const float inv_source_gamma = 1.0f / m_fSourceGamma;

		for (int i = 0; i < linear_to_srgb_table_size; ++i)
		{
			int k = (int)(255.0f * pow(i * inv_linear_to_srgb_table_size, inv_source_gamma) + .5f);
			if (k < 0)
				k = 0;
			else if (k > 255)
				k = 255;
			m_Linear_to_SRGB[i] = (unsigned char)k;
		}
	}


	// returns resampled image or NULL on error
	// returned image must be freed by the caller!
	unsigned char *Resample(unsigned char *pSrc_image, int src_width, int src_height, int channels, int dst_width, int dst_height)
	{

		if ((RSMP_MIN(dst_width, dst_height) < 1) || (RSMP_MAX(dst_width, dst_height) > RESAMPLER_MAX_DIMENSION))
		{
			// printf("Invalid output width/height!\n");
			return NULL;
		}

		const int max_components = 4;   

		if ((RSMP_MAX(src_width, src_height) > RESAMPLER_MAX_DIMENSION) || (channels > max_components))
		{
			// printf("Image is too large!\n");
			return NULL;
		}

		const char* pFilter = "blackman";	//RESAMPLER_DEFAULT_FILTER;

		Resampler* resamplers[max_components];
		std::vector<float> samples[max_components];

		// Now create a Resampler instance for each component to process. The first instance will create new contributor tables, which are shared by the resamplers 
		// used for the other components (a memory and slight cache efficiency optimization).
		resamplers[0] = new Resampler(src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, NULL, NULL, m_fFilterScale, m_fFilterScale);
		samples[0].resize(src_width);
		for (int i = 1; i < channels; i++)
		{
			resamplers[i] = new Resampler(src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, resamplers[0]->get_clist_x(), resamplers[0]->get_clist_y(), m_fFilterScale, m_fFilterScale);
			samples[i].resize(src_width);
		}      

		unsigned char *dst_image = new unsigned char[dst_width * channels * dst_height];

		const int src_pitch = src_width * channels;
		const int dst_pitch = dst_width * channels;
		int dst_y = 0;

		// printf("Resampling to %ux%u\n", dst_width, dst_height);

		for (int src_y = 0; src_y < src_height; src_y++)
		{
			const unsigned char* pSrc = &pSrc_image[src_y * src_pitch];

			for (int x = 0; x < src_width; x++)
			{
				for (int c = 0; c < channels; c++)
				{
					if ((c == 3) || ((channels == 2) && (c == 1)))
						samples[c][x] = *pSrc++ * (1.0f/255.0f);
					else
						samples[c][x] = m_SRGB_To_Linear[*pSrc++];        
				}
			}

			for (int c = 0; c < channels; c++)         
			{
				if (!resamplers[c]->put_line(&samples[c][0]))
				{
					// printf("Out of memory!\n");
					return NULL;
				}
			}         

			for ( ; ; )
			{
				int comp_index;
				for (comp_index = 0; comp_index < channels; comp_index++)
				{
					const float* pOutput_samples = resamplers[comp_index]->get_line();
					if (!pOutput_samples)
						break;

					const bool alpha_channel = (comp_index == 3) || ((channels == 2) && (comp_index == 1));
					assert(dst_y < dst_height);
					unsigned char* pDst = &dst_image[dst_y * dst_pitch + comp_index];

					for (int x = 0; x < dst_width; x++)
					{
						if (alpha_channel)
						{
							int c = (int)(255.0f * pOutput_samples[x] + .5f);
							if (c < 0) c = 0; else if (c > 255) c = 255;
							*pDst = (unsigned char)c;
						}
						else
						{
							int j = (int)(linear_to_srgb_table_size * pOutput_samples[x] + .5f);
							if (j < 0) j = 0; else if (j >= linear_to_srgb_table_size) j = linear_to_srgb_table_size - 1;
							*pDst = m_Linear_to_SRGB[j];
						}

						pDst += channels;
					}
				}     
				if (comp_index < channels)
					break; 

				dst_y++;
			}
		}

		// Delete the resamplers.
		for (int i = 0; i < channels; i++)
			delete resamplers[i];

		return dst_image;
	}
};
