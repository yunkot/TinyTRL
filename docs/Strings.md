# Strings

TinyTRL provides two text classes in the `trl` namespace:

* **`String`** — an ASCII/UTF-8 string with **Short String Optimization (SSO)** and the ability to **wrap** an existing C string without copying it.
* **`WideString`** — a UTF-16 string, provided mainly for calling the Windows API, with automatic conversion to/from `String`.

Owned strings are always null-terminated, so `data()` always points to a valid C string. A *wrapped* string borrows both the characters **and** the null terminator from the caller's buffer, which therefore must be null-terminated and must outlive the wrapper (see [Wrapping](#wrapping-an-existing-c-string-zero-copy)). Both classes follow TinyTRL's [error handling](Error_Handling.md) model: on a failed allocation the object becomes *polluted* and can be tested with `if (!string)`.

```cpp
#include "TinyTRL.h"
using namespace trl;
```

---

## `String`

### Short String Optimization (SSO)

`String` is a small, fixed-size object that stores **short** strings *inline* in its own storage, avoiding any heap allocation. Long strings are stored on the heap.

* On **64-bit** platforms the inline area holds up to **22 characters** plus the null terminator (24 bytes total).
* On **32-bit** platforms the inline area holds up to **10 characters** plus the null terminator (12 bytes total).

Because short strings live on the stack, building them costs no `malloc`/`free`. This makes `String` safe even when memory is tight: operations on short strings essentially cannot fail. The example in the [`Arrays` sample](../examples/src/Arrays.cpp) notes that short names "will be stored directly inside the string and no heap allocation would occur."

```cpp
String a = "tiny";        // short: held inline, no allocation
String b = String::Fill(10, ' ');  // still short: no allocation
String c = "a much longer sentence that cannot fit inline";  // long: heap allocation
```

You can query the current capacity (`String::capacity()`) and length (`String::length()`). A short string reports the inline capacity (22 on x64, 10 on x32); a long string reports its allocated capacity minus the byte reserved for the null terminator; a wrapped string reports `0`.

### Creating strings

