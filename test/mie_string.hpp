#pragma once
/*
	string function for SSE4.2
	@NOTE
	all functions in this header will access max 16 bytes beyond the end of input string
	see http://www.slideshare.net/herumi/x86opti3

	@author herumi
	@note modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <stdlib.h>
#define XBYAK_NO_OP_NAMES
#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4127) // constant condition
#endif

/*
	define MIE_CHAR16 as you like
*/
#ifndef MIE_CHAR16
	#if defined(_MSC_VER)
		#define MIE_CHAR16 wchar_t
	#else
		#define MIE_CHAR16 unsigned short
	#endif
#endif

namespace mie {

namespace str_util_impl {

const size_t strstrOffset = 0;
const size_t strlenOffset = strstrOffset + 96;
const size_t strchrOffset = strlenOffset + 48;
const size_t strchr_anyOffset = strchrOffset + 48;
const size_t strchr_rangeOffset = strchr_anyOffset + 48;
const size_t findCharOffset = strchr_rangeOffset + 64;
const size_t findChar_anyOffset = findCharOffset + 64;
const size_t findChar_rangeOffset = findChar_anyOffset + 64;
const size_t findStrOffset = findChar_rangeOffset + 64;
const size_t strcasestrOffset = findStrOffset + 160;
const size_t findCaseStrOffset = strcasestrOffset + 224;

const size_t strstr16Offset = findCaseStrOffset + 272;
const size_t strlen16Offset = strstr16Offset + 96;
const size_t strchr16Offset = strlen16Offset + 48;
const size_t strchr16_anyOffset = strchr16Offset + 48;
const size_t strchr16_rangeOffset = strchr16_anyOffset + 48;
const size_t findChar16Offset = strchr16_rangeOffset + 64;
const size_t findChar16_anyOffset = findChar16Offset + 64;
const size_t findChar16_rangeOffset = findChar16_anyOffset + 64;
const size_t findStr16Offset = findChar16_rangeOffset + 64;
const size_t strcasestr16Offset = findStr16Offset + 272;
const size_t findCaseStr16Offset = strcasestr16Offset + 224;

struct StringCode : Xbyak::CodeGenerator {
	const Xbyak::util::Cpu cpu;
	StringCode(char *buf, size_t size)
		try
		: Xbyak::CodeGenerator(size, buf)
	{
		assert(sizeof(MIE_CHAR16) == 2);
		assert((MIE_CHAR16)(-1) > 0);
		Xbyak::CodeArray::protect(buf, size, true);
		if (!cpu.has(Xbyak::util::Cpu::tSSE42)) {
#ifndef NDEBUG
			fprintf(stderr, "warning SSE4.2 is not available.\n");
#endif
			return;
		}
		/* this check is adhoc */
		/*
			0x2A, 0x3A, 0x2D for Core i3, i7, etc
			0x2C for Xeon X5650
		*/
		const bool isSandyBridge = cpu.displayModel != 0x2C;

		gen_strstr(isSandyBridge);

		nextOffset(strlenOffset);
		gen_strlen();

		nextOffset(strchrOffset);
		gen_strchr(M_one);

		nextOffset(strchr_anyOffset);
		gen_strchr(M_any);

		nextOffset(strchr_rangeOffset);
		gen_strchr(M_range);

		nextOffset(findCharOffset);
		gen_findChar(M_one);

		nextOffset(findChar_anyOffset);
		gen_findChar(M_any);

		nextOffset(findChar_rangeOffset);
		gen_findChar(M_range);

		nextOffset(findStrOffset);
		gen_findStr(isSandyBridge);

		nextOffset(strcasestrOffset);
		gen_strstr(isSandyBridge, true);

		nextOffset(findCaseStrOffset);
		gen_findStr(isSandyBridge, true);

		nextOffset(strstr16Offset);
		gen_strstr(isSandyBridge, false, true);

		nextOffset(strlen16Offset);
		gen_strlen(true);

		nextOffset(strchr16Offset);
		gen_strchr(M_one, true);

		nextOffset(strchr16_anyOffset);
		gen_strchr(M_any, true);

		nextOffset(strchr16_rangeOffset);
		gen_strchr(M_range, true);

		nextOffset(findChar16Offset);
		gen_findChar(M_one, true);

		nextOffset(findChar16_anyOffset);
		gen_findChar(M_any, true);

		nextOffset(findChar16_rangeOffset);
		gen_findChar(M_range, true);

		nextOffset(findStr16Offset);
		gen_findStr(isSandyBridge, true);

		nextOffset(strcasestr16Offset);
		gen_strstr(isSandyBridge, true, true);

		nextOffset(findCaseStr16Offset);
		gen_findStr(isSandyBridge, true, true);

	} catch (std::exception& e) {
		printf("ERR:%s\n", e.what());
		::exit(1);
	}
private:
	enum {
		M_one, /* match one char */
		M_any, /* match any of key[0], key[1], ... */
		M_range /* match range [key[0], key[1]], [key[2], key[3]], ... */
	};
	void nextOffset(size_t pos)
	{
		size_t cur = getSize();
		if (cur >= pos) {
			fprintf(stderr, "mie:StringCode:code size is over %d %d\n", (int)cur, (int)pos);
			::exit(1);
		}
		while (cur < pos) {
			nop();
			cur++;
		}
	}
	/*
		Am1 : 'A' - 1
		Zp1 : 'Z' + 1
		amA : 'a' - 'A'
		t : temporary register
	*/
	void setLowerReg(const Xbyak::Xmm& Am1, const Xbyak::Xmm& Zp1, const Xbyak::Xmm& amA, const Xbyak::Reg32& t, bool isWcs)
	{
		const unsigned int factor = isWcs ? 0x10001 : 0x01010101;
		mov(t, ('A' - 1) * factor);
		movd(Am1, t);
		pshufd(Am1, Am1, 0); // 'A' - 1
		mov(t, ('Z' + 1) * factor);
		movd(Zp1, t);
		pshufd(Zp1, Zp1, 0); // 'Z' + 1
		mov(t, ('a' - 'A') * factor);
		movd(amA, t);
		pshufd(amA, amA, 0); // 'a' - 'A'
	}
	/*
		toLower in x
		Am1 : 'A' - 1
		Zp1 : 'Z' + 1
		amA : 'a' - 'A'
		t0, t1 : temporary register
	*/
	void toLower(const Xbyak::Xmm& x, const Xbyak::Xmm& Am1, const Xbyak::Xmm& Zp1, const Xbyak::Xmm& amA
		, const Xbyak::Xmm& t0, const Xbyak::Xmm& t1, bool isWcs)
	{
		if (isWcs) {
			movdqa(t0, x);
			pcmpgtw(t0, Am1); // -1 if c > 'A' - 1
			movdqa(t1, Zp1);
			pcmpgtw(t1, x); // -1 if 'Z' + 1 > c
			pand(t0, t1); // -1 if [A-Z]
			pand(t0, amA); // 0x20 if c in [A-Z]
			paddw(x, t0); // [A-Z] -> [a-z]
		} else {
			movdqa(t0, x);
			pcmpgtb(t0, Am1); // -1 if c > 'A' - 1
			movdqa(t1, Zp1);
			pcmpgtb(t1, x); // -1 if 'Z' + 1 > c
			pand(t0, t1); // -1 if [A-Z]
			pand(t0, amA); // 0x20 if c in [A-Z]
			paddb(x, t0); // [A-Z] -> [a-z]
		}
	}
	// char *strstr(str, key)
	// @note key must not have ascii capital characters[A-Z] if caseInsensitive is true
	// don't care about non ascii alphabet characters even if isWcs is true
	void gen_strstr(bool isSandyBridge, bool caseInsensitive = false, bool isWcs = false)
	{
		inLocalLabel();
		using namespace Xbyak;
		const Xmm& t0 = xm2;
		const Xmm& t1 = xm3;
		const Xmm& Am1 = xm4;
		const Xmm& Zp1 = xm5;
		const Xmm& amA = xm6;
#ifdef XBYAK64
	#ifdef XBYAK64_WIN
		const Reg64& key = rdx; // 2nd
		const Reg64& save_a = r11;
		const Reg64& save_key = r9;
		mov(rax, rcx); // 1st:str
		movdqa(ptr [rsp + 8], xm6);
	#else
		const Reg64& key = rsi; // 2nd
		const Reg64& save_a = r8;
		const Reg64& save_key = r9;
		mov(rax, rdi); // 1st:str
	#endif
		const Reg64& c = rcx;
		const Reg64& a = rax;
#else
		const Reg32& key = edx;
		const Reg32& save_a = esi;
		const Reg32& save_key = edi;
		const Reg32& c = ecx;
		const Reg32& a = eax;
		const int P_ = 4 * 2;
		sub(esp, P_);
		mov(ptr [esp + 0], esi);
		mov(ptr [esp + 4], edi);
		mov(eax, ptr [esp + P_ + 4]);
		mov(key, ptr [esp + P_ + 8]);
#endif
		if (caseInsensitive) {
			setLowerReg(Am1, Zp1, amA, Reg32(save_a.getIdx()), isWcs);
		}

		const int mode = isWcs ? 13 : 12; // 0b110? = [equal ordered:unsigned:(byte|word)]
		/*
			strstr(a, key);
			input key
			use save_a, save_key, c
		*/
		movdqu(xm0, ptr [key]); // xm0 = *key
	L(".lp");
		if (caseInsensitive) {
			movdqu(xm1, ptr [a]);
			toLower(xm1, Am1, Zp1, amA, t0, t1, isWcs);
			pcmpistri(xm0, xm1, mode);
		} else {
			pcmpistri(xm0, ptr [a], mode);
		}
		if (isSandyBridge) {
			lea(a, ptr [a + 16]);
			ja(".lp"); // if (CF == 0 and ZF = 0) goto .lp
		} else {
			jbe(".headCmp"); //  if (CF == 1 or ZF == 1) goto .headCmp
			add(a, 16);
			jmp(".lp");
	L(".headCmp");
		}
		jnc(".notFound");
		// get position
		if (isWcs) {
			if (isSandyBridge) {
				lea(a, ptr [a + c * 2 - 16]);
			} else {
				lea(a, ptr [a + c * 2]);
			}
		} else {
			if (isSandyBridge) {
				lea(a, ptr [a + c - 16]);
			} else {
				add(a, c);
			}
		}
		mov(save_a, a); // save a
		mov(save_key, key); // save key
	L(".tailCmp");
		if (caseInsensitive) {
			movdqu(xm1, ptr [save_a]);
			toLower(xm1, Am1, Zp1, amA, t0, t1, isWcs);
			movdqu(t0, ptr [save_key]);
			pcmpistri(t0, xm1, mode);
		} else {
			movdqu(xm1, ptr [save_key]);
			pcmpistri(xm1, ptr [save_a], mode);
		}
		jno(".next"); // if (OF == 0) goto .next
		js(".found"); // if (SF == 1) goto .found
		// rare case
		add(save_a, 16);
		add(save_key, 16);
		jmp(".tailCmp");
	L(".next");
		add(a, isWcs ? 2 : 1);
		jmp(".lp");
	L(".notFound");
		xor_(eax, eax);
	L(".found");
#ifdef XBYAK32
		mov(esi, ptr [esp + 0]);
		mov(edi, ptr [esp + 4]);
		add(esp, P_);
#elif defined(XBYAK64_WIN)
		movdqa(xm6, ptr [rsp + 8]);
#endif
		ret();
		outLocalLabel();
	}
	void gen_strlen(bool isWcs = false)
	{
		inLocalLabel();
		using namespace Xbyak;
#if defined(XBYAK64_WIN)
		const Reg64& p = rdx;
		const Reg64& c = rcx;
		const Reg64& a = rax;
		mov(rdx, rcx);
#elif defined(XBYAK64_GCC)
		const Reg64& p = rdi;
		const Reg64& c = rcx;
		const Reg64& a = rax;
#else
		const Reg32& p = edx;
		const Reg32& c = ecx;
		const Reg32& a = eax;
		mov(edx, ptr [esp + 4]);
#endif
		const unsigned int c1 = isWcs ? 0xffff0001 : 0xff01;
		const unsigned int c2 = isWcs ? 0x15 : 0x14;
		mov(eax, c1);
		movd(xm0, eax);

		mov(a, p);
		jmp(".in");
	L("@@");
		add(a, 16);
	L(".in");
		pcmpistri(xm0, ptr [a], c2);
		jnz("@b");
		sub(a, p);
		if (isWcs) shr(a, 1);
		add(a, c);
		ret();
		outLocalLabel();
	}
	void gen_strchr(int mode, bool isWcs = false)
	{
		inLocalLabel();
		using namespace Xbyak;

#ifdef XBYAK64
	#ifdef XBYAK64_WIN
		const Reg64& p = rcx;
		const Reg64& c1 = rdx;
	#else
		const Reg64& p = rdi;
		const Reg64& c1 = rsi;
	#endif
		const Reg64& c = rcx;
		const Reg64& a = rax;
		if (mode == M_one) {
			if (isWcs) {
				movzx(c1, c1.cvt16());
			} else {
				movzx(c1, c1.cvt8());
			}
			movq(xm0, c1);
		} else {
			movdqu(xm0, ptr [c1]);
		}
		mov(a, p);
#else
		const Reg32& a = eax;
		const Reg32& c = ecx;
		if (mode == M_one) {
			if (isWcs) {
				movzx(eax, word [esp + 8]);
			} else {
				movzx(eax, byte [esp + 8]);
			}
			movd(xm0, eax);
		} else {
			mov(eax, ptr [esp + 8]);
			movdqu(xm0, ptr [eax]);
		}
		mov(a, ptr [esp + 4]);
#endif
		const int v = (mode == M_range ? 4 : 0) + (isWcs ? 1 : 0);
		jmp(".in");
	L("@@");
		add(a, 16);
	L(".in");
		pcmpistri(xm0, ptr [a], v);
		ja("@b");
		jnc(".notfound");
		if (isWcs) {
			lea(a, ptr [a + c * 2]);
		} else {
			add(a, c);
		}
		ret();
	L(".notfound");
		xor_(a, a);
		ret();
		outLocalLabel();
	}
	void gen_findChar(int mode, bool isWcs = false)
	{
		// mode = M_one ; findChar(p(=begin), end, c)
		// mode = M_any/M_range ; findChar(p(=begin), end, key, keySize)
		inLocalLabel();
		using namespace Xbyak;

#ifdef XBYAK64
	#ifdef XBYAK64_WIN
		const Reg64& p = r9;
		const Reg64& end = r10;
		if (mode == M_one) {
			if (isWcs) {
				movzx(r8d, r8w); // c:3rd
			} else {
				movzx(r8d, r8b); // c:3rd
			}
			movq(xm0, r8);
		} else {
			movdqu(xm0, ptr [r8]); // key:3rd
			mov(rax, r9); // keySize:4th
		}
		mov(p, rcx); // 1st
		mov(end, rdx); // 2nd
	#else
		const Reg64& p = rdi; // 1st
		const Reg64& end = rsi; // 2nd
		if (mode == M_one) {
			if (isWcs) {
				movzx(edx, dx); // c:3rd
			} else {
				movzx(edx, dl); // c:3rd
			}
			movq(xm0, rdx);
		} else {
			movdqu(xm0, ptr [rdx]); // key:3rd
			mov(rax, rcx); // keySize:4th
		}
	#endif
		const Reg64& a = rax;
		const Reg64& c = rcx;
		const Reg64& d = rdx;
#else
		const Reg32& p = esi;
		const Reg32& end = edi;
		const Reg32& a = eax;
		const Reg32& c = ecx;
		const Reg32& d = edx;
		push(esi);
		push(edi);
		const int P_ = 4 * 2;
		mov(p, ptr [esp + P_ + 4]);
		mov(end, ptr [esp + P_ + 8]);
		if (mode == M_one) {
			if (isWcs) {
				movzx(eax, word [esp + P_ + 12]);
			} else {
				movzx(eax, byte [esp + P_ + 12]);
			}
			movd(xm0, eax);
		} else {
			mov(eax, ptr [esp + P_ + 12]);
			movdqu(xm0, ptr [eax]);
			mov(eax, ptr [esp + P_ + 16]);
		}
#endif
		if (mode == M_one) {
			xor_(eax, eax);
			inc(eax); // eax = 1
		}
		/*
			input  : [p, end), xm0 : char(key)
			output : a
			destroy: a, c, d, p
		*/
		const int v = (mode == M_range ? 4 : 0) + (isWcs ? 1 : 0);
		mov(d, end);
		sub(d, p); // len
		if (isWcs) shr(d, 1);
		jmp(".in");
	L("@@");
		add(p, 16);
		sub(d, isWcs ? 8 : 16);
	L(".in");
		pcmpestri(xm0, ptr [p], v);
		ja("@b");
		jnc(".notfound");
		if (isWcs) {
			lea(a, ptr [p + c * 2]);
		} else {
			lea(a, ptr [p + c]);
		}
#ifdef XBYAK32
		pop(edi);
		pop(esi);
#endif
		ret();
	L(".notfound");
		mov(a, end);
#ifdef XBYAK32
		pop(edi);
		pop(esi);
#endif
		ret();
		outLocalLabel();
	}
	// findStr(const char*begin, const char *end, const char *key, size_t keySize)
	void gen_findStr(bool /*isSandyBridge*/, bool caseInsensitive = false, bool isWcs = false)
	{
		inLocalLabel();
		using namespace Xbyak;
		const Xmm& t0 = xm2;
		const Xmm& t1 = xm3;
		const Xmm& Am1 = xm4;
		const Xmm& Zp1 = xm5;
		const Xmm& amA = xm6;
#ifdef XBYAK64
	#ifdef XBYAK64_WIN
		const Reg64& p = r9;
		const Reg64& key = r8;
		const Reg64& save_p = r10;
		const Reg64& save_key = r11;
		const Reg64& save_a = r12;
		const Reg64& save_d = r13;
		const Address end = ptr [rsp + 24];
		mov(ptr [rsp + 8], r12);
		mov(ptr [rsp + 16], r13);
		if (caseInsensitive) {
			movdqa(ptr [rsp + 24], xm6);
		}
		mov(end, rdx); // save end:2nd
		mov(rax, r9); // keySize:4th
		sub(rdx, rcx); // len = end - begin(1st)
		mov(p, rcx);
	#else
		const Reg64& p = rdi; // begin::1st
		const Reg64& key = r8;
		const Reg64& save_p = r9;
		const Reg64& save_key = r10;
		const Reg64& save_a = r11;
		const Reg64& save_d = r12;
		const Reg64& end = rsi; // end::2nd
		mov(key, rdx);
		mov(rax, rcx); // keySize:4th
		mov(rdx, end);
		sub(rdx, p); // len = end - p
		push(r12);
	#endif
		const Reg64& a = rax;
		const Reg64& c = rcx;
		const Reg64& d = rdx;
#else
		const Reg32& p = esi;
		const Reg32& key = edi;
		const Reg32& save_p = ebx;
		const Reg32& save_key = ebp;
		const Reg32& a = eax;
		const Reg32& c = ecx;
		const Reg32& d = edx;
		const int P_ = 4 * 6;
		const Address save_d = ptr [esp + 16];
		const Address save_a = ptr [esp + 20];
		const Address end = ptr [esp + P_ + 8];
		sub(esp, P_);
		mov(ptr [esp + 0], esi);
		mov(ptr [esp + 4], edi);
		mov(ptr [esp + 8], ebx);
		mov(ptr [esp + 12], ebp);
		mov(p, ptr [esp + P_ + 4]); // p
		mov(edx, end); // end
		sub(edx, p); // len = end - p
		mov(key, ptr [esp + P_+ 12]); // key
		movdqu(xm0, ptr [key]);
		mov(eax, ptr [esp + P_ + 16]); // keySize
#endif
        if (isWcs) shr(d, 1);
		if (caseInsensitive) {
			setLowerReg(Am1, Zp1, amA, Reg32(save_p.getIdx()), isWcs);
		}

		const int mode = isWcs ? 13 : 12; // 0b110? = [equal ordered:unsigned:(byte|word)]
		const int charLen = isWcs ? 2 : 1;
		/*
			findStr(p, end, key, keySize)
			input  : p, d:=len=end-p, xm0:key, a:keySize
			destroy: save_p, save_d, save_key, save_keySize, a, c, d
		*/
		movdqu(xm0, ptr [key]);
	L(".lp");
		if (caseInsensitive) {
			movdqu(xm1, ptr [p]);
			toLower(xm1, Am1, Zp1, amA, t0, t1, isWcs);
			pcmpestri(xm0, xm1, mode);
		} else {
			pcmpestri(xmm0, ptr [p], mode); // 12(1100b) = [equal ordered:unsigned:byte]
		}
		lea(p, ptr [p + 16]);
		lea(d, ptr [d - (isWcs ? 8 : 16)]);
		ja(".lp"); // if (CF == 0 and ZF = 0) goto .lp
		jnc(".notFound");
		// get position
		if (isWcs) {
    		lea(p, ptr [p + c * 2 - 16]);
        } else {
    		lea(p, ptr [p + c - 16]);
        }
		sub(d, c);
		add(d, isWcs ? 8 : 16);
		mov(save_p, p);
		mov(save_key, key);
		mov(save_a, a);
		mov(save_d, d);
	L(".tailCmp");
		if (caseInsensitive) {
			movdqu(t0, ptr [save_p]);
			toLower(t0, Am1, Zp1, amA, xm1, t1, isWcs);
			movdqu(xm1, ptr [save_key]);
			pcmpestri(xm1, t0, mode);
		} else {
			movdqu(xm1, ptr [save_key]);
			pcmpestri(xm1, ptr [save_p], mode);
		}
		jno(".next"); // if (OF == 0) goto .next
		js(".found"); // if (SF == 1) goto .found
		// rare case
		add(save_p, 16);
		add(save_key, 16);
		sub(a, isWcs ? 8 : 16);
		sub(d, isWcs ? 8 : 16);
		jmp(".tailCmp");
	L(".next");
		add(p, charLen);
		mov(a, save_a);
#ifdef XBYAK32
		mov(edx, save_d);
		sub(edx, 1);
#else
		lea(d, ptr [save_d - 1]);
#endif
		jmp(".lp");
	L(".notFound");
		mov(a, end);
		jmp(".exit");
	L(".found");
		mov(a, p);
	L(".exit");
#ifdef XBYAK64
	#ifdef XBYAK64_WIN
		mov(r12, ptr [rsp + 8]);
		mov(r13, ptr [rsp + 16]);
		movdqa(ptr [rsp + 24], xm6);
	#else
		pop(r12);
	#endif
#else
		mov(esi, ptr [esp + 0]);
		mov(edi, ptr [esp + 4]);
		mov(ebx, ptr [esp + 8]);
		mov(ebp, ptr [esp + 12]);
		add(esp, P_);
#endif
		ret();
		outLocalLabel();
	}
};

template<int dummy = 0>
struct InstanceIsHere {
	static MIE_ALIGN(4096) char buf[4096];
	static StringCode code;
};

template<int dummy>
StringCode InstanceIsHere<dummy>::code(buf, sizeof(buf));

template<int dummy>
char InstanceIsHere<dummy>::buf[4096];

struct DummyCall {
	DummyCall() { InstanceIsHere<>::code.getCode(); }
};

} // str_util_impl

inline bool isAvailableSSE42()
{
	return str_util_impl::InstanceIsHere<>::code.cpu.has(Xbyak::util::Cpu::tSSE42);
}
///////////////////////////////////////////////////////////////////////////////////////
// functions like C
// const version of strstr
inline const char *strstr(const char *str, const char *key)
{
	return Xbyak::CastTo<const char*(*)(const char*, const char*)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strstrOffset)(str, key);
}

