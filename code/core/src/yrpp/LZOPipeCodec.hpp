/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwlib/lzo1x_c.cpp                            $*
 *                                                                                             *
 *                      $Author:: Jani_p                                                      $*
 *                                                                                             *
 *                     $Modtime:: 6/28/00 10:13a                                              $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* $Header: /Commando/Code/wwlib/lzo1x_c.cpp 2     7/05/00 6:26p Jani_p $ */
/* lzo1x_c.c -- standalone LZO1X-1 compressor

   This file is part of the LZO real-time data compression library.

   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the LZO library; see the file COPYING.LIB.
   If not, write to the Free Software Foundation, Inc.,
   675 Mass Ave, Cambridge, MA 02139, USA.

   Markus F.X.J. Oberhumer
   markus.oberhumer@jk.uni-linz.ac.at
 */

// Portable adaptation of CnC_Renegade 3e00c3a1, lzo1x_c.cpp/lzo1x_d.cpp.
// Same LZO1X algorithm and original stream module; bounded decoding and a
// zero-initialized per-call dictionary replace uninitialized pointer reads.
#pragma once
#include "yrpp/Memory.h"
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <memory>
namespace {
using lzo_byte = unsigned char;
using lzo_uint = std::uint32_t;
using lzo_voidp = void*;
using lzo_bytepp = const lzo_byte**;
using lzo_ptrdiff_t = std::ptrdiff_t;
constexpr int LZO_E_OK = 0;
#define LZO1X
#define LZO_BYTE(n) static_cast<lzo_byte>(n)
#define M1_MAX_OFFSET	0x0400
#if defined(LZO1X)
#define M2_MAX_OFFSET	0x0800
#elif defined(LZO1Y)
#define M2_MAX_OFFSET	0x0400
#endif
#define M3_MAX_OFFSET	0x4000
#define M4_MAX_OFFSET	0xbfff

#define MX_MAX_OFFSET	(M1_MAX_OFFSET + M2_MAX_OFFSET)

#define M1_MARKER		0
#define M2_MARKER		64
#define M3_MARKER		32
#define M4_MARKER		16


#define _DV2(p,shift1,shift2) \
		(((( (lzo_uint)(p[2]) << shift1) ^ p[1]) << shift2) ^ p[0])
#define DVAL_NEXT(dv,p) \
		dv ^= p[-1]; dv = (((dv) >> 5) ^ ((lzo_uint)(p[2]) << (2*5)))
#define _DV(p,shift) 		_DV2(p,shift,shift)
#define DVAL_FIRST(dv,p)	dv = _DV((p),5)
#define _DINDEX(dv,p)		((40799u * (dv)) >> 5)
#define DINDEX(dv,p)		(((_DINDEX(dv,p)) & 0x3fff) << 0)
#define UPDATE_D(dict,cycle,dv,p)		dict[ DINDEX(dv,p) ] = (p)
#define UPDATE_I(dict,cycle,index,p)	dict[index] = (p)


/***********************************************************************
// compress a block of data.
************************************************************************/
static int do_compress(const lzo_byte * in, lzo_uint  in_len,
	lzo_byte *out, lzo_uint *out_len,
	lzo_voidp wrkmem )
{

	const lzo_byte *ip;
	lzo_uint dv;
	lzo_byte *op;
	const lzo_byte * const in_end = in + in_len;
	const lzo_byte * const ip_end = in + in_len - 9 - 4;
	const lzo_byte *ii;
	const lzo_bytepp const dict = (const lzo_bytepp) wrkmem;

	op = out;
	ip = in;
	ii = ip;

	DVAL_FIRST(dv,ip); UPDATE_D(dict,cycle,dv,ip); ip++;
	DVAL_NEXT(dv,ip);  UPDATE_D(dict,cycle,dv,ip); ip++;
	DVAL_NEXT(dv,ip);  UPDATE_D(dict,cycle,dv,ip); ip++;
	DVAL_NEXT(dv,ip);  UPDATE_D(dict,cycle,dv,ip); ip++;

	for (;;) {
		const lzo_byte *m_pos;
		lzo_uint m_len;
		lzo_ptrdiff_t m_off;
		lzo_uint lit;

		lzo_uint dindex = DINDEX(dv,ip);
		m_pos = dict[dindex];
		UPDATE_I(dict,cycle,dindex,ip);


		if ((!m_pos || (m_off = ip - m_pos) <= 0 || m_off > M4_MAX_OFFSET)) {
		}
#if defined(LZO_UNALIGNED_OK_2)
		else
			if (* (unsigned short *) m_pos != * (unsigned short *) ip)
#else
		else
			if (m_pos[0] != ip[0] || m_pos[1] != ip[1])
#endif
		{
		} else {
			if (m_pos[2] == ip[2]) {
				lit = ip - ii;
				m_pos += 3;
				if (m_off <= M2_MAX_OFFSET)
					goto match;

				/* better compression, but slower */
				if (lit == 3) {
					assert(op - 2 > out); op[-2] |= LZO_BYTE(3);
					*op++ = *ii++; *op++ = *ii++; *op++ = *ii++;
					goto code_match;
				}

				if (*m_pos == ip[3]) {
					goto match;
				}
			} else {
				/* still need a better way for finding M1 matches */
			}
		}


	/* a literal */
		++ip;
		if (ip >= ip_end) {
			break;
		}
		DVAL_NEXT(dv,ip);
		continue;


	/* a match */

match:

		/* store current literal run */
		if (lit > 0) {
			lzo_uint t = lit;

			if (t <= 3) {
				assert(op - 2 > out);
				op[-2] |= LZO_BYTE(t);
			} else {
				if (t <= 18) {
					*op++ = LZO_BYTE(t - 3);
				} else {
					lzo_uint tt = t - 18;

					*op++ = 0;
					while (tt > 255) {
						tt -= 255;
						*op++ = 0;
					}
					assert(tt > 0);
					*op++ = LZO_BYTE(tt);
				}
			}

			do {
				*op++ = *ii++;
			} while (--t > 0);
		}


		/* code the match */
code_match:
		assert(ii == ip);
		ip += 3;
		if (*m_pos++ != *ip++ || *m_pos++ != *ip++ || *m_pos++ != *ip++ ||
		    *m_pos++ != *ip++ || *m_pos++ != *ip++ || *m_pos++ != *ip++)
		{
			--ip;
			m_len = ip - ii;
			assert(m_len >= 3); assert(m_len <= 8);

			if (m_off <= M2_MAX_OFFSET) {
				m_off -= 1;
				*op++ = LZO_BYTE(((m_len - 1) << 5) | ((m_off & 7) << 2));
				*op++ = LZO_BYTE(m_off >> 3);
			} else {
				if (m_off <= M3_MAX_OFFSET) {
					m_off -= 1;
					*op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
					goto m3_m4_offset;
				} else {
					m_off -= 0x4000;
					assert(m_off > 0); assert(m_off <= 0x7fff);
					*op++ = LZO_BYTE(M4_MARKER |
					                 ((m_off & 0x4000) >> 11) | (m_len - 2));
					goto m3_m4_offset;
				}
			}
		} else {
			const lzo_byte *end;
			end = in_end;
			while (ip < end && *m_pos == *ip) {
				m_pos++;
				ip++;
			}
			m_len = (ip - ii);
			assert(m_len >= 3);

			if (m_off <= M3_MAX_OFFSET) {
				m_off -= 1;
				if (m_len <= 33) {
					*op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
				} else {
					m_len -= 33;
					*op++ = M3_MARKER | 0;
					goto m3_m4_len;
				}
			} else {
				m_off -= 0x4000;
				assert(m_off > 0); assert(m_off <= 0x7fff);
				if (m_len <= 9) {
					*op++ = LZO_BYTE(M4_MARKER |
					                 ((m_off & 0x4000) >> 11) | (m_len - 2));
				} else {
					m_len -= 9;
					*op++ = LZO_BYTE(M4_MARKER | ((m_off & 0x4000) >> 11));
m3_m4_len:
					while (m_len > 255) {
						m_len -= 255;
						*op++ = 0;
					}
					assert(m_len > 0);
					*op++ = LZO_BYTE(m_len);
				}
			}

m3_m4_offset:
			*op++ = LZO_BYTE((m_off & 63) << 2);
			*op++ = LZO_BYTE(m_off >> 6);
		}

		ii = ip;
		if (ip >= ip_end) {
			break;
		}
		DVAL_FIRST(dv,ip);
	}

	/* store final literal run */
	if (in_end - ii > 0) {
		lzo_uint t = in_end - ii;

		if (op == out && t <= 238) {
			*op++ = LZO_BYTE(17 + t);
		} else {
			if (t <= 3) {
				op[-2] |= LZO_BYTE(t);
			} else {
				if (t <= 18) {
					*op++ = LZO_BYTE(t - 3);
				} else {
					lzo_uint tt = t - 18;

					*op++ = 0;
					while (tt > 255) {
						tt -= 255;
						*op++ = 0;
					}
					assert(tt > 0);
					*op++ = LZO_BYTE(tt);
				}
			}
		}
		do {
			*op++ = *ii++;
		} while (--t > 0);
	}

	*out_len = op - out;
	return LZO_E_OK;
}


/***********************************************************************
// public entry point
************************************************************************/

int lzo_compress     ( const lzo_byte * in, lzo_uint  in_len,
                                 lzo_byte * out, lzo_uint *out_len,
                                 lzo_voidp wrkmem )
{
	lzo_byte *op = out;
	int r = LZO_E_OK;

	if (in_len <= 0) {
		*out_len = 0;
	} else {
		if (in_len <= 9 + 4) {
			*op++ = LZO_BYTE(17 + in_len);
			do *op++ = *in++; while (--in_len > 0);
			*out_len = op - out;
		} else {
			r = do_compress(in,in_len,out,out_len,wrkmem);
		}
	}

	if (r == LZO_E_OK) {
		op = out + *out_len;
		*op++ = M4_MARKER | 1;
		*op++ = 0;
		*op = 0;
		*out_len += 3;
	}

	return r;
}



// Bounds-checked form of the pinned LZO1X decoder's literal/M1/M2/M3/M4
// state machine. Indices avoid forming pointers before the output buffer.
bool lzo_decode(const void* input, int length, void* output, int expected) {
    const auto* src = static_cast<const unsigned char*>(input);
    auto* dst = static_cast<unsigned char*>(output);
    int in = 0, out = 0, token = 0, count = 0, distance = 0;
    auto literals = [&](int n) {
        if (n < 0 || n > length - in || n > expected - out) return false;
        for (int i = 0; i < n; ++i) dst[out++] = src[in++];
        return true;
    };
    auto match = [&](int n, int back) {
        if (back <= 0 || back > out || n > expected - out) return false;
        for (int i = 0; i < n; ++i) { dst[out] = dst[out - back]; ++out; }
        return true;
    };
    auto extended = [&](int base) {
        count = base;
        while (in < length && !src[in]) {
            count += 255; ++in;
            if (count > expected) return false;
        }
        if (in == length) return false;
        count += src[in++];
        return true;
    };
    if (!length) return false;
    if (src[in] > 17) {
        count = src[in++] - 17;
        if (!literals(count)) return false;
        goto first_match;
    }
    for (;;) {
        if (in == length) return false;
        token = src[in++];
        if (token >= 16) goto match_token;
        count = token;
        if (!count && !extended(15)) return false;
        if (!literals(count + 3)) return false;
first_match:
        if (in == length) return false;
        token = src[in++];
        if (token >= 16) goto match_token;
        if (in == length) return false;
        distance = 1 + 0x800 + (token >> 2) + (src[in++] << 2);
        if (!match(3, distance)) return false;
        goto match_done;
match_token:
        if (token >= 64) {
            if (in == length) return false;
            distance = 1 + ((token >> 2) & 7) + (src[in++] << 3);
            count = (token >> 5) + 1;
        } else if (token >= 32) {
            count = token & 31;
            if (!count && !extended(31)) return false;
            if (length - in < 2) return false;
            distance = 1 + (src[in] >> 2) + (src[in + 1] << 6); in += 2;
            count += 2;
        } else if (token >= 16) {
            distance = (token & 8) << 11;
            count = token & 7;
            if (!count && !extended(7)) return false;
            if (length - in < 2) return false;
            distance += (src[in] >> 2) + (src[in + 1] << 6); in += 2;
            if (!distance) return count == 1 && out == expected && in == length;
            distance += 0x4000; count += 2;
        } else {
            if (in == length) return false;
            distance = 1 + (token >> 2) + (src[in++] << 2); count = 2;
        }
        if (!match(count, distance)) return false;
match_done:
        count = src[in - 2] & 3;
        if (!count) continue;
        if (!literals(count) || in == length) return false;
        token = src[in++];
        goto match_token;
    }
}
int lzo_capacity(int block) { return block + block / 16 + 64 + 3; }
int lzo_encode(const void* input, int length, void* output) {
    constexpr std::size_t slots = 16384;
    auto* memory = static_cast<const lzo_byte**>(YRMemory::Allocate(slots * sizeof(const lzo_byte*)));
    if (!memory) return -1;
    std::unique_ptr<const lzo_byte*, decltype(&YRMemory::Deallocate)> dictionary(memory, YRMemory::Deallocate);
    for (std::size_t i = 0; i < slots; ++i) memory[i] = nullptr;
    lzo_uint size = 0;
    lzo_compress(static_cast<const lzo_byte*>(input), lzo_uint(length), static_cast<lzo_byte*>(output), &size, memory);
    return int(size);
}
#undef M1_MAX_OFFSET
#undef M2_MAX_OFFSET
#undef M3_MAX_OFFSET
#undef M4_MAX_OFFSET
#undef MX_MAX_OFFSET
#undef M1_MARKER
#undef M2_MARKER
#undef M3_MARKER
#undef M4_MARKER
#undef _DV2
#undef DVAL_NEXT
#undef _DV
#undef DVAL_FIRST
#undef _DINDEX
#undef DINDEX
#undef UPDATE_D
#undef UPDATE_I
#undef LZO1X
#undef LZO_BYTE
struct LZOCodec {
    static int Capacity(int block) { return lzo_capacity(block); }
    static int Encode(const void* input, int length, void* output) { return lzo_encode(input, length, output); }
    static bool Decode(const void* input, int length, void* output, int expected) { return lzo_decode(input, length, output, expected); }
};
}
