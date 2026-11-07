# TinyTRL
**Tiny Template and Runtime Library** (TinyTRL) is a compact and efficient Standard C++ Library (STL) replacement.

The following templates, classes and functions are provided:
* *Array* - a general-purpose dynamic resizeable array.
* *FlatMap* - associative container between key and values using a sorted array as storage.
* *FlatSet* - a set of unique values using a sorted array as storage.
* *String* - string class that handles Ascii or UTF-8 encoded strings with Short String Optimization, and ability to "wrap" existing C strings without copying.
* *WideString* - UTF-16 string class mostly for calling Windows API with automatic conversion to/from String.
* *FileStream* - a stream class that enables reading from and writing to files on disk.
* *MemoryStream* - a stream class that enables working with memory using stream interface.
* Numerous string utilities for text comparison, search and replacement.
* Functions for working with file paths and extensions.
* Utility functions for working with files and directories.
* Basic mathematical (e.g. min, max) and utility (e.g. swap) functions.
* Basic timing functions.

Documentation:
* [Getting Started](docs/Getting_Started.md) — how to integrate TinyTRL into a project.
* [Containers](docs/Containers.md) — `Array`, `FlatMap` and `FlatSet`.
* [Strings](docs/Strings.md) — `String` and `WideString`.
* [Streams](docs/Streams.md) — `FileStream` and `MemoryStream`.
* [Error Handling](docs/Error_Handling.md) — the pollute-bit error model.

The library has the following objectives:
* Reasonably work in "freestanding" applications (those that do not link standard C++ library).
* Minimal dependency on standard C library (uses a small subset of functions such as *memcpy*, *memmove* and so on).
* Does not need or use C++ exceptions (see [Error Handling](docs/Error_Handling.md)).
* Can use native OS functions, e.g. from Windows or Posix APIs.
* Reasonably small, compact and efficient.
* Support modern OS, embedded systems and possibly some legacy OS (e.g. WinNT4) with some limitations.
* Support compiling with GCC, Clang or MSVC, targeting C++20 or later.

Usage:
* Include "TinyTRL.h" in your header and/or source file.
* Add ".cpp" files from "src" directory to the project.

Example code:
```cpp
#include "TinyTRL.h"

using namespace trl;

int main(int argc, char **argv)
{
  FlatMap<String, String> passwords = {
    UserPassword{"Dan", "user2000"},
    UserPassword{"Leo1", "leo2024"}};

  passwords.addp("henry", "rockplayer54");
  passwords.addp("James_Smith_92", "james 92");

  if (!passwords.exists("finn5"))
    passwords.addp("finn5", "finn5");

  if (!passwords)
    printf("One or more operations failed, likely due to insufficient memory.\n");

  if (String const* const password = passwords.value("henry"))
    printf("Henry's password is: %s\n", password->data());
  else
    printf("User 'henry' does not exist.\n");

  return 0;
}
```

TODO:
* Thread, synchronization and atomic functions (coming very soon).
* Map and Set using some kind of balanced trees (planned).
* HashMap container (planned).
* Rewrite some functions that still use standard C library calls (e.g. strtoll) with its own, to reduce dependencies.

***If you like the library, please consider sponsoring its development.***