// non const version of strstr
inline char *strstr(char *str, const char *key)
{
	return Xbyak::CastTo<char*(*)(char*, const char*)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strstrOffset)(str, key);
}

// functions like C
// const version of strstr16
inline const MIE_CHAR16 *strstr16(const MIE_CHAR16 *str, const MIE_CHAR16 *key)
{
	return Xbyak::CastTo<const MIE_CHAR16*(*)(const MIE_CHAR16*, const MIE_CHAR16*)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strstr16Offset)(str, key);
}

// non const version of strstr16
inline MIE_CHAR16 *strstr16(MIE_CHAR16 *str, const MIE_CHAR16 *key)
{
	return Xbyak::CastTo<MIE_CHAR16*(*)(MIE_CHAR16*, const MIE_CHAR16*)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strstr16Offset)(str, key);
}

// const version of strchr(c != 0)
inline const char *strchr(const char *str, int c)
{
	return Xbyak::CastTo<const char*(*)(const char*, int)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchrOffset)(str, c);
}

// non const version of strchr(c != 0)
inline char *strchr(char *str, int c)
{
	return Xbyak::CastTo<char*(*)(char*, int)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchrOffset)(str, c);
}

// const version of strchr16(c != 0)
inline const MIE_CHAR16 *strchr16(const MIE_CHAR16 *str, int c)
{
	return Xbyak::CastTo<const MIE_CHAR16*(*)(const MIE_CHAR16*, int)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchr16Offset)(str, c);
}

