# Error Handling

TinyTRL does not use C++ exceptions. Instead, every fallible operation is handled through an internal *error flag* (a.k.a. *pollute bit*) that is carried by the container or object performing the operation. This explicit, freestanding-friendly approach avoids exceptions entirely while still providing robust error detection.

## How it works

* Each managed object (`Array`, `FlatMap`, `FlatSet`, `String`, `WideString` and `Stream`) maintains a single error flag bit — the most significant bit (MSB) of the object's internal bookkeeping word (usually its capacity or size).
* When a fallible operation fails — typically a memory allocation failure (including during container construction), a buffer overflow, a failed read/write, or invalid/undecodable input — the object sets its error flag and is said to become **polluted**. The operation itself does not abort or throw; the polluted state is stored in the object. In practice, the pollute bit is set by container construction, by `addp()`-style operations, and by any other operation that allocates memory. (A few operations, such as `Array::add()` and `Array::insert()`, additionally report failure through their return values — see below.)
* The error state **propagates**: an operation performed on, with, or producing a polluted object will also result in a polluted object. Therefore, once any object becomes polluted, all objects derived from it (or produced in the same chain of operations) are polluted as well.
* A polluted object returns an empty or zero-filled result where applicable, so that it remains safely usable (its contents simply remain valid from before the failure).

## How to check for errors

All the objects above provide an `operator bool` that reports the current error state:

* `true` — the object is *not* polluted, i.e. all operations on it succeeded so far.
* `false` — the object is polluted, i.e. at least one operation on it failed.

```cpp
#include "TinyTRL.h"

using namespace trl;

int main()
{
  FlatMap<String, String> map;
  map.addp("key", "value");

  if (!map)
  {
    printf("One or more operations failed, likely due to insufficient memory.\n");
    return 1;
  }

  String string = "hello";
  string += " world";

  if (string)
    printf("Result: %s\n", string.data());

  return 0;
}
```

The recommended pattern is to test objects with `if (!object)` (or `if (object)`) after performing operations, especially after operations that allocate memory, and to handle the failure gracefully (report it, retry, or abort the program). Note that the `operator bool` is `explicit` and marked `[[nodiscard]]`, so a simple assignment to `bool` will not compile — always use the result in a conditional or via a cast.

In addition to the pollute bit, some operations report success or failure directly through their return values:

* `Array::add()` returns the index of the newly added element, or `Array::NotFound` (`-1`) if the addition failed — and does *not* set the pollute bit. `Array::addp()` performs the same operation but sets the pollute bit on failure instead.
* `Array::insert()`, `Array::populate()`, and the capacity-changing methods `capacity()`, `length()` and `shrink()` return `bool`, which is `false` on failure.

```cpp
Array<int> numbers;

// add() reports failure through its return value.
auto index = numbers.add(42);
if (index == Array::NotFound)
{
  printf("Failed to add element, likely due to a memory allocation failure.\n");
}

// addp() reports failure through the pollute bit instead.
numbers.addp(43);
if (!numbers)
{
  printf("Failed to add element, likely due to a memory allocation failure.\n");
}
```

## Resetting the error state

* Containers provide `unpollute()` to clear the error flag, e.g. after a retry has succeeded.
* `String::clear()` and equivalent stream operations reset the error flag while preserving the allocated resources.
* Note that checking the error flag does *not* reset it; the polluted state persists until explicitly cleared.

## Streams

For streams, the situation is slightly different: the low-level `read()`/`write()`/`seek()` members return a status that should be checked by the caller — `Size` for `read()`/`write()` (the actual number of bytes processed) and `Offset` for `seek()` (the new stream position) — where `Stream::Failure` (which is `-1`) indicates an error, while the high-level, fluent members (`readBuffer()`, `writeBuffer()`, `copy()`, `read(T)`, `write(T)`, `operator <<`, `operator >>`) set the stream's error flag on failure, which can then be checked with `if (!stream)`.

## Summary

| API | Error indicator |
| --- | --- |
| `Array`, `FlatMap`, `FlatSet` | `operator bool` (`if (!container)`), `pollute()` / `unpollute()` |
| `Array::add()`, `Array::insert()`, `Array::populate()`, `capacity()`, `length()`, `shrink()` | return value: index or `Array::NotFound` (`-1`); `false` on failure |
| `String`, `WideString` | `operator bool` (`if (!string)`), `pollute()` / `unpollute()` |
| `Stream` (low-level: `read`, `write`, `seek`) | returned `Size` / `Offset`; `Stream::Failure` (`-1`) indicates an error |
| `Stream` (high-level: `readBuffer`, `writeBuffer`, `copy`, `read(T)`, `write(T)`, `operator <<`, `operator >>`) | `operator bool` (`if (!stream)`) |