| Call | Behavior |
|---|---|
| `String()` | Empty string (default, no allocation). |
| `String(char const*)` | Copies a null-terminated C string into a `String`. |
| `String(char8_t const*)` | Same, for `char8_t` (C++20 and later builds only). |
| `String(WideString const&)` `explicit` | Converts a UTF-16 `WideString` to UTF-8 (see [Converting](#converting-string-and-widestring)). |
| `String(char)` `explicit` | A single character. |
| `String(String const&)` / `String(String&&)` | Copy / move. |
| `String::Fill(n, fill=0)` | `n` copies of a character (e.g. `String::Fill(8, '.')`). |
| `String::FromBuffer(buf, n)` | Copy `n` characters from a (possibly not null-terminated) buffer. |
| `String::FromRawBytes(buf, n)` | Copy `n` raw bytes (binary-safe). |
| `String::Wrap(string, length=0)` | Borrow an existing null-terminated C string (see [Wrapping](#wrapping-an-existing-c-string-zero-copy)). |
| `String::Invalid()` | Returns an empty, already-polluted string. |
| `String::Empty` | A static empty string constant. |

Two constants are also defined: `String::MaxLength` (the maximum representable string length) and `String::NotFound` (`-1`, the "not found" index returned by the search functions).

Mutating operations (`append`, `prepend`, `concatenate`, `insert`, `erase`, `replace`, `substr`, `copy`, `assign`, and the `+` / `+=` operators) can all trigger an allocation when the result no longer fits inline. Each will set the error flag on failure — test the result with `if (!string)`.

```cpp
String greeting = "hello";
greeting += ", world";          // may allocate if it overflows the inline area
greeting = "Hi " + greeting;    // concatenation via operator +

if (!greeting)
  printf("Concatenation failed (likely out of memory).\n");
else
  printf("%s\n", greeting.data());
```

### Member operations

Besides the mutating operations above, `String` offers:

| Member | Description |
|---|---|
| `empty()` | `true` if the string has zero characters. |
| `wrapped()` | `true` if the string is a borrowed view (see [Wrapping](#wrapping-an-existing-c-string-zero-copy)). |
| `length(n)` | Change the length to `n`. Returns `false` on an invalid range or a failed allocation, leaving the string unchanged (it does not pollute). |
| `capacity(n)` | Grow the capacity to at least `n`. Returns `false` on an invalid range or a failed allocation. Never shrinks. |
| `assign(source)` | Replace the contents with those of `source`. Returns `false` on allocation failure, leaving the current string unchanged; on success it propagates the source's polluted status. |
| `shrink()` | Reallocate a long string to fit its contents, or move it back into the inline area. Returns `false` for a wrapped string or on failure. |
| `store(dest, destLen)` | Copy up to `destLen` characters into a raw buffer, zero-filling any remainder. Returns the number of characters actually copied. |
| `insert(ch, position)` | Insert a single character at `position`. Returns `true` on success; on allocation failure it returns `false` without polluting. |
| `unpollute()` | Clear the error flag, removing the polluted status. |
| `clear()` | Set the length to zero and clear the error flag, preserving the capacity of a long string. |
| `burn()` | Securely erase the contents and reset the length. Returns `false` for a wrapped string, which it cannot touch. |

Six free relational operators compare two `String` values lexicographically: `==`, `!=`, `<`, `>`, `<=`, `>=`. (For case-insensitive comparison see `utility::compareText` / `utility::TextComparer` below.)

---

### Wrapping an existing C string (zero-copy)

`String::Wrap()` is the mechanism that lets a `String` *borrow* an existing null-terminated C string instead of copying it:

```cpp
String String::Wrap(char const* string, Length length = 0);
```

* If `length` is `0` it is calculated automatically; otherwise it must match `strlen(string)` (and the string must be properly null-terminated).
* If the borrowed string is short (fits in the inline area) it is simply copied into the `String` — there is no benefit to wrapping a short string, so it is stored as a normal short string.
* If the pointer is not correctly aligned, a copy is made.
* Otherwise (long and aligned), the `String` stores a **pointer** straight into the caller's buffer and uses no heap memory at all. The `String` is then said to be **wrapped**, and `wrapped()` returns `true`.

A wrapped string is a *read-only view*: its `capacity()` is `0`, it is not owned by the `String`, and the original buffer must outlive the `String` (including its null terminator, which is borrowed rather than stored). Passing a wrapped string where a plain C string is expected is cheap — just take `data()`.

> **Important:** the non-const accessors `data()`, `operator[]`, `first()`, `last()`, `begin()`, and `end()` are **not allowed** on a wrapped `String` — they assert in debug builds (the behavior is otherwise undefined). If you need to modify the string, it must be unwrapped first.

### Unwrapping and its cost

Modifying a wrapped string — or explicitly calling `String::unwrap()` — converts it from a borrowed view into an owned string:

```cpp
bool String::unwrap();   // true on success (or if already unwrapped), false on allocation failure
```

Unwrapping always requires a **copy**, and the cost depends on the size:

* If the string is **long**: one `malloc(length + 1)` and one `memcpy` of the whole string onto the heap.
* If the string is **short**: it is copied into the inline area (no heap allocation).

If the allocation fails, `unwrap()` returns `false` and the string is left **unchanged and still wrapped**. Note that many mutating operations (e.g. `append`, `erase`, and the `searchReplace*` utilities) call `unwrap()` automatically, so editing a wrapped string silently triggers this copy — and on failure they pollute the string themselves.

```cpp
char const* const original = "a fairly long read-only message that lives elsewhere";
String view = String::Wrap(original);   // zero-copy wrap; no heap buffer allocated

printf("%s\n", view.data());            // reading is fine

view.append("!");                       // triggers unwrap(): malloc + memcpy, then appends
if (view)
  printf("Now owned and modified: %s\n", view.data());
else
  printf("Failed to unwrap (out of memory).\n");
```

---

## `WideString`

`WideString` stores UTF-16 text (`WideChar` is `wchar_t` where `wchar_t` is 16-bit, otherwise `char16_t`). It always appends a null terminator.

Common constructors:

| Call | Behavior |
|---|---|
| `WideString()` | Empty wide string (no allocation until it holds characters). |
| `WideString(String const&)` `explicit` | Converts a UTF-8 `String` to UTF-16. |
| `WideString(char const*)` `explicit` | Converts a null-terminated UTF-8 C string to UTF-16. |
| `WideString(char8_t const*)` `explicit` | Same, for `char8_t` (C++20 and later builds only). |
| `WideString(WideChar const*)` | Copies a null-terminated UTF-16 C string. |
| `WideString(WideString const&)` / `WideString(WideString&&)` | Copy / move. |
| `WideString::FromBuffer(buf, n)` | Copy `n` wide characters from a buffer. |
| `WideString::FromBufferByteSwap(buf, n)` | Same, byte-swapping each unit (for cross-endian data). |
| `WideString::Fill(n, fill=0)` | `n` copies of a wide character. |
| `WideString::Wrap(WideChar const*)` | Wraps an existing null-terminated wide string (borrowed, read-only). |

Like `String`, `WideString` defines `WideString::MaxLength` and `WideString::NotFound` (equal to `String::NotFound`).

`WideString` has `length()` (query) and `length(n)` (change; returns `false` on failure), `empty()`, `data()`, `operator[]`, `first()`, `last()`, `begin()`/`end()`, `wrapped()`, `pollute()`, `clear()` (releases the storage and sets the length to zero), and `burn()` (securely erases the contents). A wrapped `WideString` is likewise a read-only view: its non-const accessors assert in debug builds, and — unlike `String` — it cannot be unwrapped in place.

Typical use — preparing a UTF-8 result for a Windows API call:

```cpp
String utf8 = "C:\\Program Files\\MyApp";
WideString wide(utf8);              // UTF-8 -> UTF-16 (validated conversion)

if (wide)
{
  // wide.data() is a valid null-terminated UTF-16 string for the Windows API.
  printf("UTF-16 length: %d\n", (int)wide.length());
}
else
  printf("Conversion failed.\n");
```

---

## Converting `String` and `WideString`

Conversion between the two classes is available in both directions and happens through the constructors (and the assignment operators where provided):

* `String(WideString const&)` — UTF-16 → UTF-8.
* `WideString(String const&)`, `WideString(char const*)` — UTF-8 → UTF-16.

Both directions perform a **validated** conversion of the source encoding. The conversion runs in two passes (measure the output size, then convert), always produces a null-terminated result, and allocates a new buffer, so both are capable of failing — polluting the result and reporting `false` from `operator bool` — if memory is unavailable or the output would overflow. The building blocks are also exposed directly in `trl::utility`:

```cpp
// UTF-16 -> UTF-8. dest==nullptr only computes the required size.
String::Length utility::convertUTF16ToUTF8(char* dest, WideString::WideChar const* source, WideString::Length sourceLength);

// UTF-8 -> UTF-16. dest==nullptr only computes the required size.
WideString::Length utility::convertUTF8ToUTF16(WideString::WideChar* dest, char const* source, String::Length sourceLength);
```

---

## String utilities (`trl::utility`)

A rich set of helpers operates on `String` (and, for several, on raw C strings). Functions that take a `length` argument default that argument to `0`, which means "the whole string." All search functions return `String::NotFound` (i.e. `-1`) when the match is not found.

### Character utilities

| Function | Description |
|---|---|
| `upperCase(unsigned char)` / `lowerCase(unsigned char)` | Convert a single ANSI character to upper/lower case. |
| `upperCase(String const&)` / `lowerCase(String const&)` | Return a new string with all ASCII characters cased. |
| `calculateLength(char const*, length=0)` | Count the characters in a null-terminated C string, up to `length` if it is non-zero. |

### Comparison

Case-sensitive vs. case-insensitive text comparison (case-insensitive applies to ASCII only).

| Function | Description |
|---|---|
| `compareStr(left, right, len=0)` | Lexicographic compare; returns `<0`, `0`, `>0`. |
| `sameStr(left, right, len=0)` | `true` if exactly equal. |
| `compareText(left, right, len=0)` | Compare ignoring case. |
| `sameText(left, right, len=0)` | `true` if equal ignoring case. |

Both `char const*` and `String const&` overloads exist, and `compareText`/`sameText` additionally accept an explicit length for each side (for C strings that are not null-terminated).

### Searching

| Function | Description |
|---|---|
| `findStr(text, match, pos=0, len=0)` | First occurrence index of `match`. |
| `findStrLast(text, match, pos=0, len=0)` | Last occurrence index of `match`. |
| `findText` / `findTextLast` | Same, case-insensitive. |
| `findChar(text, ch, pos=0, len=0)` / `findCharLast` | First/last index of a single character. |
| `containsStr(text, match, pos=0, len=0)` / `containsText` | `true` if `match` is present. |
| `startsWith(text, match, pos=0)` / `startsWithText` | `true` if `text` begins with `match`. |
| `endsWith(text, match)` / `endsWithText` | `true` if `text` ends with `match`. |

### Search and replace

`searchReplace`-style functions perform an **in-place** modification and return the same `String&` for chaining. Because they mutate the string, they will **auto-unwrap** a wrapped `String` first — which entails an allocation and can therefore pollute the string on failure.

| Function | Description |
|---|---|
| `searchReplace(str, match, replacement)` | Replace the first match (`char` or `String` match). |
| `searchReplaceAll(str, match, replacement)` | Replace every match. |
| `searchReplaceText(str, match, replacement)` | Replace the first match, case-insensitively. |
| `searchReplaceTextAll(str, match, replacement)` | Replace every match, case-insensitively. |
| `searchEraseAll(str, match)` | Remove every match. |

```cpp
String path = "C:\\Temp\\My File.TXT";

utility::searchReplaceAll(path, ' ', '_');           // spaces  -> underscores
utility::searchReplaceText(path, "Temp", "temp");    // case-insensitive word swap

if (path)
  printf("After replace: %s\n", path.data());
```

### Number and size conversion

| Function | Description |
|---|---|
| `strToInt(int64_t&, String const&, base=0)` | Parse to a 64-bit int (bool return). `base` is passed through to `strtoll` (0 = auto-detect). |
| `strToInt(String const&, int64_t def=0, base=0)` | Parse to a 64-bit int, returning a default on failure. |
| `intToStr(int64_t, base=10)` | Format a 64-bit int in bases 2–36 (the result is polluted for any other base). |
| `strToFloat(float&, String const&)` / `strToFloat(String const&, float def=0.0f)` | Parse a `float` (bool return / default on failure). |
| `floatToStr(float)` | Format a `float` (6 significant digits, `%g` style). |
| `strToDouble(double&, String const&)` / `strToDouble(String const&, double def=0.0)` | Parse a `double` (bool return / default on failure). |
| `doubleToStr(double)` | Format a `double`. |

### File path utilities

Platform-specific helpers for building and parsing file paths. `utility::PathDelimeter` is `'\\'` on Windows and `'/'` elsewhere; `utility::EndLine` is `"\r\n"` on Windows and `"\n"` elsewhere.

| Function | Description |
|---|---|
| `fixFilePath(String)` | Replace every wrong-direction delimiter with the platform's. |
| `appendFileSubPath(String, String const&)` | Append a sub-path, inserting the delimiter if it is missing. |
| `extractFileName(String const&)` | Return the file name after the last delimiter (or after the drive colon on Windows). |
| `extractFilePath(String const&)` | Return the directory portion, including its trailing delimiter; `String::Empty` if there is none. |
| `extractFileExtension(String const&)` | Return the extension with its dot (e.g. `".exe"`); `String::Empty` if the name has no extension. |
| `changeFileExtension(String const&, String const&)` | Replace the extension — the new extension must start with `'.'`; an empty extension strips it. |

```cpp
String path = "C:/Program Files/MyApp/setup.exe";
path = utility::fixFilePath(path);                       // C:\Program Files\MyApp\setup.exe (on Windows)
String name = utility::extractFileName(path);            // setup.exe
String dir  = utility::extractFilePath(path);            // C:\Program Files\MyApp\
String ext  = utility::extractFileExtension(path);       // .exe
String renamed = utility::changeFileExtension(path, ".msi");  // C:\Program Files\MyApp\setup.msi
```

### Comparers (functors)

Useful as comparers for containers such as `FlatMap`/`FlatSet` and `quickSort<>`:

* `utility::StringComparer` — case-sensitive comparison.
* `utility::TextComparer` — case-insensitive comparison (see `Array::quickSort<utility::TextComparer>()` in the [Arrays sample](../examples/src/Arrays.cpp)).

---

## Summary

| Concern | `String` | `WideString` |
|---|---|---|
| Encoding | ASCII/UTF-8 | UTF-16 |
| Inline storage | Yes (22 chars on x64, 10 on x32) | No (allocated only when non-empty) |
| Wrap C string | Yes (`String::Wrap`) | Yes (`WideString::Wrap`) |
| Unwrap in place | Yes (`String::unwrap()`) | No |
| Conversions | From `WideString` and C strings | From `String` and C strings |
| Relational operators | `==` `!=` `<` `>` `<=` `>=` | — (use the `utility` comparers) |
| Error reporting | `if (!s)`, `pollute()` / `unpollute()` | `if (!s)`, `pollute()` |
| Secure clear | `s.burn()` | `s.burn()` |

The library targets C++20 or later; the only C++20-specific parts are the `char8_t` overloads, which are compiled out when building with an older standard. For the exact list of members and their signatures, see [`include/TinyTRL_Strings.h`](../include/TinyTRL_Strings.h).