// non const version of strchr16(c != 0)
inline MIE_CHAR16 *strchr16(MIE_CHAR16 *str, int c)
{
	return Xbyak::CastTo<MIE_CHAR16*(*)(MIE_CHAR16*, int)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchr16Offset)(str, c);
}

inline size_t strlen(const char *str)
{
	return Xbyak::CastTo<size_t(*)(const char*)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strlenOffset)(str);
}

inline size_t strlen16(const MIE_CHAR16 *str)
{
	return Xbyak::CastTo<size_t(*)(const MIE_CHAR16*)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strlen16Offset)(str);
}

/*
	find key[0] or key[1], ... in str
	@note strlen(key) <= 16, key[i] != 0
*/
inline const char *strchr_any(const char *str, const char *key)
{
	return Xbyak::CastTo<const char *(*)(const char*, const char *key)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchr_anyOffset)(str, key);
}
inline char *strchr_any(char *str, const char *key)
{
	return Xbyak::CastTo<char *(*)(char*, const char *key)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchr_anyOffset)(str, key);
}

/*
	find key[0] or key[1], ... in str
	@note strlen16(key) <= 8, key[i] != 0
*/
inline const MIE_CHAR16 *strchr16_any(const MIE_CHAR16 *str, const MIE_CHAR16 *key)
{
	return Xbyak::CastTo<const MIE_CHAR16 *(*)(const MIE_CHAR16*, const MIE_CHAR16 *key)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchr16_anyOffset)(str, key);
}
inline MIE_CHAR16 *strchr16_any(MIE_CHAR16 *str, const MIE_CHAR16 *key)
{
	return Xbyak::CastTo<MIE_CHAR16 *(*)(MIE_CHAR16*, const MIE_CHAR16 *key)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchr16_anyOffset)(str, key);
}

