#pragma once

#include <string>
#include <vector>

#define TEST_EQUAL(a, b) { if ((a) != (b)) { fprintf(stderr, "%s:%d err a=%lld, b=%lld\n", __FILE__, __LINE__, (long long)(a), (long long)(b)); exit(1); } }

typedef std::vector<std::string> StrVec;

struct Ret {
	int val;
	double clk;
	Ret() : val(0), clk(0) {}
	bool operator!=(const Ret& rhs) const { return val != rhs.val; }
	operator long long() const { return val; }
};

template<class C, const C* (*f)(const C *str, const C *key)>
struct FstrstrT {
	const C *str_;
	const C *key_;
	typedef const C* type;
	void set(const std::basic_string<C>& str, const std::basic_string<C>& key)
	{
		str_ = &str[0];
		key_ = &key[0];
	}
	FstrstrT() : str_(0), key_(0) { }
	const C *begin() const { return str_; }
	const C *end() const { return 0; }
	const C *find(const C *p) const { return f(p, key_); }
};

template<const char* (*f)(const char*str, const char *key)>
struct Fstrstr : FstrstrT<char, f> {};

template<const MIE_CHAR16* (*f)(const MIE_CHAR16*str, const MIE_CHAR16 *key)>
struct Fstrstr16 : FstrstrT<MIE_CHAR16, f> {};

template<class C, const C* (*f)(const C*str, int c)>
struct FstrchrT {
	const C *str_;
	int c_;
	typedef const C* type;
	void set(const std::basic_string<C>& str, const std::basic_string<C>& key)
	{
		str_ = &str[0];
		c_ = key[0];
	}
	FstrchrT() : str_(0), c_(0) { }
	const C *begin() const { return str_; }
	const C *end() const { return 0; }
	const C *find(const C *p) const { return f(p, c_); }
};

template<const char* (*f)(const char*str, int c)>
struct Fstrchr : FstrchrT<char, f> {};

template<const MIE_CHAR16* (*f)(const MIE_CHAR16*str, int c)>
struct Fstrchr16 : FstrchrT<MIE_CHAR16, f> {};

template<class C, const C* (*f)(const C*begin, const C *end, const C *key, size_t size)>
struct FrangeT {
	const C *str_;
	const C *end_;
	const C *key_;
	size_t keySize_;
	typedef const C* type;
	void set(const std::basic_string<C>& str, const std::basic_string<C>& key)
	{
		str_ = &str[0];
		end_ = str_ + str.size();
		key_ = &key[0];
		keySize_ = key.size();
	}
	FrangeT() : str_(0), end_(0), key_(0), keySize_(0) { }
	const C *begin() const { return str_; }
	const C *end() const { return end_; }
	const C *find(const C *p) const { return f(p, end_, key_, keySize_); }
};

template<const char* (*f)(const char*begin, const char *end, const char *key, size_t size)>
struct Frange : FrangeT<char, f> {};

template<const MIE_CHAR16* (*f)(const MIE_CHAR16*begin, const MIE_CHAR16 *end, const MIE_CHAR16 *key, size_t size)>
struct Frange16 : FrangeT<MIE_CHAR16, f> {};

template<class C, const C* (*f)(const C*begin, const C *end, C c)>
struct Frange_char_T {
	const C *str_;
	const C *end_;
	C c_;
	size_t keySize_;
	typedef const C* type;
	void set(const std::basic_string<C>& str, const std::basic_string<C>& key)
	{
		str_ = &str[0];
		end_ = str_ + str.size();
		c_ = key[0];
	}
	Frange_char_T() : str_(0), end_(0), c_(0) { }
	const C *begin() const { return str_; }
	const C *end() const { return end_; }
	const C *find(const C *p) const { return f(p, end_, c_); }
};

template<const char* (*f)(const char*begin, const char *end, char c)>
struct Frange_char : Frange_char_T<char, f> {};

