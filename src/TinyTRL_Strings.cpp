/*
 * This file is part of Tiny Template and Runtime Library (TinyTRL).
 * Copyright (c) 2024 Yuriy Kotsarenko. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and limitations under the License.
 */

#include "TinyTRL_Strings.h"

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <iconv.h>
#endif

#include <cwchar>
#include <errno.h>

namespace trl {

// Forward declarations

// Swaps byte order in a 16-bit unsigned integer.
static uint16_t byteSwap16(uint16_t const value) noexcept;

#ifdef __PLATFORM_BIG_ENDIAN

// Swaps byte order in a 32-bit unsigned integer.
static uint32_t byteSwap32(uint32_t const value) noexcept;

// Swaps byte order in a 64-bit unsigned integer.
static uint64_t byteSwap64(uint64_t const value) noexcept;

#endif

// String members.

String const String::Empty = {};

String::String()
: _chars(nullptr),
  _capacity(0),
  _length(0)
{
}

String::String(char const* const string)
: String()
{
  if (string)
  {
    size_t const length = utility::calculateLength(string);
    if (length <= static_cast<size_t>(MaxLength))
    {
      if (this->length(static_cast<Length>(length)))
        ::memcpy(data(), string, length + NullLength);
      else
        pollute();
    }
    else
      pollute(); // Source string length overflow.
  }
}

#if __cplusplus > 201811L

String::String(char8_t const* const string)
: String(reinterpret_cast<char const*>(string))
{
}

#endif

String::String(WideString const& string)
: String()
{
  String::Length const sourceLength = string.length();
  if (sourceLength > 0)
  {
    Length length = utility::convertUTF16ToUTF8(nullptr, string.data(), sourceLength);
    if (length < MaxLength && length >= 0 && this->capacity(length))
    {
      if (length > 0)
        length = utility::convertUTF16ToUTF8(data(), string.data(), sourceLength);

      if (length >= 0)
        static_cast<void>(this->length(length));
      else
        pollute();
    }
    else
      pollute(); // Overflow
  }
  if (!string)
    pollute(); // Propagate pollute status from source string.
}

String::String(char const charCode)
: String()
{
  _bytes[0] = charCode;
  _bytes[ShortLengthOffset] = 1;
}

String::String(String const& source)
: String()
{
  assert(this != &source); // Check for self-assignment

  if (!assign(source))
    pollute();
}

String::String(String&& source) noexcept
: _chars(source._chars),
  _capacity(source._capacity),
  _length(source._length)
{
  assert(this != &source); // Check for self-assignment

  source._chars = nullptr;
  source._capacity = source._length = 0;
}

String& String::operator = (String const& source)
{
  assert(this != &source); // Check for self-assignment

  if (!assign(source))
    pollute();

  return *this;
}

String& String::operator = (String&& source) noexcept
{
  assert(this != &source); // Check for self-assignment.

  if (longBit() && !wrappedBit())
    ::free(_chars);

  _chars = source._chars;
  _capacity = source._capacity;
  _length = source._length;

  source._chars = nullptr;
  source._capacity = source._length = 0;

  return *this;
}

String String::Fill(Length const length, char const fill)
{
  String res;

  if (length >= 0 && length <= MaxLength)
  {
    if (length > 0)
    {
      if (res.length(length))
        ::memset(res.data(), fill, length);
      else
        res.pollute();
    }
  }
  else
    res.pollute();

  return res;
}

String String::FromBuffer(char const* const buffer, Length length)
{
  String res;

  if (length >= 0 && length <= MaxLength)
  {
    if (buffer)
    {
      length = utility::calculateLength(buffer, length);

      if (res.length(length))
        ::memcpy(res.data(), buffer, length);
      else
        res.pollute();
    }
    else if (length > 0)
      res.pollute(); // Positive length but buffer is NULL.
  }
  else
    res.pollute(); // Negative or overflowing length specified.

  return res;
}

String String::FromRawBytes(char const* const buffer, Length const length)
{
  String res;

  if (length >= 0 && length <= MaxLength)
  {
    if (buffer)
    {
      if (res.length(length))
        ::memcpy(res.data(), buffer, length);
      else
        res.pollute(); // Memory allocation failed.
    }
    else if (length > 0)
      res.pollute(); // Positive length but buffer is NULL.
  }
  else
    res.pollute(); // Negative or overflowing length specified.

  return res;
}

String String::Wrap(char const* const string, Length length)
{
  String res;

  if (length >= 0 && length <= MaxLength)
  {
    if (string)
    {
      // Either length must be set to zero to autocalculate, or it must match the size of wrapped string,
      // and such string must be properly null-terminated.
      assert(length == 0 || (string[length] == 0 && utility::calculateLength(string) == length));

      if (length == 0)
        length = utility::calculateLength(string);

      if (length <= ShortCapacity - NullLength)
      { // The provided string qualifies as a short string, so does not require wrapping.
        // Note: no need to copy null character as string should be pre-filled with zeros anyway.
        ::memcpy(res._bytes, string, static_cast<size_t>(length));
        res._bytes[ShortLengthOffset] = static_cast<uint8_t>(length);
      }
      else if (reinterpret_cast<uintptr_t>(string) % alignof(String))
      { // Pointer does not have correct alignment, a copy of the string has to be created.
        if (res.length(length))
          ::memcpy(res._chars, string, static_cast<size_t>(length + NullLength));
        else
          res.pollute();
      }
      else
      { // Wrap an existing null-terminated C string with a known length.
        res._chars = const_cast<char*>(string);
        res._length = static_cast<Size>(length) | LongBit;
      }
    }
  }
  else
    res.pollute(); // Negative or overflowing length specified.

  return res;
}

#if __cplusplus > 201811L

String String::FromBuffer(char8_t const* const string, Length const length)
{
  return FromBuffer(reinterpret_cast<char const*>(string), length);
}

String String::FromRawBytes(char8_t const* const string, Length const length)
{
  return FromRawBytes(reinterpret_cast<char const*>(string), length);
}

String String::Wrap(char8_t const* const string, Length const length)
{
  return Wrap(reinterpret_cast<char const*>(string), length);
}

#endif

String String::Invalid()
{
  String string;
  string.pollute();
  return string;
}

String::~String()
{
  if (longBit() && !wrappedBit())
    ::free(_chars);
}

String& String::operator += (char const suffix)
{
  return append(suffix);
}

String& String::operator += (char const* const suffix)
{
  return append(suffix);
}

String& String::operator += (String const& suffix)
{
  return append(suffix);
}

String operator + (String const& prefix, char const suffix)
{
  String res;
  if (!res.internalConcatenate(prefix, suffix) || !prefix)
    res.pollute();
  return res;
}

String operator + (char const prefix, String const& suffix)
{
  String res;
  if (!res.internalConcatenate(prefix, suffix) || !suffix)
    res.pollute();
  return res;
}

String operator + (String const& prefix, char const* const suffix)
{
  String res;
  if (!res.internalConcatenate(prefix, String::Wrap(suffix)) || !suffix)
    res.pollute();
  return res;
}

String operator + (char const* const prefix, String const& suffix)
{
  String res;
  if (!res.internalConcatenate(String::Wrap(prefix), suffix) || !suffix)
    res.pollute();
  return res;
}

String operator + (String const& prefix, String const& suffix)
{
  String res;
  if (!res.internalConcatenate(prefix, suffix) || !prefix || !suffix)
    res.pollute();
  return res;
}

char* String::data()
{
  assert(!wrapped());
  return longBit() ? _chars : reinterpret_cast<char*>(_bytes);
}

char const* String::data() const
{
  return longBit() ? _chars : reinterpret_cast<char const*>(_bytes);
}

char& String::operator [] (Length const index)
{
  assert(!wrapped());
  return data()[index];
}

char const& String::operator [] (Length const index) const
{
  return data()[index];
}

char& String::first()
{
  assert(!wrapped());
  return *data();
}

char const& String::first() const
{
  return *data();
}

char& String::last()
{
  assert(!wrapped());
  Length const length = this->length();
  return *(length ? data() + length - 1 : data());
}

char const& String::last() const
{
  Length const length = this->length();
  return *(length ? data() + length - 1 : data());
}

char const* String::begin() const
{
  return data();
}

char const* String::end() const
{
  return data() + length();
}

char* String::begin()
{
  assert(!wrapped());
  return data();
}

char* String::end()
{
  assert(!wrapped());
  return data() + length();
}

String::operator bool () const
{
  return longBit() ? !(endianCapacity() & LongBit) : !(_bytes[ShortLengthOffset] & ShortPolluteBit);
}

String::Length String::capacity() const
{
  if (longBit())
  {
    if (!wrappedBit())
      return static_cast<Length>(endianCapacity() & LongLengthMask) - NullLength;
    else
      return 0; // Wrapped string has no capacity.
  }
  else
    return ShortCapacity - NullLength;
}

bool String::capacity(Length const capacity)
{ // Remember that this->capacity() subtracts null character from real capacity.
  if (capacity >= 0 && capacity <= MaxLength)
  { // Reallocate and preserve null character during copy operations.
    if (Length const actualCapacity = this->capacity(); actualCapacity < capacity)
      return reallocate(math::computeNextCapacity(capacity + NullLength, actualCapacity, ShortCapacity));
    else
      return true; // Current capacity is sufficient.
  }
  else
    return false; // Capacity underflow or overflow.
}

String::Length String::length() const
{
  return longBit() ? longLength() : shortLength();
}

bool String::length(Length const length)
{
  if (length >= 0 && length <= MaxLength)
  {
    Length const currentLength = this->length();

    if (currentLength != length)
    {
      if (internalCapacity(length))
      {
        writeLength(length);
        return true; // String reallocated.
      }
      else
        return false; // Allocation failed.
    }
    else
      return true; // Current string length is the same as desired one.
  }
  else
    return false; // Negative length is invalid.
}

bool String::empty() const
{
  return length() == 0;
}

bool String::wrapped() const
{
  return longBit() && wrappedBit();
}

String& String::pollute()
{
  if (longBit())
    endianCapacity(endianCapacity() | LongBit);
  else
    _bytes[ShortLengthOffset] |= ShortPolluteBit;

  return *this;
}

String& String::unpollute()
{
  if (longBit())
    endianCapacity(endianCapacity() & ~LongBit);
  else
    _bytes[ShortLengthOffset] &= ~ShortPolluteBit;

  return *this;
}

void String::clear()
{
  if (longBit() && !wrappedBit())
  {
    endianCapacity(endianCapacity() & LongLengthMask); // Reset pollute bit.
    endianLength(LongBit); // Zero length.
    _chars[0] = 0;
  }
  else
  {
    _chars = nullptr;
    _capacity = _length = 0;
  }
}

bool String::burn()
{
  if (!longBit() || !wrappedBit())
  {
    if (Length const length = this->length())
    {
      secureErase(data(), length);

      // Reset string length back to zero.
      if (longBit())
      {
        endianCapacity(endianCapacity() & LongLengthMask); // Reset pollute bit.
        endianLength(LongBit); // Zero length.
      }
      else
        _bytes[ShortLengthOffset] = 0;

      return true;
    }
    else
      return true; // Empty string is secure.
  }
  else
    return false; // Cannot burn a wrappedBit string.
}

bool String::shrink()
{
  if (!wrapped())
  {
    if (Length const length = this->length())
    {
      if (length + NullLength <= ShortCapacity)
      {
        if (longBit())
        { // Convert from long to short string.
          Size const polluteBit = endianCapacity() & LongBit;
          ::memcpy(_bytes, _chars, length + NullLength);
          _bytes[ShortLengthOffset] = static_cast<uint8_t>(length) | (polluteBit ? ShortPolluteBit : 0);
        }
        return true; // Short string.
      }
      else
        return reallocate(length + NullLength);
    }
    else
    { // Release string contents.
      if (longBit())
        ::free(_chars);

      _chars = nullptr;
      _capacity = _length = 0;

      return true;
    }
  }
  else
    return false; // Cannot shrink a wrapped string.
}

bool String::unwrap()
{
  return !wrapped() ? true : reallocate(longLength() + NullLength);
}

bool String::assign(String const& source)
{
  if (this != &source)
  {
    Length const length = source.length();

    if (this->length() != length)
    {
      if (internalCapacity(length))
        writeLength(length);
      else
        return false; // Allocation failed.
    }
    if (length)
      ::memcpy(data(), source.data(), length);

    if (!source)
      pollute(); // Propagate pollute status from source string.

    return true;
  }
  else
    return true; // Self-assignment.
}

String& String::append(char const suffix)
{
  if (!internalAppend(suffix))
    pollute();
  return *this;
}

String& String::append(char const* const suffix)
{
  if (!internalAppend(Wrap(suffix)))
    pollute();
  return *this;
}

String& String::append(String const& suffix)
{
  if (!internalAppend(suffix) || !suffix)
    pollute();
  return *this;
}

String& String::prepend(char prefix)
{
  if (!internalPrepend(prefix))
    pollute();
  return *this;
}

String& String::prepend(char const* const prefix)
{
  if (!internalPrepend(Wrap(prefix)))
    pollute();
  return *this;
}

String& String::prepend(String const& prefix)
{
  if (!internalPrepend(prefix))
    pollute();
  return *this;
}

String& String::concatenate(String const& prefix, char const suffix)
{
  if (!internalConcatenate(prefix, suffix) || !prefix)
    pollute();
  return *this;
}

String& String::concatenate(char const prefix, String const& suffix)
{
  if (!internalConcatenate(prefix, suffix) || !suffix)
    pollute();
  return *this;
}

String& String::concatenate(String const& prefix, String const& suffix)
{
  if (!internalConcatenate(prefix, suffix) || !prefix || !suffix)
    pollute();
  return *this;
}

String& String::concatenate(String const& prefix, char const* const suffix)
{
  if (!internalConcatenate(prefix, Wrap(suffix)) || !prefix)
    pollute();
  return *this;
}

String& String::concatenate(char const* const prefix, String const& suffix)
{
  if (!internalConcatenate(Wrap(prefix), suffix) || !suffix)
    pollute();
  return *this;
}

String& String::copy(String const& source, Length const sourcePosition, Length const sourceLength)
{
  if (!internalCopy(source, sourcePosition, sourceLength) || !source)
    pollute();
  return *this;
}

String String::substr(Length const position, Length const length) const
{
  String res;
  if (!res.internalCopy(*this, position, length) || !*this)
    res.pollute();
  return res;
}

String& String::replace(String const& source, Length const position, Length const length,
  Length const sourcePosition, Length const sourceLength)
{
  if (!internalReplace(source, position, length, sourcePosition, sourceLength) || !source)
    pollute();
  return *this;
}

String& String::insert(String const& source, Length const position, Length const sourcePosition,
  Length const sourceLength)
{
  if (!internalInsert(source, position, sourcePosition, sourceLength) || !source)
    pollute();
  return *this;
}

String& String::insert(String const& source, Length const position)
{
  if (!internalInsert(source, position, 0, source.length()) || !source)
    pollute();
  return *this;
}

bool String::insert(char const charCode, Length position)
{
  Length const currentLength = this->length();
  position = math::saturate<Length>(position, 0, currentLength);

  if (!internalCapacity(currentLength + 1))
    return false; // Allocation failed.

  if (Length const moveLength = currentLength - position)
    ::memmove(data() + position + 1, data() + position, moveLength);

  data()[position] = charCode;
  writeLength(currentLength + 1);

  return true;
}

String& String::erase(Length position, Length length)
{
  if (length == NotFound)
    length = this->length();

  if (length = math::max<Length>(length, 0); length)
  {
    Length const currentLength = this->length();

    if (position < 0)
    {
      length += position;
      position = 0;
    }
    if (position + length > currentLength)
    {
      length = math::max<Length>(currentLength - position, 0);
      position = math::min(position, currentLength);
    }
    if (length != 0)
    {
      if (!unwrap())
        return pollute(); // Allocation failed.

      if (Length const moveLength = currentLength - (position + length))
        ::memmove(data() + position, data() + position + length, moveLength);

      writeLength(currentLength - length);
    }
  }
  return *this;
}

String::Length String::store(void* const dest, Length const destLen) const
{
  if (dest && destLen > 0)
  {
    Length const copyBytes = math::min(length(), destLen);
    if (copyBytes)
      ::memcpy(dest, data(), copyBytes);

    Length const remaining = destLen - copyBytes;
    if (remaining)
      ::memset(static_cast<unsigned char*>(dest) + copyBytes, 0, remaining);

    return copyBytes;
  }
  else
    return 0;
}

bool String::internalAppend(char const suffix)
{
  if (Length const length = this->length(); length < MaxLength)
  {
    Length const destLength = length + 1;

    if (!internalCapacity(destLength))
      return false; // Allocation failed.

    data()[length] = suffix;
    writeLength(destLength);
    return true;
  }
  else
    return false; // Overflow.
}

bool String::internalAppend(String const& suffix)
{
  Length const length = this->length();
  Length suffixLength = suffix.length(), destLength;
  bool complete = true;

  if (length > MaxLength - suffixLength)
  { // Overflow.
    destLength = MaxLength;
    suffixLength = MaxLength - length;
    complete = false;
  }
  else
    destLength = length + suffixLength;

  if (suffixLength)
  {
    if (!internalCapacity(destLength))
      return false; // Allocation failed.

    ::memcpy(data() + length, suffix.data(), suffixLength);
    writeLength(destLength);
  }
  return complete;
}

bool String::internalPrepend(char const prefix)
{
  if (Length const length = this->length(); length < MaxLength)
  {
    Length const destLength = length + 1;

    if (!internalCapacity(destLength))
      return false; // Allocation failed.

    char* const data = this->data();
    if (length)
      ::memmove(data + 1, data, length);

    *data = prefix;
    writeLength(destLength);
    return true;
  }
  else
    return false; // Overflow;
}

bool String::internalPrepend(String const& prefix)
{
  if (Length const prefixLength = prefix.length())
  {
    Length length = this->length(), destLength;
    bool complete = true;

    if (length > MaxLength - prefixLength)
    { // Overflow
      destLength = MaxLength;
      length = MaxLength - prefixLength;
      complete = false;
    }
    else
      destLength = length + prefixLength;

    if (!internalCapacity(destLength))
      return false; // Allocation failed.

    char* const data = this->data();

    if (length)
      ::memmove(data + prefixLength, data, length);

    ::memcpy(data, prefix.data(), prefixLength);
    writeLength(destLength);
    return complete;
  }
  else
    return true; // Prefix is empty.
}

bool String::internalConcatenate(String const& prefix, char const suffix)
{
  Length const prefixLength = prefix.length();
  Length const destLength = prefixLength < MaxLength ? prefixLength + 1 : prefixLength;

  if (!internalCapacity(destLength))
    return false; // Allocation failed.

  char* const data = this->data();

  if (prefixLength)
    ::memcpy(data, prefix.data(), prefixLength);

  if (destLength > prefixLength)
    data[prefixLength] = suffix;

  writeLength(destLength);
  return destLength > prefixLength;
}

bool String::internalConcatenate(char prefix, String const& suffix)
{
  Length suffixLength = suffix.length(), destLength;
  bool complete = true;

  if (suffixLength >= MaxLength)
  { // Overflow.
    destLength = MaxLength;
    suffixLength = MaxLength - 1;
    complete = false;
  }
  else
    destLength = suffixLength + 1;

  if (!internalCapacity(destLength))
    return false; // Allocation failed.

  char* const data = this->data();

  if (suffixLength)
    ::memcpy(data + 1, suffix.data(), suffixLength);

  *data = prefix;
  writeLength(destLength);
  return complete;
}

bool String::internalConcatenate(String const& prefix, String const& suffix)
{
  Length const prefixLength = prefix.length();
  Length suffixLength = suffix.length(), destLength;
  bool complete = true;

  if (suffixLength > MaxLength - prefixLength)
  { // Overflow.
    destLength = MaxLength;
    suffixLength = MaxLength - prefixLength;
    complete = false;
  }
  else
    destLength = prefixLength + suffixLength;

  if (!internalCapacity(destLength))
    return false; // Allocation failed.

  char* const data = this->data();

  if (prefixLength)
    ::memcpy(data, prefix.data(), prefixLength);

  if (suffixLength)
    ::memcpy(data + prefixLength, suffix.data(), suffixLength);

  writeLength(destLength);
  return complete;
}

bool String::internalCopy(String const& source, Length sourcePosition, Length sourceLength)
{
  Length const actualSourceLength = source.length();

  if (sourceLength == NotFound)
    sourceLength = actualSourceLength;

  if (sourceLength >= 0)
  {
    if (sourcePosition < 0)
    {
      sourceLength += sourcePosition;
      sourcePosition = 0;
    }
    if (sourcePosition + sourceLength > actualSourceLength)
      sourceLength = math::max<Length>(actualSourceLength - sourcePosition, 0);

    if (sourceLength > 0)
    {
      if (!internalCapacity(sourceLength))
        return false; // Allocation failed.

      ::memcpy(data(), source.data() + sourcePosition, sourceLength);
      writeLength(sourceLength);
      return true;
    }
    else
      return true; // Nothing to copy.
  }
  else
    return false; // Invalid length specified.
}

bool String::internalReplace(String const& source, Length position, Length length, Length sourcePosition,
  Length sourceLength)
{
  if (length == NotFound)
    length = this->length();

  if (sourceLength == NotFound)
    sourceLength = source.length();

  length = math::max<Length>(length, 0);
  sourceLength = math::max<Length>(sourceLength, 0);

  if (length || sourceLength)
  {
    {
      Length const actualSourceLength = source.length();

      if (sourcePosition < 0)
      {
        sourceLength += sourcePosition;
        sourcePosition = 0;
      }
      if (sourcePosition + sourceLength > actualSourceLength)
      {
        sourceLength = math::max<Length>(actualSourceLength - sourcePosition, 0);
        sourcePosition = math::min<Length>(sourcePosition, actualSourceLength);
      }
    }
    Length const actualLength = this->length();

    if (position < 0)
    {
      length += position;
      position = 0;
    }
    if (position + length > actualLength)
    {
      length = math::max<Length>(actualLength - position, 0);
      position = math::min<Length>(position, actualLength);
    }
    Length const lengthDiff = sourceLength - length;

    if ((lengthDiff || sourceLength) && !unwrap())
      return false; // Allocation failed.

    if (lengthDiff)
    {
      if (lengthDiff > 0 && (actualLength > MaxLength - lengthDiff ||
        !internalCapacity(actualLength + lengthDiff)))
        return false; // Allocation failed or overflow.

      Length const moveLength = actualLength - (position + length);
      if (moveLength)
        ::memmove(data() + position + sourceLength, data() + position + length, moveLength);
    }
    if (sourceLength)
      ::memcpy(data() + position, source.data() + sourcePosition, sourceLength);

    if (lengthDiff)
      writeLength(actualLength + lengthDiff);
  }
  return true;
}

bool String::internalInsert(String const& source, Length position, Length sourcePosition,
  Length sourceLength)
{
  if (sourceLength == NotFound)
    sourceLength = source.length();

  if (sourceLength = math::max<Length>(sourceLength, 0); sourceLength)
  {
    Length const actualSourceLength = source.length();

    if (sourcePosition < 0)
    {
      sourceLength += sourcePosition;
      sourcePosition = 0;
    }
    if (sourcePosition + sourceLength > actualSourceLength)
    {
      sourceLength = math::max<Length>(actualSourceLength - sourcePosition, 0);
      sourcePosition = math::min(sourcePosition, actualSourceLength);
    }
    Length const currentLength = this->length();
    position = math::saturate<Length>(position, 0, currentLength);

    if (sourceLength)
    {
      if (!internalCapacity(currentLength + sourceLength))
        return false; // Allocation failed.

      if (Length const moveLength = currentLength - position)
        ::memmove(data() + position + sourceLength, data() + position, moveLength);

      ::memcpy(data() + position, source.data() + sourcePosition, sourceLength);
      writeLength(currentLength + sourceLength);
    }
  }
  return true;
}

bool String::internalCapacity(Length const capacity)
{ // Remember that this->capacity() subtracts null character from real capacity.
  if (Length const actualCapacity = this->capacity(); actualCapacity < capacity)
    return reallocate(math::computeNextCapacity(capacity + NullLength, actualCapacity, ShortCapacity), 0);
  else
    return true; // Current capacity is sufficient.
}

bool String::reallocate(Length const capacity, Length const copyNullLength)
{
  assert(copyNullLength >= 0 && copyNullLength <= NullLength);
  assert(capacity >= 0 && capacity > length());

  if (longBit())
  {
    Size const polluteBit = endianCapacity() & LongBit;

    if (!wrappedBit())
    { // Change size of a long string.
      if (char* chars = reinterpret_cast<char*>(::realloc(_chars, static_cast<size_t>(capacity))))
      {
        _chars = chars;
        endianCapacity(static_cast<Size>(capacity) | polluteBit);
        return true;
      }
      else
        return false; // allocation failed.
    }
    else
    {
      Length const length = longLength();
      assert(length > 0); // Wrapped string must have at least one character.

      if (capacity > ShortCapacity)
      { // Convert wrapped to long string.
        if (char* chars = reinterpret_cast<char*>(malloc(static_cast<size_t>(capacity))))
        {
          ::memcpy(chars, _chars, static_cast<size_t>(length + copyNullLength));
          _chars = chars;
          endianCapacity(static_cast<Size>(capacity) | polluteBit);
          return true;
        }
        else
          return false; // allocation failed.
      }
      else
      { // Convert wrapped to short string.
        ::memcpy(_bytes, _chars, static_cast<size_t>(length + copyNullLength));
        _bytes[ShortLengthOffset] = static_cast<uint8_t>(length) | (polluteBit ? ShortPolluteBit : 0);
        return true;
      }
    }
  }
  else
  { // Convert from short to long string.
    if (char* chars = reinterpret_cast<char*>(malloc(static_cast<size_t>(capacity))))
    {
      Size const polluteBit = _bytes[ShortLengthOffset] & ShortPolluteBit ? LongBit : 0;
      Size const length = shortLength();

      char shortContent[ShortCapacity];
      ::memcpy(shortContent, _bytes, static_cast<size_t>(length + NullLength));

      _chars = chars;
      endianCapacity(static_cast<Size>(capacity) | polluteBit);
      endianLength(length | LongBit); // String is now located on the heap.

      ::memcpy(_chars, shortContent, static_cast<size_t>(length + NullLength));
      return true;
    }
    else
      return false; // allocation failed.
  }
}

void String::writeLength(Length const length)
{
  assert(length >= 0 && length <= capacity());

  if (longBit())
  {
    assert(!wrappedBit()); // writeLength() must not be called when string is wrapped.

    endianLength(static_cast<Size>(length) | LongBit);
    _chars[length] = 0;
  }
  else
  {
    _bytes[ShortLengthOffset] = static_cast<uint8_t>(length) | (_bytes[ShortLengthOffset] & ShortPolluteBit);
    _bytes[length] = 0;
  }
}

String::Length String::longLength() const
{
  return static_cast<Length>(endianLength() & LongLengthMask);
}

String::Length String::shortLength() const
{
  return _bytes[ShortLengthOffset] & ShortLengthMask;
}

bool String::wrappedBit() const
{
  assert(longBit()); // wrappedBit() must not be called when long bit is not set.
  return (endianCapacity() & LongLengthMask) == 0;
}

bool String::longBit() const
{
  return _bytes[ShortLengthOffset] & ShortLongBit;
}

String::Size String::endianLength() const
{
#ifdef __PLATFORM_BIG_ENDIAN
  #ifdef __PLATFORM_X64
    return byteSwap64(_length);
  #else
    return byteSwap32(_length);
  #endif
#else
  return _length;
#endif
}

void String::endianLength(Size const length)
{
#ifdef __PLATFORM_BIG_ENDIAN
  #ifdef __PLATFORM_X64
    _length = byteSwap64(length);
  #else
    _length = byteSwap32(length);
  #endif
#else
  _length = length;
#endif
}

String::Size String::endianCapacity() const
{
#ifdef __PLATFORM_BIG_ENDIAN
  #ifdef __PLATFORM_X64
    return byteSwap64(_capacity);
  #else
    return byteSwap32(_capacity);
  #endif
#else
  return _capacity;
#endif
}

void String::endianCapacity(Size const capacity)
{
#ifdef __PLATFORM_BIG_ENDIAN
  #ifdef __PLATFORM_X64
    _capacity = byteSwap64(capacity);
  #else
    _capacity = byteSwap32(capacity);
  #endif
#else
  _capacity = capacity;
#endif
}

void String::secureErase(char* const string, Length length)
{
  assert(string && length > 0);
  for (char volatile* dest = static_cast<char volatile *>(string); length--; )
    *dest++ = 0;
}

// WideString members.

WideString::WideString()
: _chars(nullptr),
  _length(0)
{
}

WideString::WideString(String const& string)
: WideString()
{
  String::Length const sourceLength = string.length();
  if (sourceLength > 0)
  {
    Length length = utility::convertUTF8ToUTF16(nullptr, string.data(), sourceLength);
    if (length < MaxLength && length >= 0 && this->length(length))
    {
      if (length > 0)
        length = utility::convertUTF8ToUTF16(data(), string.data(), sourceLength);

      if (length >= 0)
        static_cast<void>(this->length(length));
      else
        pollute();
    }
    else
      pollute(); // Overflow
  }
  if (!string)
    pollute(); // Propagate pollute status from source string.
}

WideString::WideString(char const* string)
: WideString()
{
  if (string)
  {
    Length const length = utility::calculateLength(string);
    Length wideLength = utility::convertUTF8ToUTF16(nullptr, string, length);

    if (wideLength > MaxLength)
    { // Overflow check.
      pollute();
      wideLength = MaxLength;
    }
    if (wideLength && this->length(wideLength))
      utility::convertUTF8ToUTF16(data(), string, length);
    else
      pollute(); // Issues with conversions or a memory allocation failure.
  }
}

#if __cplusplus > 201811L
WideString::WideString(char8_t const* const string)
: WideString(reinterpret_cast<char const*>(string))
{
}
#endif

WideString::WideString(WideChar const* const string)
: WideString()
{
  if (string)
  {
    Length length = calculateLength(string);

    if (length > MaxLength)
    { // Overflow check.
      pollute();
      length = MaxLength;
    }
    if (this->length(length))
    {
      if (length > 0)
        ::memcpy(data(), string, (length + 1) * sizeof(WideChar));
    }
    else
      pollute(); // Memory allocation failure.
  }
}

WideString::WideString(WideString const& source)
: WideString()
{
  assert(this != &source); // Check for self-assignment.

  if (!source.wrapped())
  { // Copy contents of a regular string.
    Length const length = source.length();
    if (length > 0)
    {
      if (resize(length))
      {
        ::memcpy(_chars, source._chars, static_cast<size_t>(length) * sizeof(WideChar));
        if (!source)
          pollute();
      }
      else
        pollute();
    }
  }
  else
  { // Copy wrapped pointers from source string.
    _chars = source._chars;
    _length = source._length;
  }
}

WideString::WideString(WideString&& source) noexcept
: _chars(source._chars),
  _length(source._length)
{
  assert(this != &source); // Check for self-assignment.

  source._chars = nullptr;
  source._length = 0;
}

WideString WideString::FromBuffer(WideChar const* const buffer, Length length)
{
  WideString res;

  if (length >= 0 && length <= MaxLength)
  {
    if (buffer)
    {
      length = calculateLength(buffer, length);

      if (res.length(length))
        ::memcpy(res.data(), buffer, static_cast<size_t>(length) * sizeof(WideChar));
      else
        res.pollute();
    }
  }
  else
    res.pollute();

  return res;
}

WideString WideString::FromBufferByteSwap(WideChar const* const buffer, Length length)
{
  WideString res;

  if (length >= 0 && length <= MaxLength)
  {
    if (buffer)
    {
      length = calculateLength(buffer, length);

      if (res.length(length))
      {
        WideChar* const dest = res.data();

        for (Length i = 0; i < length; ++i)
          dest[i] = byteSwap16(buffer[i]);
      }
      else
        res.pollute();
    }
  }
  else
    res.pollute();

  return res;
}

WideString WideString::Fill(Length const length, WideChar const fill)
{
  WideString res;

  if (length >= 0 && length <= MaxLength)
  {
    if (length > 0)
    {
      if (res.length(length))
      {
        for (Length i = 0; i < length; ++i)
          res._chars[i] = fill;
      }
      else
        res.pollute();
    }
  }
  else
    res.pollute();

  return res;
}

WideString WideString::Wrap(WideChar const* const string)
{
  WideString res;
  res._chars = const_cast<WideChar*>(string);
  return res;
}

WideString& WideString::operator = (WideString const& source)
{
  assert(this != &source); // Check for self-assignment.

  if (!source.wrapped())
  { // Copy contents of a regular string.
    Length const length = source.length();
    if (length > 0)
    {
      if (resize(length))
      {
        ::memcpy(_chars, source._chars, static_cast<size_t>(length) * sizeof(WideChar));
        if (!source)
          pollute();
      }
      else
        pollute();
    }
    else
      clear();
  }
  else
  { // Copy wrapped pointers from source string.
    if (!wrapped())
      ::free(_chars); // Release owned storage before re-pointing to the wrapped buffer.
    _chars = source._chars;
    _length = source._length;
  }
  return *this;
}

WideString& WideString::operator = (WideString&& source) noexcept
{
  assert(this != &source); // Check for self-assignment.

  if (!wrapped())
    ::free(_chars);

  _chars = source._chars;
  _length = source._length;
  source._chars = nullptr;
  source._length = 0;
  return *this;
}

WideString::~WideString()
{
  if (!wrapped())
    ::free(_chars);
}

WideString::WideChar* WideString::data()
{
  assert(!wrapped());
  return _chars;
}

WideString::WideChar const* WideString::data() const
{
  return _chars;
}

WideString::WideChar& WideString::operator [] (Length const index)
{
  assert(!wrapped());
  return _chars[index];
}

WideString::WideChar const& WideString::operator [] (Length const index) const
{
  return _chars[index];
}

WideString::WideChar& WideString::first()
{
  assert(!wrapped());
  return *_chars;
}

WideString::WideChar const& WideString::first() const
{
  return *_chars;
}

WideString::WideChar& WideString::last()
{
  assert(!wrapped());
  Length const length = this->length();
  return *(length ? _chars + length - 1 : _chars);
}

WideString::WideChar const& WideString::last() const
{
  assert(!wrapped());
  Length const length = this->length();
  return *(length ? _chars + length - 1 : _chars);
}

WideString::WideChar const* WideString::begin() const
{
  assert(!wrapped());
  return _chars;
}

WideString::WideChar const* WideString::end() const
{
  assert(!wrapped());
  return _chars + length();
}

WideString::WideChar* WideString::begin()
{
  assert(!wrapped());
  return _chars;
}

WideString::WideChar* WideString::end()
{
  assert(!wrapped());
  return _chars + length();
}

WideString::operator bool () const
{
  return !(_length & LongPolluteBit);
}

WideString::Length WideString::length() const
{
  return !wrapped() ? readLength() : calculateLength(_chars);
}

bool WideString::length(Length const length)
{
  if (length >= 0 && length <= MaxLength)
  {
    if (length != this->length())
    {
      if (length)
        return resize(length);
      else
      {
        clear();
        return true; // Relased string.
      }
    }
    else
      return true; // No change required.
  }
  else
    return false; // Negative length is invalid.
}

bool WideString::empty() const
{
  return !_chars;
}

void WideString::pollute()
{
  _length |= LongPolluteBit;
}

void WideString::clear()
{
  if (!wrapped())
    ::free(_chars);
  _chars = nullptr;
  _length = 0;
}

bool WideString::burn()
{
  if (_chars)
  {
    Length const length = readLength();
    if (length > 0)
    {
      secureErase(_chars, length);
      return true; // String contents have been secured.
    }
    else
      return false; // Cannot burn a wrapped string.
  }
  else
    return true; // String is empty, so is secure.
}

bool WideString::wrapped() const
{
  return _chars && !(_length & LongLengthMask);
}

bool WideString::resize(Length const length)
{
  assert(length >= 0);
  Size const currentEndianLength = _length;
  Size const polluteBit = currentEndianLength & LongPolluteBit;
  Length const currentLength = static_cast<Length>(currentEndianLength & LongLengthMask);

  if (!_chars || currentLength)
  { // Resize a regular string.
    if (WideChar* chars = reinterpret_cast<WideChar*>(::realloc(_chars,
      static_cast<size_t>(length + 1) * sizeof(WideChar))))
    {
      _chars = chars;
      _chars[length] = 0;

      _length = static_cast<Size>(length) | polluteBit;
      return true;
    }
    else
      return false; // allocation failed.
  }
  else
  { // Resize a wrapped string.
    if (WideChar* chars = reinterpret_cast<WideChar*>(::malloc(static_cast<size_t>(length + 1) *
      sizeof(WideChar))))
    {
      if (Length const copyLength = math::min(currentLength, calculateLength(_chars)))
        ::memcpy(chars, _chars, static_cast<size_t>(copyLength));

      _chars = chars;
      _chars[length] = 0;

      _length = static_cast<Size>(length) | polluteBit;
      return true;
    }
    else
      return false; // allocation failed.
  }
}

WideString::Length WideString::readLength() const
{
  return static_cast<Length>(_length & LongLengthMask);
}

void WideString::secureErase(WideChar* const string, Length length)
{
  for (WideChar volatile* dest = static_cast<WideChar volatile *>(string); length--; )
    *dest++ = 0;
}

WideString::Length WideString::calculateLength(WideChar const* string, Length const length)
{
  assert(string);
  Length count = 0;
  for (; *string && (length == 0 || count < length); ++string)
    ++count;
  return count;
}

// Global string operators.

bool operator < (String const& left, String const& right)
{
  return utility::compareStr(left, right) < 0;
}

bool operator > (String const& left, String const& right)
{
  return utility::compareStr(left, right) > 0;
}

bool operator <= (String const& left, String const& right)
{
  return utility::compareStr(left, right) <= 0;
}

bool operator >= (String const& left, String const& right)
{
  return utility::compareStr(left, right) >= 0;
}

bool operator == (String const& left, String const& right)
{
  return utility::compareStr(left, right) == 0;
}

bool operator != (String const& left, String const& right)
{
  return utility::compareStr(left, right) != 0;
}

// Global utility functions.

namespace utility {

// Character utilities.

static bool compareStringPointers(void const* const leftText, void const* const rightText,
  String::Length& comparison)
{
  bool equal = true;

  if (leftText && !rightText)
  { // Text2 is empty, whereas Text1 is not.
    comparison = 1;
    equal = false;
  }
  else if (!leftText && rightText)
  { // Text1 is empty, whereas Text2 is not.
    comparison = -1;
    equal = false;
  }
  else if (!leftText && !rightText)
    // Both Text1 and Text2 are empty.
    equal = false;

  return equal;
}

static bool compareTextCharacters(unsigned char code1, unsigned char code2, String::Length& comparison)
{
  bool different = false;

  if (code1 != code2)
  {
    code1 = upperCase(code1);
    code2 = upperCase(code2);

    if (code1 != code2)
    {
      comparison = static_cast<String::Length>(code1) - code2;
      different = true;
    }
  }
  return different;
}

unsigned char upperCase(unsigned char const charCode)
{
  return charCode >= 97u && charCode <= 122u ? charCode - 32u : charCode;
}

unsigned char lowerCase(unsigned char const charCode)
{
  return charCode >= 65u && charCode <= 90u ? charCode + 32u : charCode;
}

String::Length calculateLength(char const* string, String::Length length)
{
  if (!string)
    return 0;

  if (length <= 0)
    length = String::MaxLength;

  String::Length count = 0;
  for (; *string && count < length; ++string)
    ++count;
  return count;
}

String::Length compareStr(char const* const left, char const* const right, String::Length const length)
{
  String::Length difference = 0;

  if (compareStringPointers(left, right, difference))
  {
    String::Length lengthLeft = 0, lengthRight = 0, currentLength = 0;

    // Comparison has to be performed one character at a time because string length of the left and/or right
    // sides isn't known at this point.

    while (length <= 0 || currentLength < length)
    {
      char const codeLeft = left[lengthLeft], codeRight = right[lengthRight];
      if (!codeLeft && !codeRight)
        break; // Both strings end at the same time.

      if (codeLeft)
        ++lengthLeft;

      if (codeRight)
        ++lengthRight;

      if (lengthLeft != lengthRight)
      {
        difference = lengthLeft - lengthRight;
        break; // One of the strings is shorter.
      }
      if (codeLeft != codeRight)
      {
        difference = static_cast<String::Length>(codeLeft) - codeRight;
        break; // Values are different.
      }
      currentLength++;
    }
  }
  return difference;
}

bool sameStr(char const* const left, char const* right, String::Length const length)
{
  return compareStr(left, right, length) == 0;
}

String::Length compareStr(String const& left, String const& right, String::Length const length)
{
  String::Length lengthLeft = left.length(), lengthRight = right.length();
  if (length > 0)
  {
    lengthLeft = math::min(lengthLeft, length);
    lengthRight = math::min(lengthRight, length);
  }
  String::Length comparison = 0;

  if (lengthLeft > 0 && lengthRight > 0)
    comparison = ::memcmp(left.data(), right.data(), math::min(lengthLeft, lengthRight));

  if (!comparison)
    comparison = lengthLeft - lengthRight;

  return comparison;
}

bool sameStr(String const& left, String const& right, String::Length const length)
{
  return compareStr(left, right, length) == 0;
}

String::Length compareText(char const* const left, char const* const right, String::Length const length)
{
  String::Length difference = 0;

  if (compareStringPointers(left, right, difference))
  {
    String::Length lengthLeft = 0, lengthRight = 0, currentLength = 0;

    // Comparison has to be performed one character at a time because string length of the left and/or right
    // sides isn't known at this point.

    while (!length || currentLength < length)
    {
      unsigned char const codeLeft = left[lengthLeft], codeRight = right[lengthRight];
      if (!codeLeft && !codeRight)
        break; // Both strings end at the same time.

      if (codeLeft)
        ++lengthLeft;

      if (codeRight)
        ++lengthRight;

      if (lengthLeft != lengthRight)
      {
        difference = lengthLeft - lengthRight;
        break; // One of the strings is shorter.
      }
      if (compareTextCharacters(codeLeft, codeRight, difference))
        break; // Values are different.

      currentLength++;
    }
  }
  return difference;
}

bool sameText(char const* const left, char const* const right, String::Length const length)
{
  return compareText(left, right, length) == 0;
}

String::Length compareText(String const& leftText, String const& rightText, String::Length const length)
{
  return compareText(leftText.data(), leftText.length(), rightText.data(), rightText.length(), length);
}

bool sameText(String const& leftText, String const& rightText, String::Length const length)
{
  return compareText(leftText, rightText, length) == 0;
}

String::Length compareText(char const* const left, String::Length lengthLeft, char const* const right,
  String::Length lengthRight, String::Length const length)
{
  if (length > 0)
  {
    lengthLeft = math::min(lengthLeft, length);
    lengthRight = math::min(lengthRight, length);
  }
  String::Length const lengthCompare = math::min(lengthLeft, lengthRight);

  for (String::Length i = 0; i < lengthCompare; ++i)
  {
    unsigned char codeLeft = left[i], codeRight = right[i];
    if (codeLeft != codeRight)
    {
      codeLeft = upperCase(codeLeft);
      codeRight = upperCase(codeRight);

      if (codeLeft != codeRight)
        return static_cast<String::Length>(codeLeft) - codeRight;
    }
  }
  return lengthLeft - lengthRight;
}

bool sameText(char const* const left, String::Length const lengthLeft, char const* const right,
  String::Length const lengthRight, String::Length const length)
{
  return compareText(left, lengthLeft, right, lengthRight, length) == 0;
}

String::Length findStr(String const& string, String const& match, String::Length position,
  String::Length length)
{
  String::Length const stringLength = string.length();

  if (!length)
    length = stringLength;

  if (position < 0)
  {
    length += position;
    position = 0;
  }
  if (position + length > stringLength)
  {
    length = math::max<String::Length>(stringLength - position, 0);
    position = math::min(position, stringLength);
  }
  if (length)
    if (String::Length const matchLength = match.length())
    {
      String::Length const matchMaxLength = math::min(position + length, stringLength) - matchLength;
      char const* matchData = match.data();
      char const* stringData = string.data();

      for (String::Length i = position; i <= matchMaxLength; ++i)
        if (::memcmp(matchData, stringData + i, matchLength) == 0)
          return i;
    }

  return String::NotFound;
}

String::Length findStr(String const& string, char const* const match, String::Length const position,
  String::Length const length)
{
  return findStr(string, String::Wrap(match), position, length);
}

String::Length findStrLast(String const& string, String const& match, String::Length position,
  String::Length length)
{
  String::Length const stringLength = string.length();

  if (!length)
    length = stringLength;

  if (position < 0)
  {
    length += position;
    position = 0;
  }
  if (position + length > stringLength)
  {
    length = math::max<String::Length>(stringLength - position, 0);
    position = math::min(position, stringLength);
  }
  String::Length index = String::NotFound;

  if (length)
    if (String::Length const matchLength = match.length())
    {
      String::Length const matchMaxLength = math::min(position + length, stringLength) - matchLength;
      char const* matchData = match.data();
      char const* stringData = string.data();

      for (String::Length i = position; i <= matchMaxLength; ++i)
        if (::memcmp(matchData, stringData + i, matchLength) == 0)
        {
          index = i;
          i += matchLength - 1;
        }
    }

  return index;
}

String::Length findStrLast(String const& string, char const* const match, String::Length const position,
  String::Length const length)
{
  return findStrLast(string, String::Wrap(match), position, length);
}

String::Length findText(String const& string, String const& match, String::Length position,
  String::Length length)
{
  String::Length const stringLength = string.length();

  if (!length)
    length = stringLength;

  if (position < 0)
  {
    length += position;
    position = 0;
  }
  if (position + length > stringLength)
  {
    length = math::max<String::Length>(stringLength - position, 0);
    position = math::min(position, stringLength);
  }
  if (length)
    if (String::Length const matchLength = match.length())
    {
      String::Length const matchMaxLength = math::min(position + length, stringLength) - matchLength;
      char const* matchData = match.data();
      char const* stringData = string.data();

      for (String::Length j = position; j <= matchMaxLength; ++j)
      {
        bool matched = true;

        for (String::Length i = 0; i < matchLength; ++i)
          if (upperCase(matchData[i]) != upperCase(stringData[j + i]))
          {
            matched = false;
            break;
          }

        if (matched)
          return j;
      }
    }

  return String::NotFound;
}

String::Length findText(String const& string, char const* const match, String::Length const position,
  String::Length const length)
{
  return findText(string, String::Wrap(match), position, length);
}

String::Length findTextLast(String const& string, String const& match, String::Length position,
  String::Length length)
{
  String::Length const stringLength = string.length();

  if (!length)
    length = stringLength;

  if (position < 0)
  {
    length += position;
    position = 0;
  }
  if (position + length > stringLength)
  {
    length = math::max<String::Length>(stringLength - position, 0);
    position = math::min(position, stringLength);
  }
  String::Length index = String::NotFound;

  if (length)
    if (String::Length const matchLength = match.length())
    {
      String::Length const matchMaxLength = math::min(position + length, stringLength) - matchLength;
      char const* matchData = match.data();
      char const* stringData = string.data();

      for (String::Length j = position; j <= matchMaxLength; ++j)
      {
        bool matched = true;

        for (String::Length i = 0; i < matchLength; ++i)
          if (upperCase(matchData[i]) != upperCase(stringData[j + i]))
          {
            matched = false;
            break;
          }

        if (matched)
        {
          index = j;
          j += matchLength - 1;
        }
      }
    }

  return index;
}

String::Length findTextLast(String const& string, char const* const match, String::Length const position,
  String::Length const length)
{
  return findTextLast(string, String::Wrap(match), position, length);
}

String::Length findChar(String const& string, char const charCode, String::Length position,
  String::Length length)
{
  String::Length const stringLength = string.length();

  if (!length)
    length = stringLength;

  if (position < 0)
  {
    length += position;
    position = 0;
  }
  if (position + length > stringLength)
  {
    length = math::max<String::Length>(stringLength - position, 0);
    position = math::min(position, stringLength);
  }
  if (length != 0)
  {
    char const* data = string.data() + position;

    for (String::Length i = 0; i < length; ++i)
      if (data[i] == charCode)
        return position + i;
  }
  return String::NotFound;
}

String::Length findCharLast(String const& string, char const charCode, String::Length position,
  String::Length length)
{
  String::Length const stringLength = string.length();

  if (!length)
    length = stringLength;

  if (position < 0)
  {
    length += position;
    position = 0;
  }
  if (position + length > stringLength)
  {
    length = math::max<String::Length>(stringLength - position, 0);
    position = math::min(position, stringLength);
  }

  String::Length index = String::NotFound;

  if (length)
  {
    char const* data = string.data() + position;

    for (String::Length i = 0; i < length; ++i)
      if (data[i] == charCode)
        index = position + i;
  }
  return index;
}

bool containsStr(String const& string, String const& match, String::Length const position,
  String::Length const length)
{
  return findStr(string, match, position, length) != String::NotFound;
}

bool containsStr(String const& string, char const* const match, String::Length const position,
  String::Length const length)
{
  return findStr(string, String::Wrap(match), position, length) != String::NotFound;
}

bool containsText(String const& string, String const& match, String::Length const position,
  String::Length const length)
{
  return findText(string, match, position, length) != String::NotFound;
}

bool containsText(String const& string, char const* const match, String::Length const position,
  String::Length const length)
{
  return findText(string, String::Wrap(match), position, length) != String::NotFound;
}

bool startsWith(String const& string, String const& match, String::Length position)
{
  position = math::max<String::Length>(position, 0);
  String::Length const matchLength = match.length();

  if (matchLength && (string.length() - position >= matchLength))
    return ::memcmp(string.data() + position, match.data(), matchLength) == 0;
  else
    return false; // Empty match or insufficient space in string for the match.
}

bool startsWith(String const& string, char const* const match, String::Length const position)
{
  return startsWith(string, String::Wrap(match), position);
}

bool startsWithText(String const& string, String const& match, String::Length position)
{
  position = math::max<String::Length>(position, 0);
  String::Length const matchLength = match.length();

  if (matchLength && (string.length() - position >= matchLength))
  {
    char const* stringContents = string.data();
    char const* matchContents = match.data();

    for (String::Length i = 0; i < matchLength; ++i)
    {
      char codeString = stringContents[i], codeMatch = matchContents[i];
      if (codeString != codeMatch)
      {
        codeString = upperCase(codeString);
        codeMatch = upperCase(codeMatch);

        if (codeString != codeMatch)
          return false; // String doesn't start with a match.
      }
    }
    return true;
  }
  else
    return false; // Empty match or insufficient space in string for the match.
}

bool startsWithText(String const& string, char const* const match, String::Length const position)
{
  return startsWithText(string, String::Wrap(match), position);
}

bool endsWith(String const& string, String const& match)
{
  if (!string.empty() && !match.empty() && string.length() >= match.length())
  {
    String::Length const matchLength = match.length();
    return ::memcmp(string.data() + string.length() - matchLength, match.data(), matchLength) == 0;
  }
  else
    return false;
}

bool endsWith(String const& string, char const* match)
{
  return endsWith(string, String::Wrap(match));
}

bool endsWithText(String const& string, String const& match)
{
  if (!string.empty() && !match.empty() && string.length() >= match.length())
  {
    String::Length const matchLength = match.length();
    char const* stringContents = string.data() + string.length() - matchLength;
    char const* matchContents = match.data();

    for (String::Length i = 0; i < matchLength; ++i)
    {
      char codeString = stringContents[i], codeMatch = matchContents[i];
      if (codeString != codeMatch)
      {
        codeString = upperCase(codeString);
        codeMatch = upperCase(codeMatch);

        if (codeString != codeMatch)
          return false; // String doesn't start with a match.
      }
    }
    return true;
  }
  else
    return false;
}

bool endsWithText(String const& string, char const* match)
{
  return endsWithText(string, String::Wrap(match));
}

String& searchReplace(String& string, char const match, char const replacement)
{
  if (string.unwrap())
  {
    String::Length const length = string.length();
    char* contents = string.data();

    for (String::Length position = 0; position < length; ++position)
      if (contents[position] == match)
      {
        contents[position] = replacement;
        break;
      }
  }
  else
    string.pollute();
  return string;
}

String& searchReplaceAll(String& string, char const match, char const replacement)
{
  if (string.unwrap())
  {
    String::Length const length = string.length();
    char* contents = string.data();

    for (String::Length position = 0; position < length; ++position)
      if (contents[position] == match)
        contents[position] = replacement;
  }
  else
    string.pollute();
  return string;
}

String& searchReplace(String& string, String const& match, String const& replacement)
{
  String::Length const matchLength = match.length();
  if (matchLength)
  {
    if (String::Length const position = findStr(string, match); position != String::NotFound)
    {
      string.replace(replacement, position, matchLength, 0, replacement.length());
      if (!replacement)
        string.pollute();
    }
  }
  if (!match)
    string.pollute();
  return string;
}

String& searchReplaceAll(String& string, String const& match, String const& replacement)
{
  if (String::Length const matchLength = match.length())
  {
    String::Length const replacementLength = replacement.length();

    for (String::Length position = 0; (position = findStr(string, match, position)) != String::NotFound; )
    {
      if (!string.replace(replacement, position, matchLength, 0, replacementLength))
        break; // Stop replace process to avoid infinite loop.
      position += replacementLength;
    }
  }
  if (!match)
    string.pollute();
  return string;
}

String& searchReplaceText(String& string, String const& match, String const& replacement)
{
  String::Length const matchLength = match.length();
  if (matchLength)
  {
    if (String::Length const position = findText(string, match); position != String::NotFound)
      string.replace(replacement, position, matchLength, 0, replacement.length());
  }
  if (!match)
    string.pollute();
  return string;
}

String& searchReplaceTextAll(String& string, String const& match, String const& replacement)
{
  if (String::Length const matchLength = match.length())
  {
    String::Length const replacementLength = replacement.length();

    for (String::Length position = 0; (position = findText(string, match, position)) != String::NotFound; )
    {
      if (!string.replace(replacement, position, matchLength, 0, replacementLength))
        break; // Stop replace process to avoid infinite loop.
      position += replacementLength;
    }
  }
  if (!match)
    string.pollute();
  return string;
}

String& searchEraseAll(String& string, String const& match)
{
  if (String::Length const matchLength = match.length())
  {
    for (String::Length position = 0; (position = findStr(string, match, position)) != String::NotFound; )
      string.erase(position, matchLength);
  }
  if (!match)
    string.pollute();
  return string;
}

// Number conversions.

bool strToInt(int64_t& dest, String const& string, int32_t const base)
{
  dest = ::strtoll(string.data(), nullptr, base);
  return dest || (errno != EINVAL && errno != ERANGE);
}

int64_t strToInt(String const& string, int64_t const defaultValue, int32_t const base)
{
  int64_t value;
  if (!strToInt(value, string, base))
    value = defaultValue;
  return value;
}

String intToStr(int64_t value, int32_t const base)
{
  static char const* const baseChars =
    "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

  String string;

  if (base >= 2 && base <= 36)
  {
    int64_t prev;

    do {
      prev = value;
      value /= base;
      string.append(baseChars[35 + (prev - value * base)]);
    } while (value);

    if (prev < 0)
      string.append('-');

    char* last = &string.last(), *first = &string.first();

    while (first < last)
    {
      char temp = *last;
      *last--= *first;
      *first++ = temp;
    }
  }
  else
    string.pollute();

  return string;
}

bool strToFloat(float& dest, String const& string)
{
  char* endp = const_cast<char*>(string.data());
  dest = strtof(string.data(), &endp);
  return endp != string.data();
}

float strToFloat(String const& string, float const defaultValue)
{
  float value;
  if (!strToFloat(value, string))
    value = defaultValue;
  return value;
}

String floatToStr(float const value)
{
  String string;

  if (string.length(snprintf(nullptr, 0, "%.6g", value)))
    snprintf(string.data(), string.length() + 1, "%.6g", value);
  else
    string.pollute();

  return string;
}

bool strToDouble(double& dest, String const& string)
{
  char* endp = const_cast<char*>(string.data());
  dest = strtod(string.data(), &endp);
  return endp != string.data();
}

double strToDouble(String const& string, double const defaultValue)
{
  double value;
  if (!strToDouble(value, string))
    value = defaultValue;
  return value;
}

String doubleToStr(double const value)
{
  String string;

  if (string.length(snprintf(nullptr, 0, "%.11g", value)))
    snprintf(string.data(), string.length(), "%.11g", value);
  else
    string.pollute();

  return string;
}

// Case functions.

String upperCase(String const& string)
{
  String text;

  if (text.length(string.length()))
  {
    char const* source = string.data();
    char* dest = text.data();

    for (String::Length i = 0; i < text.length(); ++i)
      dest[i] = upperCase(source[i]);
  }
  else
    text.pollute();

  return text;
}

String lowerCase(String const& string)
{
  String text;

  if (text.length(string.length()))
  {
    char const* source = string.data();
    char* dest = text.data();

    for (String::Length i = 0; i < text.length(); ++i)
      dest[i] = lowerCase(source[i]);
  }
  else
    text.pollute();

  return text;
}

// Unicode utilities.

String::Length convertUTF16ToUTF8(char* const dest, WideString::WideChar const* const source,
  WideString::Length const sourceLength)
{
  if (!source)
    return 0;

#ifdef _WIN32
  int length = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWSTR>(source),
    static_cast<int>(sourceLength), nullptr, 0, nullptr, nullptr);
  if (length <= 0 || !dest)
    return math::max(length, 0);