/*
	find c such that key[0] <= c && c <= key[1], key[2] <= c && c <= key[3], ... in str
	@note strlen(key) <= 16, key[i] != 0
*/
inline const char *strchr_range(const char *str, const char *key)
{
	return Xbyak::CastTo<const char *(*)(const char*, const char *key)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchr_rangeOffset)(str, key);
}
inline char *strchr_range(char *str, const char *key)
{
	return Xbyak::CastTo<char *(*)(char*, const char *key)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchr_rangeOffset)(str, key);
}

/*
	find c such that key[0] <= c && c <= key[1], key[2] <= c && c <= key[3], ... in str
	@note strlen16(key) <= 8, key[i] != 0
*/
inline const MIE_CHAR16 *strchr16_range(const MIE_CHAR16 *str, const MIE_CHAR16 *key)
{
	return Xbyak::CastTo<const MIE_CHAR16 *(*)(const MIE_CHAR16*, const MIE_CHAR16 *key)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchr16_rangeOffset)(str, key);
}
inline MIE_CHAR16 *strchr16_range(MIE_CHAR16 *str, const MIE_CHAR16 *key)
{
	return Xbyak::CastTo<MIE_CHAR16 *(*)(MIE_CHAR16*, const MIE_CHAR16 *key)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strchr16_rangeOffset)(str, key);
}

