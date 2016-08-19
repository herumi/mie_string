// #define MIE_STRING_INLINE
// #define MIE_STRING_IMPL
#include "mie_string.h"
#include <assert.h>
#include <string.h>
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>
#include <cybozu/mmap.hpp>
#include <cybozu/xorshift.hpp>

#ifndef MIE_ALIGN
	#ifdef _MSC_VER
		#define MIE_ALIGN(x) __declspec(align(x))
	#else
		#define MIE_ALIGN(x) __attribute__((aligned(x)))
	#endif
#endif

const char *findCharAnyC(const char *p, size_t size, const char *key, size_t keySize)
{
	for (size_t i = 0; i < size; i++) {
		const char c = p[i];
		if (memchr(key, c, keySize) != 0) return p + i;
	}
	return NULL;
}

const char *findCharRangeC(const char *p, size_t size, const char *key, size_t keySize)
{
	for (size_t i = 0; i < size; i++) {
		const char c = p[i];
		for (size_t j = 0; j < keySize / 2; j++) {
			if (key[j * 2] <= c && c <= key[j * 2 + 1]) return p + i;
		}
	}
	return NULL;
}

const char *textTbl[] = {
	"abcdefghijklmnopqrstuvexyz",
	"aaabbbsssddd",
	"Y",
	"-0",
	"--1",
	"---2",
	"----3",
	"-----4",
	"------5",
	"-------6",
};

CYBOZU_TEST_AUTO(findAny)
{
	const char *keyTbl[] = {
		"d",
		"X",
		"ds",
		"sd",
		"lmXYz",
		"0123456789abcdef",
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(textTbl); i++) {
		const char *text = textTbl[i];
		const size_t size = strlen(text);
		for (size_t j = 0; j < CYBOZU_NUM_OF_ARRAY(keyTbl); j++) {
			const char *key = keyTbl[j];
			const size_t keySize = strlen(key);
			for (size_t k = 0; k < size; k++) {
				const char *p = mie_findCharAny(text + k, size - k, key, keySize);
				const char *q = findCharAnyC(text + k , size - k, key, keySize);
				CYBOZU_TEST_EQUAL_POINTER(p, q);
			}
		}
	}
}

CYBOZU_TEST_AUTO(findRange)
{
	const char *keyTbl[] = {
		"09",
		"xy",
		"57",
		"su03",
		"AZ09gk",
		"0011223344556677",
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(textTbl); i++) {
		const char *text = textTbl[i];
		const size_t size = strlen(text);
		for (size_t j = 0; j < CYBOZU_NUM_OF_ARRAY(keyTbl); j++) {
			const char *key = keyTbl[j];
			const size_t keySize = strlen(key);
			for (size_t k = 0; k < size; k++) {
				const char *p = mie_findCharRange(text + k, size - k, key, keySize);
				const char *q = findCharRangeC(text + k , size - k, key, keySize);
				CYBOZU_TEST_EQUAL_POINTER(p, q);
			}
		}
	}
}

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif
const char *getBoundary()
{
	const size_t pageSize = 4096;
	static MIE_ALIGN(4096) char buf[pageSize * 2];
	{
#ifdef _WIN32
		DWORD old;
		bool isOK = VirtualProtect(buf + pageSize, pageSize, PAGE_NOACCESS, &old) != 0;
#else
		bool isOK = mprotect(buf + pageSize, pageSize, PROT_NONE) == 0;
#endif
		if (!isOK) {
			perror("protect");
			fprintf(stderr, "can't change access mode\n");
			exit(1);
		}
	}
	char *base = buf + pageSize - 32;
	for (int i = 0; i < 31; i++) {
		base[i] = 'x';
	}
	base[31] = 'y';
	return base;
}