  length = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWSTR>(source), static_cast<int>(sourceLength),
    dest, length, nullptr, nullptr);

  return math::max(length, 0);
#else
  if (!dest)
    return sourceLength * 4; // Overestimate, no way to know the actual length before the conversion.

  iconv_t const cd = iconv_open("UTF-8", "UTF-16LE");
  if (cd == reinterpret_cast<iconv_t>(-1))
    return String::NotFound;

  size_t inBytesLeft = static_cast<size_t>(sourceLength) * sizeof(String::WideChar);
  size_t outBytesLeft = sourceLength * 4;

  char* bufferIn = reinterpret_cast<char*>(const_cast<WideString::WideChar*>(source));
  char* bufferOut = dest;

  size_t const res = iconv(cd, &bufferIn, &inBytesLeft, &bufferOut, &outBytesLeft);
  iconv_close(cd);

  if (res == static_cast<size_t>(-1))
    return String::NotFound;

  return bufferOut - bufferIn;
#endif
}

WideString::Length convertUTF8ToUTF16(WideString::WideChar* const dest, char const* const source,
  String::Length const sourceLength)
{
  if (!source)
    return 0;

#ifdef _WIN32
  int length = MultiByteToWideChar(CP_UTF8, 0, source, static_cast<int>(sourceLength), nullptr, 0);
  if (length <= 0 || !dest)
    return math::max(length, 0);

  length = MultiByteToWideChar(CP_UTF8, 0, source, static_cast<int>(sourceLength),
    reinterpret_cast<LPWSTR>(dest), length);

  return math::max(length, 0);
#else
  if (!dest)
    return sourceLength * 2; // Overestimate, no way to know the actual length before the conversion.

  iconv_t const cd = iconv_open("UTF-16LE", "UTF-8");
  if (cd == reinterpret_cast<iconv_t>(-1))
    return String::NotFound;

  size_t inBytesLeft = sourceLength;
  size_t outBytesLeft = sourceLength * 2;

  char* bufferIn = const_cast<char*>(source);
  char* bufferOut = reinterpret_cast<char*>(dest);

  size_t const res = iconv(cd, &bufferIn, &inBytesLeft, &bufferOut, &outBytesLeft);
  iconv_close(cd);

  if (res == static_cast<size_t>(-1))
    return String::NotFound;

  return (bufferOut - bufferIn) / sizeof(WideString::WideChar*);
#endif
}