/*
	find c in [begin, end)
	if c is not found then return end
*/
inline const char *findChar(const char *begin, const char *end, char c)
{
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<const char *(*)(const char*, const char *, char c)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findCharOffset)(begin, end, c);
}
inline char *findChar(char *begin, const char *end, char c)
{
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<char *(*)(char*, const char *, char c)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findCharOffset)(begin, end, c);
}

/*
	find c in [begin, end)
	if c is not found then return end
*/
inline const MIE_CHAR16 *findChar16(const MIE_CHAR16 *begin, const MIE_CHAR16 *end, MIE_CHAR16 c)
{
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<const MIE_CHAR16 *(*)(const MIE_CHAR16*, const MIE_CHAR16 *, MIE_CHAR16 c)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findChar16Offset)(begin, end, c);
}
inline MIE_CHAR16 *findChar16(MIE_CHAR16 *begin, const MIE_CHAR16 *end, MIE_CHAR16 c)
{
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<MIE_CHAR16 *(*)(MIE_CHAR16*, const MIE_CHAR16 *, MIE_CHAR16 c)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findChar16Offset)(begin, end, c);
}

/*
	find any of key[0..keySize - 1] in [begin, end)
	if char is not found then return end
	@note keySize <= 16
*/
inline const char *findChar_any(const char *begin, const char *end, const char *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<const char *(*)(const char*, const char *, const char *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findChar_anyOffset)(begin, end, key, keySize);
}
inline char *findChar_any(char *begin, const char *end, const char *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<char *(*)(char*, const char *, const char *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findChar_anyOffset)(begin, end, key, keySize);
}

