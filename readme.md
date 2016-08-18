# mie_string

Functions to find charaters written by SSE 4.2

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
