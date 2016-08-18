#define CYBOZU_TEST_DISABLE_AUTO_RUN
// #define MIE_STRING_INLINE
// #define MIE_STRING_IMPL
#include "mie_string.h"
#include <assert.h>
#include <string.h>
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>
#include <cybozu/mmap.hpp>

std::string g_fileName;

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

const char *findCRLF(const char *text, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		char c = text[i];
		if (c == '\r' || c == '\n') return text + i;
	}
	return NULL;
}

const char *mie_findCRLF(const char *text, size_t size)
{
	return mie_findCharAny(text, size, "\r\n", 2);
}

const char *findDigit(const char *text, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		char c = text[i];
		if (isdigit(c)) return text + i;
	}
	return NULL;
}

const char *mie_findDigit(const char *text, size_t size)
{
	return mie_findCharRange(text, size, "09", 2);
}

const char *findTab(const char *text, size_t size)
{
	return (const char*)memchr(text, '\t', size);
}

const char *mie_findTab(const char *text, size_t size)
{
	return mie_findCharAny(text, size, "\t", 1);
}

const char *findRare(const char *text, size_t size)
{
	return (const char*)memchr(text, '`', size);
}

const char *mie_findRare(const char *text, size_t size)
{
	return mie_findCharAny(text, size, "`", 1);
}

const char *findHex(const char *text, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		char c = text[i];
		if (memchr("0123456789abcdefABCDEF", c, 10 + 6 + 6)) return text + i;
	}
	return NULL;
}

const char *mie_findHex(const char *text, size_t size)
{
	return mie_findCharRange(text, size, "09afAF", 6);
}

const char *findAlnum(const char *text, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (isalnum(text[i])) return text + i;
	}
	return NULL;
}

const char *mie_findAlnum(const char *text, size_t size)
{
	return mie_findCharRange(text, size, "09azAZ", 6);
}

typedef const char* (*find_t)(const char*, size_t);

size_t seekText(const char *text, size_t size, find_t f)
{
	size_t n = 0;
	while (size > 0) {
		const char *p = f(text, size);
		if (p == 0) break;
		size_t s = p - text + 1;
		n += s;
		size -= s;
		text = p + 1;
	}
	return n;
}

void benchSub(const char *msg, const char *text, size_t size, find_t f, find_t g)
{
	printf("%s\n", msg);
	size_t a = seekText(text, size, f);
	size_t b = seekText(text, size, g);
	CYBOZU_TEST_EQUAL(a, b);
	const int n = 1;
	CYBOZU_BENCH_C("mie  ", n, seekText, text, size, mie_findCRLF);
	CYBOZU_BENCH_C("naive", n, seekText, text, size, findCRLF);
}

CYBOZU_TEST_AUTO(bench)
{
	printf("file=%s\n", g_fileName.c_str());
	cybozu::Mmap m(g_fileName);
	const char *text = m.get();
	size_t size = m.size();
	benchSub("CRLF", text, size, findCRLF, mie_findCRLF);
	benchSub("rare", text, size, findRare, mie_findRare);
	benchSub("digit", text, size, findDigit, mie_findDigit);
	benchSub("tab", text, size, findTab, mie_findTab);
	benchSub("hex", text, size, findHex, mie_findHex);
	benchSub("alnum", text, size, findAlnum, mie_findAlnum);
}

int main(int argc, char *argv[])
{
	g_fileName = argc == 1 ? "mie_string_test.cpp" : argv[1];
	return cybozu::test::autoRun.run(argc, argv);
}