/*
	find any of key[0..keySize - 1] in [begin, end)
	if char is not found then return end
	@note keySize <= 8
*/
inline const MIE_CHAR16 *findChar16_any(const MIE_CHAR16 *begin, const MIE_CHAR16 *end, const MIE_CHAR16 *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<const MIE_CHAR16 *(*)(const MIE_CHAR16*, const MIE_CHAR16 *, const MIE_CHAR16 *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findChar16_anyOffset)(begin, end, key, keySize);
}
inline MIE_CHAR16 *findChar16_any(MIE_CHAR16 *begin, const MIE_CHAR16 *end, const MIE_CHAR16 *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<MIE_CHAR16 *(*)(MIE_CHAR16*, const MIE_CHAR16 *, const MIE_CHAR16 *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findChar16_anyOffset)(begin, end, key, keySize);
}

/*
	find character in range [key[0]..key[1]], [key[2]..key[3]], ... in [begin, end)
	if char is not found then return end
	@note keySize <= 16
*/
inline const char *findChar_range(const char *begin, const char *end, const char *key, size_t keySize)
{
	assert((keySize % 2) == 0);
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<const char *(*)(const char*, const char *, const char *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findChar_rangeOffset)(begin, end, key, keySize);
}
inline char *findChar_range(char *begin, const char *end, const char *key, size_t keySize)
{
	assert((keySize % 2) == 0);
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<char *(*)(char*, const char *, const char *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findChar_rangeOffset)(begin, end, key, keySize);
}

