# mie_string

Functions to find characters by SSE 4.2

# Supported CPU and OS

* Intel 64-bit Core i or later
* 64-bit Linux and 64-bit Windows

# test

```
mkdir work
cd work
git clone git://github.com/herumi/mie_string.git
git clone git://github.com/herumi/cybozulib.git
cd mie_string
g++ mie_string_test.cpp -Ofast -march=native -I ../../cybozulib/include
./a.out <text file>
```

# How to use

* Linux
```
nasm -f elf64 mie_string_x86.asm
```
Include `mie_string.h` and link `mie_string_x86.o`.

* Windows
```
nasm -f win64 -D_WIN64 mie_string_x86.asm
```
Include `mie_string.h` and link `mie_string_x86.obj`.

* Use intrinsic
```
#define MIE_STRING_INLINE
#include "mie_string.h"
```
It is easy to use, but a little slower than using `mie_string_x86.asm`.


# API

```
/*
  find any char of key[0], ..., key[keySize - 1] in [p, p + size) and return the pointer
  if not found then return NULL
  @note (0 < keySize) && (keySize <= 16)
*/
const char *mie_findCharAny(const char *p, size_t size, const char *key, size_t keySize);
```

```
/*
  find character in range [key[0]..key[1]], [key[2]..key[3]], ... in [p, p + size) and return the pointer
  if not found then return NULL
  @note (0 < keySize) && (keySize <= 16) && ((keySize % 2) == 0)
*/
const char *mie_findCharRange(const char *p, size_t size, const char *key, size_t keySize);
```

# Sample

* To find '\r', '\n', ' ', '\t'
```
mie_findCharAny(text, size, "\r\n \t", 4);
```

* To find alphabet(a-zA-Z) and number(0-9)
```
mie_findCharRange(text, size, "azAZ09", 6);
```

# License

modified new BSD License
http://opensource.org/licenses/BSD-3-Clause

# Author

MITSUNARI Shigeo(herumi@nifty.com)
