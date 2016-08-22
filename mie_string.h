#pragma once
/**
	@file
	@brief fast function to find character by SSE4.2
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <stdlib.h>

// #define MIE_STRING_DECL
// #define MIE_STRING_INLINE
// #define MIE_STRING_IMPL
#if !defined(MIE_STRING_INLINE) && !defined(MIE_STRING_DECL) && !defined(MIE_STRING_IMPL) && !defined(MIE_STRING_DECL_ASM)
	#define MIE_STRING_INLINE
#endif

#ifdef MIE_STRING_DECL_ASM
#if defined(__AVX__)
	#define mie_findCharAny mie_findCharAnyAVX
	#define mie_findCharRange mie_findCharRangeAVX
#else
	#define mie_findCharAny mie_findCharAnySSE
	#define mie_findCharRange mie_findCharRangeSSE
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
	find any char of key[0], ..., key[keySize - 1] in [p, p + size) and return the pointer
	if not found then return NULL
	@note (0 < keySize) && (keySize <= 16)
*/
const char *mie_findCharAny(const char *p, size_t size, const char *key, size_t keySize);

/*
	find character in range [key[0]..key[1]], [key[2]..key[3]], ... in [p, p + size) and return the pointer
	if not found then return NULL
	@note (0 < keySize) && (keySize <= 16) && ((keySize % 2) == 0)
*/
const char *mie_findCharRange(const char *p, size_t size, const char *key, size_t keySize);

#ifdef __cplusplus
}
#endif

#if defined(MIE_STRING_INLINE) || defined(MIE_STRING_IMPL)

#ifdef _MSC_VER
	#include <intrin.h>
#else
	#include <x86intrin.h>
#endif

#ifdef _MSC_VER
__inline
#else
inline
#endif
__m128i mie_safe_load(const void *p, size_t size)
{
	static const unsigned char shiftPtn[32] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
	};
	size_t addr = (size_t)p;
	size_t addr2 = addr & 0xfff;
	if (addr2 > 0xff0 && addr2 + size <= 0x1000) {
		addr2 = addr & ~(size_t)15;
		size_t shift = addr & 15;
		__m128i ptn = _mm_loadu_si128((const __m128i*)(shiftPtn + shift));
		return _mm_shuffle_epi8(_mm_load_si128((const __m128i*)addr2), ptn);
	} else {
		return _mm_loadu_si128((const __m128i*)p);
	}
}

#define MIE_FIND_CHAR_GENERIC(mode) \
	const __m128i r = _mm_loadu_si128((const __m128i*)key); \
	__m128i v; \
	int c; \
	if (size >= 16) { \
		size_t left = size & ~15; \
		do { \
			v = _mm_loadu_si128((const __m128i*)p); \
			c = _mm_cmpestri(r, (int)keySize, v, (int)left, mode); \
			if (c != 16) { \
				p += c; \
				return p; \
			} \
			p += 16; \
			left -= 16; \
		} while (left != 0); \
	} \
	size &= 15; \
	if (size == 0) return NULL; \
	v = mie_safe_load(p, size); \
	c = _mm_cmpestri(r, (int)keySize, v, (int)size, mode); \
	if (c != 16) { \
		return p + c; \
	} \
	return NULL;

#ifdef MIE_STRING_INLINE
inline
#endif
const char *mie_findCharAny(const char *p, size_t size, const char *key, size_t keySize)
{
	MIE_FIND_CHAR_GENERIC(0)
}

#ifdef MIE_STRING_INLINE
inline
#endif
const char *mie_findCharRange(const char *p, size_t size, const char *key, size_t keySize)
{
	MIE_FIND_CHAR_GENERIC(4)
}
#undef MIE_FIND_CHAR_GENERIC

#endif