// Path utilities.

static String::Length fileNameStart(String const& filePath)
{
  String::Length lastDelimPos = findCharLast(filePath, PathDelimeter);
#ifdef _WIN32
  lastDelimPos = lastDelimPos != String::NotFound ? lastDelimPos : findCharLast(filePath, ':');
#endif
  return lastDelimPos != String::NotFound ? lastDelimPos + 1 : 0;
}

String fixFilePath(String filePath)
{
#ifdef _WIN32
  char constexpr const wrongDelimeter = '/';
#else
  char constexpr const wrongDelimeter = '\\';
#endif

  searchReplaceAll(filePath, wrongDelimeter, utility::PathDelimeter);
  return filePath;
}

String appendFileSubPath(String filePath, String const& subPath)
{
  if (!filePath.empty() && filePath.last() != utility::PathDelimeter)
    filePath.append(utility::PathDelimeter);

  filePath.append(subPath);
  return filePath;
}

String extractFileName(String const& filePath)
{
  return filePath.substr(fileNameStart(filePath));
}

String extractFilePath(String const& filePath)
{
  String::Length const start = fileNameStart(filePath);
  return start > 0 ? filePath.substr(0, start) : String::Empty;
}

String extractFileExtension(String const& filePath)
{
  String::Length const start = fileNameStart(filePath);
  String::Length const dot = findCharLast(filePath, '.');

  return
    dot != String::NotFound && dot > start && dot != filePath.length() - 1 ?
    filePath.substr(dot) : String::Empty;
}