CYBOZU_TEST_AUTO(edge)
{
	const char *const base = getBoundary();
	for (size_t i = 0; i < 32; i++) {
		const char *text = base + i;
		const size_t size = 32 - i;
		const char *p, *q;
		p = findCharAnyC(text, size, "abc", 3);
		q = mie_findCharAny(text, size, "abc", 3);
		CYBOZU_TEST_EQUAL_POINTER(p, q);

		p = findCharAnyC(text, size, "abcy", 4);
		q = mie_findCharAny(text, size, "abcy", 4);
		CYBOZU_TEST_EQUAL_POINTER(p, q);

		p = findCharRangeC(text, size, "09awAZ", 6);
		q = mie_findCharRange(text, size, "09awAZ", 6);
		CYBOZU_TEST_EQUAL_POINTER(p, q);

		p = findCharRangeC(text, size, "09awAZyz", 8);
		q = mie_findCharRange(text, size, "09awAZyz", 8);
		CYBOZU_TEST_EQUAL_POINTER(p, q);
	}
}

typedef const char* (*find_t)(const char*, size_t);

int seekText(const char *text, size_t size, find_t f)
{
	int n = 0;
	const char *end = text + size;
	while (text < end) {
		const char *p = f(text, size);
		if (p == 0) break;
		text = p + 1;
		size = end - text;
		n++;
	}
	return n;
}

/*
	fill text with any of tbl[] and put 'x' at the interval ave +/- d/2
*/
void makeText(std::string& text, size_t n, size_t ave, size_t d, const char *tbl)
{
	if (ave < d) throw cybozu::Exception("makeText:bad ave") << ave << d;
	cybozu::XorShift rg;
	text.resize(n);
	for (size_t i = 0; i < n; i++) text[i] = '.';
	size_t tblLen = strlen(tbl);
	size_t pos = 0;
	for (;;) {
		pos += ave + (rg() % d)  - (d / 2);
		if (pos >= n) break;
		text[pos] = tbl[rg() % tblLen];
	}
}

const char *memchr_find_x(const char *text, size_t size)
{
	return (const char*)memchr(text, 'x', size);
}

const char *loop_find_x(const char *text, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (text[i] == 'x') return text + i;
	}
	return NULL;
}

const char *any_find_x(const char *text, size_t size)
{
	return mie_findCharAny(text, size, "x", 1);
}

const char *range_find_x(const char *text, size_t size)
{
	return mie_findCharRange(text, size, "xx", 2);
}

CYBOZU_TEST_AUTO(find1)
{
	puts("find x");
	const size_t n = 1024 * 1024;
	const struct Tbl {
		size_t ave;
		size_t d;
	} tbl[] = {
		{ 5, 2 },
		{ 10, 5 },
		{ 16, 3 },
		{ 24, 4 },
		{ 32, 5 },
		{ 64, 5 },
		{ 128, 5 },
		{ 256, 5 },
	};
	const int C = 100;
	std::string text;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		makeText(text, n, tbl[i].ave, tbl[i].d, "x");
		int c0, c1;
		printf("ave=%d\n", (int)tbl[i].ave);
		c0 = 0; CYBOZU_BENCH_C("findAny ", C, c0 += seekText, text.c_str(), text.size(), any_find_x);

		c1 = 0; CYBOZU_BENCH_C("findChar", C, c1 += seekText, text.c_str(), text.size(), range_find_x);
		CYBOZU_TEST_EQUAL(c0, c1);

		c1 = 0; CYBOZU_BENCH_C("memchr  ", C, c1 += seekText, text.c_str(), text.size(), memchr_find_x);
		CYBOZU_TEST_EQUAL(c0, c1);

		c1 = 0; CYBOZU_BENCH_C("loop    ", C, c1 += seekText, text.c_str(), text.size(), loop_find_x);
		CYBOZU_TEST_EQUAL(c0, c1);
	}
}

const char *loop_find_sp(const char *text, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		char c = text[i];
		if (c == ' ' || c == '\r' || c == '\n' || c == '\t') return text + i;
	}
	return NULL;
}

const struct SpTbl {
	char tbl[256];
	SpTbl()
	{
		memset(tbl, 0, sizeof(tbl));
		tbl[' '] = 1;
		tbl['\r'] = 1;
		tbl['\n'] = 1;
		tbl['\t'] = 1;
	}
} spTbl;

