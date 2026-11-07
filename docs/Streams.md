# Streams

TinyTRL provides buffered input/output for both files and memory through a small set of classes in the `trl` namespace:

* **`Stream`** — an abstract base that defines the operation set (position, size, read/write, seek, copy) and the *pollute* error state. You never instantiate it directly.
* **`FileStream`** — reads and writes files on disk, using native file handles under the hood.
* **`MemoryStream`** (via an internal `BaseMemoryStream`) — reads and writes an in-memory buffer that grows as needed.

```cpp
#include "TinyTRL.h"
using namespace trl;
```

All three share the same API surface and the same [error-handling model](Error_Handling.md): a *pollute* bit that is set when a fallible operation fails and can be tested with `if (!stream)`. The one important subtlety — and the reason this page exists — is that a stream exposes **two layers of API**, and they report failures in two different ways (see [Two API layers](#two-api-layers-error-reporting)).

---

## Types and constants

| Name | Value | Meaning |
|---|---|---|
| `Stream::Size` | `int64_t` on x64, `int32_t` on 32-bit | Buffer sizes / number of bytes. |
| `Stream::Offset` | `int64_t` | Positions and stream sizes (always 64-bit). |
| `Stream::Failure` | `-1` | Sentinel returned by the low-level operations on failure. |
| `Stream::DefaultBlockSize` | `8192` | Block size used by `copy()` unless you override it. |

Because `Size` is signed on every platform, a negative value is reserved for the "failed" result (`Failure` == `-1`) rather than encoding an error some other way.

---

## Two API layers (error reporting)

This is the core distinction when working with streams. The full treatment lives on the [Error Handling](Error_Handling.md#streams) page; here is how the pieces map to the stream code.

### 1. Low-level byte operations — *check the return value*

These return the number of bytes actually processed, or `Stream::Failure` (`-1`). They **do not** set the pollute bit themselves, so you are responsible for checking the result:

```cpp
Stream::Size read(void* buffer, Stream::Size size);
Stream::Size write(void const* buffer, Stream::Size size);
Stream::Offset seek(Stream::Offset offset, SeekOrigin origin = SeekOrigin::Beginning);
Stream::Offset position();   // current position from the start (>= 0), or Failure
Stream::Offset size();       // total size (>= 0), or Failure
```

```cpp
char buffer[256];
Stream::Size const got = fileStream.read(buffer, sizeof(buffer));

if (got < 0)
  printf("Read failed.\n");        // got == Stream::Failure
else if (got < sizeof(buffer))
  printf("Short read: %d bytes (possibly end of file).\n", (int)got);
```

> **Short reads are normal.** `read()` returns fewer bytes than requested when it reaches the end of the stream — that is not an error. It only returns `Failure` when the underlying I/O actually failed (and `write()` returns fewer bytes than requested on a partial write).

### 2. High-level "fluent" operations — *check the pollute bit*

These operate on fixed sizes, whole objects, or `String`s and **automatically set the pollute bit** on failure so you can test with `if (!stream)`. The `readBuffer` / `writeBuffer` / `operator <<` / `operator >>` forms return `Stream&`, so calls can be chained:

```cpp
Stream& readBuffer(void* buffer, Stream::Size size);
Stream& writeBuffer(void const* buffer, Stream::Size size);

template<typename T> T read();                          // read one whole object
template<typename T> void write(T const& value);        // write one whole object
template<typename T> Stream& operator >> (T& value);    // read into a variable
template<typename T> Stream& operator << (T const& value); // write a value

Stream& operator << (String const& string);             // write a string
Stream& operator >> (String& string);                   // read the remainder into a string

Stream::Offset copy(Stream& source, Stream::Offset size = 0, Stream::Size blockSize = Stream::DefaultBlockSize);
```

```cpp
int32_t value;
memoryStream << 0xDEADBEEF;   // writeBuffer under the hood
memoryStream >> value;        // readBuffer under the hood

if (!memoryStream)
  printf("One or more stream operations failed.\n");
```

The `read<T>()` / `write<T>()` templates wrap `readBuffer` / `writeBuffer` with `sizeof(T)`, so a partial read/write of a structure pollutes the stream — you don't have to compare byte counts yourself. There is also a dedicated `Stream::read()` overload (returning a new `String`) that reads the *remainder* of the stream, and a matching `Stream::write(String const&)` that writes a whole string.

---

## Seek origins and position

Every stream tracks a single *position* (the read/write cursor), always expressed as an offset from the **beginning** of the stream. `seek()` moves it relative to a `SeekOrigin`:

```cpp
enum class SeekOrigin : uint8_t { Beginning, Current, End };
```

| `origin` | `offset` | Resulting position |
|---|---|---|
| `Beginning` (default) | `off` | `off` (clamped to `0` if negative) |
| `Current` | `+off` | `current + off` |
| `Current` | `-off` | `current - off` (clamped to `0` if it would underflow) |
| `End` | `+off` | `size + off` |
| `End` | `-off` | `size - off` (clamped to `0` if it would underflow) |

Examples:

```cpp
stream.seek(0, SeekOrigin::Beginning);   // go to the start
stream.seek(-1, SeekOrigin::End);        // position just before the last byte
stream.seek(4, SeekOrigin::Current);     // advance 4 bytes from here
stream.position();                        // returns the value produced by the last seek/read/write
```

`seek()` returns the **new absolute position** (an `Offset`) or `Failure`. Note the asymmetry:

* For a **`MemoryStream`** the position is kept in memory, so `seek()` and `position()` effectively **never fail** (a bad offset is simply clamped to a valid one).
* For a **`FileStream`** the seek goes through the OS, so it **can** return `Failure`.

`truncate()` shrinks (or, for a `MemoryStream`, extends) the stream's `size` to the *current* position and returns whether it succeeded. `flush()` forces buffered data out to the underlying source — meaningful for `FileStream` (it calls `FlushFileBuffers` / `fdatasync`), a no-op that returns `true` for `MemoryStream`, and unsupported (`false`) on the abstract `Stream`.

---

## `copy` between streams

`copy()` transfers bytes from a *source* stream into *this* stream, starting at each stream's current position:

```cpp
Stream::Offset Stream::copy(Stream& source, Offset size = 0, Size blockSize = DefaultBlockSize);
```

* `size == 0` (the default) copies the **whole remainder** of the source; otherwise at most `size` bytes.
* `blockSize` is the size of the staging buffer; if the target is a `MemoryStream` with an empty, unallocated buffer, it is pre-allocated once up front to avoid repeated growth.
* Returns the number of bytes actually copied, or `Failure` (which also sets the pollute bit) if an allocation or I/O step fails.

Copying a `MemoryStream` into a `FileStream` is the idiomatic way to serialize in-memory data to disk:

```cpp
FileStream file("out.bin", FileStream::ModeCreate | FileStream::ShareExclusive);
file.copy(memoryStream);   // size==0 -> copy the whole memory stream
file.flush();

if (file)
  printf("Wrote %s\n", "out.bin");
```

---

## `FileStream`

`FileStream` opens a file when it is constructed and closes its handle when it is destroyed (RAII). It wraps native handles: `CreateFileW`/`ReadFile`/`WriteFile` on Windows, and POSIX `open`/`read`/`write`/`lseek` with `_FILE_OFFSET_BITS=64` elsewhere.

```cpp
FileStream(String const& fileName, FileMode mode, FileAttributes attributes = 0);
```

### File modes

`FileMode` is a `uint32_t` made by OR-ing together mode and sharing flags:

```cpp
enum {
  ModeRead     = 0x01,   // open for reading
  ModeWrite    = 0x02,   // open for writing
  ModeTruncate = 0x04,   // truncate an existing file

  ShareDenyRead   = 0x10, // deny other processes reading
  ShareDenyWrite  = 0x20, // deny other processes writing
  ShareDenyDelete = 0x40, // deny other processes deleting (or renaming)

  ModeCreate  = ModeRead | ModeWrite | ModeTruncate, // open for R/W, truncate if it exists
  ModeAppend  = ModeRead | ModeWrite,                // open for R/W, create if it doesn't exist
  ShareExclusive = ShareDenyRead | ShareDenyWrite    // deny others reading or writing
};
```

Common combinations:

| Mode | Effect (Windows) | Typical use |
|---|---|---|
| `ModeCreate \| ShareExclusive` | create + truncate, exclusive lock | start a brand-new file |
| `ModeAppend` | open + create, no truncate | append to an existing file |
| `ModeRead \| ShareDenyWrite` | open existing for read | safely read a file |

```cpp
// Write.
{
  FileStream f("data.bin", FileStream::ModeCreate | FileStream::ShareExclusive);
  if (f)
  {
    f.write<uint32_t>(0xA7B4_7241u);
    f.write<float>(32.5f);
    f << String::FromBuffer("hello", 5);
  }
} // implicitly flushed and closed here

// Read back.
{
  FileStream f("data.bin", FileStream::ModeRead | FileStream::ShareDenyWrite);
  if (f)
  {
    uint32_t n; float x;
    f >> n; f >> x;
  }
}
```

> **Cross-platform caveat.** The *sharing* flags are honoured natively on Windows. On POSIX they behave differently:
>
> * The deny-flags are implemented as an advisory whole-file `fcntl` lock, applied **only** when `ShareExclusive` is set — so, e.g., `ModeRead|ShareDenyWrite` does **not** lock the file.
> * `ShareDenyDelete` is not supported: if it is set, the file is not opened at all and the stream reports itself as invalid.
> * Write modes do **not** require `ModeRead`; `ModeWrite` alone opens the file with `O_WRONLY` (created if it doesn't exist).
> * Some filesystems do not support locking at all — `EINVAL`/`ENOTSUP` from the lock attempt are tolerated — but any other lock failure closes the handle, so the open fails.
>
> Plan your sharing expectations accordingly.

### `operator bool`

`FileStream` overrides `operator bool` so it reports `true` only when **both** the handle is valid and the stream is not polluted:

```cpp
if (!f)   // either the open failed, or a prior I/O operation polluted it
  handleFailure();
```

### File / directory helpers

| Call | Description |
|---|---|
| `FileStream::loadString(String const& fileName)` | Read an entire file into a `String`. Note: a UTF-8 file may keep its BOM (`0xEF 0xBB 0xBF`). |
| `FileStream::saveString(String const& fileName, String const& contents)` | Write a `String` to a file, returning `bool`. |
| `FileStream::fileExists(String const& name)` | `true` if a regular file exists at `name`. |
| `FileStream::directoryExists(String const& name)` | `true` if a directory exists at `name`. |
| `FileStream::createDirectory(String const& name)` | Create the directory if absent (POSIX also creates missing parents); `true` on success. |

`loadString` / `saveString` treat a `String` as an opaque byte buffer, so they are fine for binary data as well as text (see `exampleTextFiles` in the [Streams sample](../examples/src/Streams.cpp)).

---

## `MemoryStream`

`MemoryStream` stores its data in a growable heap buffer. There is no file backing it; it can be created on its own, pre-allocated, grown, truncated, and shrunk back down.

```cpp
MemoryStream(size_t capacity = 0);   // 0 -> start empty; otherwise preallocate
```

| Call | Description |
|---|---|
| `DataType* memory()` / `const* memory()` | Direct (read/write) pointer to the underlying buffer. `DataType` is `uint8_t`. |
| `size_t capacity() const` | Currently allocated capacity in bytes. |
| `bool capacity(size_t)` | Grow the capacity to at least the given size (`true` on success). |
| `bool shrinkToFit()` | Reclaim the buffer down to `size()`; if the stream is empty, this is equivalent to `clear()` (resets the pollute bit, keeps the allocated buffer). |
| `void clear()` | Reset `size` to 0 for a `MemoryStream`; **also clears the pollute bit** while keeping the allocated capacity. |
| `bool flush()` | No-op for a memory stream; returns `true`. |
| `bool truncate()` | Set `size` to the current position (growing with reallocation if the position is beyond `size`). |

`MemoryStream` grows with `realloc` as it is written. A growth can fail (memory exhaustion), in which case the operation returns `Failure` (low-level `write`) or sets the pollute bit (fluent `write<T>` / `writeBuffer`). The same is true of `copy()` and `capacity()`.

```cpp
MemoryStream buffer;         // start empty
buffer.write<uint32_t>(0xA7B4_7241u);
buffer.write<float>(32.5f);
buffer << "Test";

uint8_t const* bytes = buffer.memory();      // read raw bytes directly
Stream::Size const len = static_cast<Stream::Size>(buffer.size());

if (!buffer)
  printf("Failed to build the stream (likely out of memory).\n");
```

`MemoryStream` also implements move semantics (the buffer pointer is stolen from the source), while a copy re-allocates and copies its bytes.

---

## Choosing between them

| Concern | `FileStream` | `MemoryStream` |
|---|---|---|
| Backing store | a file on disk | an in-memory buffer |
| Can fail on I/O | Yes (`read`/`write`/`seek` can return `Failure`) | Only on allocation failures |
| `operator bool` also checks open | Yes (handle validity) | No (pollute only) |
| `flush()` | Real (`FlushFileBuffers` / `fdatasync`) | No-op, returns `true` |
| Buffer access | `handle()` (`FileStream::Handle`) | `memory()`, `capacity()`, `shrinkToFit()` |
| Typical use | Persist data, read back, files/dirs | Serialize/deserialize, accumulate, interop with `memory()`. |

Both implement the same `Stream` interface, so `copy()` can move data between any pair of them — buffer a response in memory and flush it to a file in a single line, or read a file into memory and hand it off elsewhere, all through the common `Stream&` API.

---

## Summary

| Concern | Details |
|---|---|
| Failure codes | Low-level `read`/`write`/`seek` return `Stream::Failure` (`-1`) or a byte/offset count. |
| Fluent API | `readBuffer`, `writeBuffer`, `read<T>`, `write<T>`, `operator <<`/`>>`, and `copy` set the pollute bit on failure — test with `if (!stream)`. |
| Error state | A single *pollute* bit per stream; see [Error Handling](Error_Handling.md). |
| Positioning | `seek(Offset, SeekOrigin)` with `Beginning`/`Current`/`End`; `position()`, `size()`, `truncate()`, `flush()`. |
| Cross-stream | `Stream::copy(Stream&, Offset size = 0, Size blockSize = DefaultBlockSize)`. |
| File specifics | `FileMode`/`Share*` flags (constructor arguments), `handle()`, `loadString`/`saveString`, `fileExists`/`directoryExists`/`createDirectory`. |
| Memory specifics | `memory()`, `capacity()`, `shrinkToFit()`, `clear()`, preallocated `capacity`. |

All of the above assumes a C++20 toolchain; for exact signatures see [`include/TinyTRL_Streams.h`](../include/TinyTRL_Streams.h).
