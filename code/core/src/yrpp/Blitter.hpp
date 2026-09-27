// Private definitions of existing YRpp Blitter helpers.
#pragma once
#include "yrpp/Blitters/Blitter.h"
#include "yrpp/DrawingBuffers.h"
#include "yrpp/AlphaLightingRemapClass.h"
#include <algorithm>
#include <cstring>

template<bool UseZBuffer, bool UseABuffer, typename T>
inline void RLEBlitter::Process_Pre_Lines(T*& dest, byte*& src, int& len, const int& line, WORD*& zbuf, WORD*& abuf)
{
	if (line > 0)
	{
		int off = -line;
		do
		{
			if (*src++)
				++off;
			else
				off += *src++;
		}
		while (off < 0);

		dest += off;
		len -= off;
		// YR 0x0049915D..0x0049918F advances color/Z/alpha across the
		// clipped transparent prefix, but not the auxiliary Z-adjust pointer.
		// Keep this original behavior: with SHA depth, camera clipping can
		// change occlusion.

		if constexpr (UseZBuffer)
		{
			zbuf += off;
			ZBuffer::Instance->AdjustPointer(zbuf);
		}
		if constexpr (UseABuffer)
		{
			abuf += off;
			ABuffer::Instance->AdjustPointer(abuf);
		}
	}
}

template<bool UseZBuffer, bool UseABuffer, int ZMode, typename T, typename Fn>
inline void RLEBlitter::Process_Pixel_Datas(T* dest, byte* src, int len, int zbase, WORD* zbuf, WORD* abuf, byte* zadjust, Fn f)
{
	if (len < 0)
		return;

	while (len > 0)
	{
		if (byte srcv = *src++)
		{
			if constexpr (ZMode == 1) {
				f(*dest, srcv, zbase, *zbuf++, zadjust[0], zadjust[1]);
				zadjust += 2;
			} else if constexpr (UseZBuffer && UseABuffer)
				f(*dest, srcv, zbase, *zbuf++, *zadjust++, *abuf++);
			else if constexpr (UseZBuffer && !UseABuffer)
				f(*dest, srcv, zbase, *zbuf++, *zadjust++);
			else if constexpr (!UseZBuffer && UseABuffer)
				f(*dest, srcv, *abuf++);
			else // !UseZBuffer && !UseABuffer
				f(*dest, srcv);

			++dest;
			--len;
		}
		else
		{
			byte off = *src++;
			len -= off;
			dest += off;

			if constexpr (UseZBuffer && ZMode != 2)
			{
				zbuf += off;
				zadjust += off;
			}
			if constexpr (UseABuffer)
				abuf += off;
		}

		if constexpr (UseZBuffer)
			ZBuffer::Instance->AdjustPointer(zbuf);
		if constexpr (UseABuffer)
			ABuffer::Instance->AdjustPointer(abuf);
	}
}
