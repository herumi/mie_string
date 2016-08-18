#pragma once
#include <stdlib.h>

// #define MIE_STRING_DECL
// #define MIE_STRING_INLINE
// #define MIE_STRING_IMPL
#if !defined(MIE_STRING_INLINE) && !defined(MIE_STRING_DECL) && !defined(MIE_STRING_IMPL)
	#define MIE_STRING_INLINE
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
#ifndef MIE_ALIGN
	#ifdef _MSC_VER
		#define MIE_ALIGN(x) __declspec(align(x))
	#else
		#define MIE_ALIGN(x) __attribute__((aligned(x)))
	#endif
#endif

inline __m128i mie_shr_byte(__m128i v, size_t shift)
{
	static const unsigned char MIE_ALIGN(16) shiftPtn[32] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
	};
	__m128i s = _mm_loadu_si128((const __m128i*)(shiftPtn + shift));
	return _mm_shuffle_epi8(v, s);
}

#ifdef MIE_STRING_INLINE
inline
#endif
const char *mie_findCharAny(const char *p, size_t size, const char *key, size_t keySize)
{
	const int mode = 0;
	const __m128i r = _mm_loadu_si128((const __m128i*)key);
	__m128i v;
	for (;;) {
		if (size >= 16) {
			v = _mm_loadu_si128((const __m128i*)p);
		} else {
			if (size == 0) return NULL;
			size_t addr = (size_t)p;
			size_t addr2 = addr & 0xfff;
			if (addr2 > 0xff0 && addr2 + size <= 0x1000) {
				addr2 = addr & ~(size_t)15;
				v = mie_shr_byte(_mm_load_si128((const __m128i*)addr2), addr & 15);
			} else {
				v = _mm_loadu_si128((const __m128i*)p);
			}
		}
		if (!_mm_cmpestra(r, (int)keySize, v, (int)size, mode)) break;
		p += 16;
		size -= 16;
	}
	if (_mm_cmpestrc(r, (int)keySize, v, (int)size, mode)) {
		return p += _mm_cmpestri(r, (int)keySize, v, (int)size, mode);
	}
	return NULL;
}

#ifdef MIE_STRING_INLINE
inline
#endif
const char *mie_findCharRange(const char *p, size_t size, const char *key, size_t keySize)
{
	const int mode = 4;
	const __m128i r = _mm_loadu_si128((const __m128i*)key);
	__m128i v;
	for (;;) {
		if (size >= 16) {
			v = _mm_loadu_si128((const __m128i*)p);
		} else {
			if (size == 0) return NULL;
			size_t addr = (size_t)p;
			size_t addr2 = addr & 0xfff;
			if (addr2 > 0xff0 && addr2 + size <= 0x1000) {
				addr2 = addr & ~(size_t)15;
				v = mie_shr_byte(_mm_load_si128((const __m128i*)addr2), addr & 15);
			} else {
				v = _mm_loadu_si128((const __m128i*)p);
			}
		}
		if (!_mm_cmpestra(r, (int)keySize, v, (int)size, mode)) break;
		p += 16;
		size -= 16;
	}
	if (_mm_cmpestrc(r, (int)keySize, v, (int)size, mode)) {
		return p += _mm_cmpestri(r, (int)keySize, v, (int)size, mode);
	}
	return NULL;
}

#endif