/*
	find character in range [key[0]..key[1]], [key[2]..key[3]], ... in [begin, end)
	if char is not found then return end
	@note keySize <= 8
*/
inline const MIE_CHAR16 *findChar16_range(const MIE_CHAR16 *begin, const MIE_CHAR16 *end, const MIE_CHAR16 *key, size_t keySize)
{
	assert((keySize % 2) == 0);
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<const MIE_CHAR16 *(*)(const MIE_CHAR16*, const MIE_CHAR16 *, const MIE_CHAR16 *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findChar16_rangeOffset)(begin, end, key, keySize);
}
inline MIE_CHAR16 *findChar16_range(MIE_CHAR16 *begin, const MIE_CHAR16 *end, const MIE_CHAR16 *key, size_t keySize)
{
	assert((keySize % 2) == 0);
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<MIE_CHAR16 *(*)(MIE_CHAR16*, const MIE_CHAR16 *, const MIE_CHAR16 *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findChar16_rangeOffset)(begin, end, key, keySize);
}

/*
	find [key, key + keySize) in [begin, end)
*/
inline const char *findStr(const char*begin, const char *end, const char *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<const char *(*)(const char*, const char *, const char *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findStrOffset)(begin, end, key, keySize);
}
inline char *findStr(char*begin, const char *end, const char *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<char *(*)(char*, const char *, const char *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findStrOffset)(begin, end, key, keySize);
}