template<const MIE_CHAR16* (*f)(const MIE_CHAR16*begin, const MIE_CHAR16 *end, MIE_CHAR16 c)>
struct Frange_char16 : Frange_char_T<MIE_CHAR16, f> {};

template<class C, class F>
Ret benchmark1(const std::basic_string<C>& str, const std::basic_string<C>& key, F f)
{
	const int N = 1;
	int val = 0;
	f.set(str, key);
	Xbyak::util::Clock clk;

	for (int i = 0; i < N; i++) {
		typename F::type p = f.begin();
		typename F::type end = f.end();
		for (;;) {
			clk.begin();
			typename F::type q = f.find(p);
			clk.end();

			if (q == end) break;
			val += (int)(q - p);
			p = q + 1;
		}
	}
 if (val == 0) val = (int)(str.size()) * N;
	Ret ret;
	ret.val = val;
	ret.clk = clk.getClock() / (double)val;
	return ret;
}

inline std::string u16tos(const std::basic_string<MIE_CHAR16>& str)
{
	std::string ret;
	for (size_t i = 0; i < str.size(); i++) {
		ret += (char)str[i];
	}
	return ret;
}

inline std::string u16tos(const std::string& str) { return str; }

inline std::basic_string<MIE_CHAR16> stou16(const std::string& str)
{
	std::basic_string<MIE_CHAR16> ret;
	for (size_t i = 0; i < str.size(); i++) {
		ret += (MIE_CHAR16)str[i];
	}
	return ret;
}
template<class C, class F1, class F2>
void benchmarkT(const char *msg1, F1 f1, const char *msg2, F2 f2, const std::basic_string<C>& str, const std::basic_string<C>& key)
{
	Ret r1 = benchmark1(str, key, f1);
	Ret r2 = benchmark1(str, key, f2);
	printf("%25s %16s % 6.2f %16s % 6.2f %5.2f\n", u16tos(key.substr(0, 25)).c_str(), msg1, r1.clk, msg2, r2.clk, r1.clk / r2.clk);
	TEST_EQUAL(r1, r2);
}

template<class F1, class F2>
void benchmark(const char *msg1, F1 f1, const char *msg2, F2 f2, const std::string& str, const std::string& key)
{
	benchmarkT<char, F1, F2>(msg1, f1, msg2, f2, str, key);
}

template<class F1, class F2>
void benchmark16(const char *msg1, F1 f1, const char *msg2, F2 f2, const std::basic_string<MIE_CHAR16>& str, const std::basic_string<MIE_CHAR16>& key)
{
	benchmarkT<MIE_CHAR16, F1, F2>(msg1, f1, msg2, f2, str, key);
}

template<class F1, class F2>
void benchmarkTbl(const char *msg1, F1 f1, const char *msg2, F2 f2, const std::string& str, const StrVec& keyTbl)
{
	for (size_t i = 0; i < keyTbl.size(); i++) {
		benchmark(msg1, f1, msg2, f2, str, keyTbl[i]);
	}
}

template<class F1, class F2, class C, class Key>
void verify(F1 f1, F2 f2, const std::basic_string<C>& str, const Key& key)
{
	f1.set(str, key);
	f2.set(str, key);

	typename F1::type p = f1.begin();
	typename F1::type end = f1.end();
	for (;;) {
		typename F1::type q1 = f1.find(p);
		typename F2::type q2 = f2.find(p);
		TEST_EQUAL(q1, q2);
		if (q1 == end) break;
		p = q1 + 1;
	}
}

// dirty hack for gcc 4.2 on mac
#if defined(__APPLE__) || defined(__clang__)
	#define STRSTR mystrstr
	#define STRCHR mystrchr
inline const char *mystrstr(const char*text, const char *key)
{
	return strstr(text, key);
}
inline const char *mystrchr(const char*text, int c)
{
	return strchr(text, c);
}

#else
	#define STRSTR strstr
	#define STRCHR strchr
#endif