String changeFileExtension(String const& filePath, String const& extension)
{
  if (!filePath.empty())
  {
    String::Length const start = fileNameStart(filePath);
    String::Length const dot = findCharLast(filePath, '.');

    String newFilePath(filePath);

    if (dot != String::NotFound && dot > start)
      newFilePath.replace(extension, dot);
    else
      newFilePath.append(extension);

    return newFilePath;
  }
  else
    return filePath;
}

// Service functions.

String::Length StringComparer::operator () (String const& left, String const& right) const
{
  return compareStr(left, right);
}

String::Length TextComparer::operator () (String const& left, String const& right) const
{
  return compareText(left, right);
}

} // namespace utility

// Utility functions

uint16_t byteSwap16(uint16_t const value) noexcept
{
#ifdef _MSC_VER
  return _byteswap_ushort(value);
#else
  return __builtin_bswap16(value);
#endif
}

#ifdef __PLATFORM_BIG_ENDIAN

uint32_t byteSwap32(uint32_t const value) noexcept
{
#ifdef _MSC_VER
  return _byteswap_ulong(value);
#else
  return __builtin_bswap32(value);
#endif
}

uint64_t byteSwap64(uint64_t const value) noexcept
{
#ifdef _MSC_VER
  return _byteswap_uint64(value);
#else
  return __builtin_bswap64(value);
#endif
}

#endif

} // namespace trl