/*
	find [key, key + keySize) in [begin, end)
*/
inline const MIE_CHAR16 *findStr16(const MIE_CHAR16*begin, const MIE_CHAR16 *end, const MIE_CHAR16 *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<const MIE_CHAR16 *(*)(const MIE_CHAR16*, const MIE_CHAR16 *, const MIE_CHAR16 *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findStr16Offset)(begin, end, key, keySize);
}
inline MIE_CHAR16 *findStr16(MIE_CHAR16*begin, const MIE_CHAR16 *end, const MIE_CHAR16 *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<MIE_CHAR16 *(*)(MIE_CHAR16*, const MIE_CHAR16 *, const MIE_CHAR16 *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findStr16Offset)(begin, end, key, keySize);
}

/*
	case insensitive strstr
	@note key must not have capital characters [A-Z]
*/
inline const char *strcasestr(const char *str, const char *key)
{
	if (*str == 0) return 0;
	return Xbyak::CastTo<const char*(*)(const char*, const char*)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strcasestrOffset)(str, key);
}

// non const version of strstr
inline char *strcasestr(char *str, const char *key)
{
	if (*str == 0) return 0;
	return Xbyak::CastTo<char*(*)(char*, const char*)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strcasestrOffset)(str, key);
}
/*
	case insensitive strstr
	@note key must not have capital MIE_CHAR16acters [A-Z]
*/
inline const MIE_CHAR16 *strcasestr16(const MIE_CHAR16 *str, const MIE_CHAR16 *key)
{
	if (*str == 0) return 0;
	return Xbyak::CastTo<const MIE_CHAR16*(*)(const MIE_CHAR16*, const MIE_CHAR16*)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strcasestr16Offset)(str, key);
}

// non const version of strstr
inline MIE_CHAR16 *strcasestr16(MIE_CHAR16 *str, const MIE_CHAR16 *key)
{
	if (*str == 0) return 0;
	return Xbyak::CastTo<MIE_CHAR16*(*)(MIE_CHAR16*, const MIE_CHAR16*)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::strcasestr16Offset)(str, key);
}
/*
	case insensitive find [key, key + keySize) in [begin, end)
	@note key must not have capital characters [A-Z]
*/
inline const char *findCaseStr(const char*begin, const char *end, const char *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<const char *(*)(const char*, const char *, const char *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findCaseStrOffset)(begin, end, key, keySize);
}
inline char *findCaseStr(char*begin, const char *end, const char *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<char *(*)(char*, const char *, const char *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findCaseStrOffset)(begin, end, key, keySize);
}
/*
	case insensitive find [key, key + keySize) in [begin, end)
	@note key must not have capital characters [A-Z]
*/
inline const MIE_CHAR16 *findCaseStr16(const MIE_CHAR16 *begin, const MIE_CHAR16 *end, const MIE_CHAR16 *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<const MIE_CHAR16 *(*)(const MIE_CHAR16*, const MIE_CHAR16 *, const MIE_CHAR16 *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findCaseStr16Offset)(begin, end, key, keySize);
}
inline MIE_CHAR16 *findCaseStr16(MIE_CHAR16 *begin, const MIE_CHAR16 *end, const MIE_CHAR16 *key, size_t keySize)
{
	if (keySize == 0) {
		return begin;
	}
	if (begin == end) {
		return begin;
	}
	return Xbyak::CastTo<MIE_CHAR16 *(*)(MIE_CHAR16*, const MIE_CHAR16 *, const MIE_CHAR16 *,size_t)>(str_util_impl::InstanceIsHere<>::buf + str_util_impl::findCaseStr16Offset)(begin, end, key, keySize);
}

} // mie

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