const char *tbl_find_sp(const char *text, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		unsigned char c = text[i];
		if (spTbl.tbl[c]) return text + i;
	}
	return NULL;
}

const char *any_find_sp(const char *text, size_t size)
{
	return mie_findCharAny(text, size, " \r\n\t", 4);
}

CYBOZU_TEST_AUTO(find_sp)
{
	puts("find sp");
	const size_t n = 1024 * 1024;
	const struct Tbl {
		size_t ave;
		size_t d;
	} tbl[] = {
		{ 5, 2 },
		{ 10, 5 },
		{ 16, 3 },
		{ 24, 4 },
		{ 32, 5 },
		{ 64, 5 },
		{ 128, 5 },
		{ 256, 5 },
	};
	const int C = 100;
	std::string text;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		makeText(text, n, tbl[i].ave, tbl[i].d, " \r\n\t");
		int c0, c1;
		printf("ave=%d\n", (int)tbl[i].ave);
		c0 = 0; CYBOZU_BENCH_C("findAny ", C, c0 += seekText, text.c_str(), text.size(), any_find_sp);

		c1 = 0; CYBOZU_BENCH_C("loop    ", C, c1 += seekText, text.c_str(), text.size(), loop_find_sp);
		CYBOZU_TEST_EQUAL(c0, c1);

		c1 = 0; CYBOZU_BENCH_C("tbl     ", C, c1 += seekText, text.c_str(), text.size(), tbl_find_sp);
		CYBOZU_TEST_EQUAL(c0, c1);
	}
}

const char *loop_find_hex(const char *text, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		char c = text[i];
		if ((unsigned char)(c - '0') <= 9) return text + i;
		if ((unsigned char)(c - 'a') <= 6) return text + i;
	}
	return NULL;
}

const struct HexTbl {
	char tbl[256];
	HexTbl()
	{
		memset(tbl, 0, sizeof(tbl));
		for (int i = 0; i < 10; i++) {
			tbl['0' + i] = 1;
		}
		for (int i = 0; i < 6; i++) {
			tbl['a' + i] = 1;
		}
	}
} hexTbl;

const char *tbl_find_hex(const char *text, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		unsigned char c = text[i];
		if (hexTbl.tbl[c]) return text + i;
	}
	return NULL;
}

const char *any_find_hex(const char *text, size_t size)
{
	return mie_findCharAny(text, size, "0123456789abcdef", 16);
}

const char *range_find_hex(const char *text, size_t size)
{
	return mie_findCharRange(text, size, "09af", 4);
}

CYBOZU_TEST_AUTO(find_hex)
{
	puts("find hex");
	const size_t n = 1024 * 1024;
	const struct Tbl {
		size_t ave;
		size_t d;
	} tbl[] = {
		{ 5, 2 },
		{ 10, 5 },
		{ 16, 3 },
		{ 24, 4 },
		{ 32, 5 },
		{ 64, 5 },
		{ 128, 5 },
		{ 256, 5 },
	};
	const int C = 100;
	std::string text;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		makeText(text, n, tbl[i].ave, tbl[i].d, "0123456789abcdef");
		int c0, c1;
		printf("ave=%d\n", (int)tbl[i].ave);
		c0 = 0; CYBOZU_BENCH_C("findAny  ", C, c0 += seekText, text.c_str(), text.size(), any_find_hex);

		c1 = 0; CYBOZU_BENCH_C("loop     ", C, c1 += seekText, text.c_str(), text.size(), loop_find_hex);
		CYBOZU_TEST_EQUAL(c0, c1);

		c1 = 0; CYBOZU_BENCH_C("tbl      ", C, c1 += seekText, text.c_str(), text.size(), tbl_find_hex);
		CYBOZU_TEST_EQUAL(c0, c1);

		c1 = 0; CYBOZU_BENCH_C("findRange", C, c1 += seekText, text.c_str(), text.size(), range_find_hex);
		CYBOZU_TEST_EQUAL(c0, c1);
	}
}
