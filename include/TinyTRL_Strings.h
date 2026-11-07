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

// TinyTRL_Strings.h
#pragma once

#include "TinyTRL_Math.h"

namespace trl {

// Forward declaration of WideString.
class WideString;

/// String class that handles ASCII or UTF8-encoded strings with Short String Optimization that can store
/// up to 22 characters on 64-bit platforms and up to 10 characters on 32-bit platforms. This implementation
/// always appends null terminating character to the end of string, ensuring that \c String::data() always
/// points to a null-terminated string.
class String
{
public:
  /// UTF-16 character type.
#if WCHAR_MAX == UINT16_MAX
  typedef wchar_t WideChar;
#else
  typedef char16_t WideChar;
#endif

  /// String type used to store the actual size bits.
#ifdef __PLATFORM_X64
  typedef uint64_t Size;
#else
  typedef uint32_t Size;
#endif

  /// String type used to store the length.
#ifdef __PLATFORM_X64
  typedef int64_t Length;
#else
  typedef int32_t Length;
#endif

  /// Maximum available string length.
#ifdef __PLATFORM_X64
  static Length constexpr const MaxLength = INT64_MAX - 1;
#else
  static Length constexpr const MaxLength = INT32_MAX - 1;
#endif

  /// Constant that indicates index not found.
  static Length constexpr const NotFound = -1;

  /// Creates an empty string.
  String();

  /// Creates a new string instance copying contents from an existing null-terminated string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  String(char const* string);

#if __cplusplus > 201811L

  /// Creates a new string instance copying contents from an existing null-terminated string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  String(char8_t const* string);

#endif

  /// Creates a new string instance from an existing UTF-16 string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  explicit String(WideString const& string);

  /// Creates a new string instance consisting in a single character.
  explicit String(char charCode);

  /// Creates a new string instance being a copy of another string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  String(String const& source);

  /// Creates a new string instance and moves into it the contents of source string.
  String(String&& source) noexcept;

  /// Copies contents of source string to the current one.
  /// Note: an unsuccessful memory allocation pollutes the string.
  String& operator = (String const& source);

  /// Moves contents of source string to the current one.
  String& operator = (String&& source) noexcept;

  /// Creates a new string instance filled by a number of consecutive copies of character.
  /// Note: an unsuccessful memory allocation pollutes the string.
  [[nodiscard]] static String Fill(Length length, char fill = 0);

  /// Creates a new string instance copying contents from an existing buffer containing a string that
  /// may optionally be null-terminated. If source buffer is not null-terminated, then up to \c length
  /// characters will be copied. Note: an unsuccessful memory allocation pollutes the string.
  [[nodiscard]] static String FromBuffer(char const* buffer, Length length);

  /// Creates a new string instance copying contents from an existing data that is not null-terminated.
  /// Note: an unsuccessful memory allocation pollutes the string.
  [[nodiscard]] static String FromRawBytes(char const* buffer, Length length);

  /// Creates a new string instance wrapping an existing C null-terminated string. If \c length is greater
  /// than zero, then it should contain an actual string length that must match output of "strlen".
  /// If a string to be wrapped fits into a short string, or is not properly aligned, then it will be copied.
  /// Note: providing invalid parameters pollutes the string.
  [[nodiscard]] static String Wrap(char const* string, Length length = 0);

#if __cplusplus > 201811L

  /// Creates a new string instance copying contents from an existing buffer containing a string that
  /// may optionally be null-terminated. If source buffer is not null-terminated, then up to \c length
  /// characters will be copied. Note: an unsuccessful memory allocation pollutes the string.
  [[nodiscard]] static String FromBuffer(char8_t const* buffer, Length length);

  /// Creates a new string instance copying contents from an existing data that is not null-terminated.
  /// Note: an unsuccessful memory allocation pollutes the string.
  [[nodiscard]] static String FromRawBytes(char8_t const* buffer, Length length);

  /// Creates a new string instance wrapping an existing C null-terminated string. If \c length is greater
  /// than zero, then it should contain an actual string length that must match output of "strlen".
  /// If a string to be wrapped fits into a short string, then it will be copied. Note: an unsuccessful
  /// memory allocation pollutes the string.
  [[nodiscard]] static String Wrap(char8_t const* string, Length length = 0);

#endif

  /// Creates an empty invalid (polluted) string.
  [[nodiscard]] static String Invalid();

  /// Represents an empty string.
  static String const Empty;

  /// Default string destructor.
  ~String();

  /// Appends a character to the string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  String& operator += (char suffix);

  /// Appends a zero-terminated string to the current one.
  /// Note: an unsuccessful memory allocation pollutes the string.
  String& operator += (char const* suffix);

  /// Appends another string to current one.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if another string is polluted,
  /// the current string becomes polluted as well.
  String& operator += (String const& suffix);

  /// Concatenates string and a character.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if the first string is polluted,
  /// the current string becomes polluted as well.
  friend String operator + (String const& prefix, char suffix);

  /// Concatenates a character and string.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if the second string is polluted,
  /// the current string becomes polluted as well.
  friend String operator + (char prefix, String const& suffix);

  /// Concatenates string and a zero-terminated string.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if the first string is polluted,
  /// the current string becomes polluted as well.
  friend String operator + (String const& prefix, char const* suffix);

  /// Concatenates a zero-terminated string and another string.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if the second string is polluted,
  /// the current string becomes polluted as well.
  friend String operator + (char const* prefix, String const& suffix);

  /// Concatenates two text strings.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if at least one of two given
  /// strings is polluted, the current string becomes polluted as well.
  friend String operator + (String const& prefix, String const& suffix);

  /// Returns pointer to string contents.
  /// Note: using this function to access a wrapped string results in an undefined behavior.
  [[nodiscard]] char* data();

  /// Returns a constant pointer to string contents.
  [[nodiscard]] char const* data() const;

  /// Provides addressing of string as if it was an array (without bounds checking).
  /// Note: using this function to access a wrapped string results in an undefined behavior.
  [[nodiscard]] char& operator [] (Length index);

  /// Provides addressing of string as if it was a constant array (without bounds checking).
  [[nodiscard]] char const& operator [] (Length index) const;

  /// Returns reference to first element in the string.
  /// Note: using this function to access a wrapped string results in an undefined behavior.
  [[nodiscard]] char& first();

  /// Returns constant reference to first element in the string.
  [[nodiscard]] char const& first() const;

  /// Returns reference to last element in the string.
  /// Note: using this function to access a wrapped string results in an undefined behavior.
  [[nodiscard]] char& last();

  /// Returns constant reference to last element in the string.
  [[nodiscard]] char const& last() const;

  /// Returns constant pointer to the first character in the string.
  /// Note: this function is provided to enable C++11 ranged for and should not be used otherwise.
  char const* begin() const;

  /// Returns constant pointer to one character past last in the string.
  /// Note: this function is provided to enable C++11 ranged for and should not be used otherwise.
  char const* end() const;

  /// Returns pointer to the first character in the string.
  /// Note: this function is provided to enable C++11 ranged for and should not be used otherwise.
  /// Also, using this function to access a wrapped string results in an undefined behavior.
  char* begin();

  /// Returns pointer to one character past last in the string.
  /// Note: this function is provided to enable C++11 ranged for and should not be used otherwise.
  /// Also, using this function to access a wrapped string results in an undefined behavior.
  char* end();

  /// Tests whether a string is not polluted. A polluted string has an error bit set after a memory
  /// allocation failure. A string operation where at least one string parameter is polluted will result
  /// in a polluted string. Therefore, this polluted state propagates and persists until is validated
  /// and string is cleared.
  [[nodiscard]] explicit operator bool () const;

  /// Returns string capacity.
  [[nodiscard]] Length capacity() const;

  /// Changes string capacity.
  [[nodiscard]] bool capacity(Length capacity);

  /// Returns string length.
  [[nodiscard]] Length length() const;

  /// Changes string length.
  [[nodiscard]] bool length(Length length);

  /// Tests whether string is empty.
  [[nodiscard]] bool empty() const;

  /// Indicates whether a string is wrapped.
  [[nodiscard]] bool wrapped() const;

  /// Sets an error bit in the string, marking it as polluted.
  String& pollute();

  /// Resets error bit in the string, removing pollute status.
  String& unpollute();

  /// Sets string length to zero and resets error flag removing pollute status while preserving currently
  /// allocated capacity.
  void clear();

  /// Clears securely the contents of the string and resets its length (along with pollute status).
  bool burn();

  /// Reallocates string to fit its contents.
  [[nodiscard]] bool shrink();

  /// If string is wrapped, converts it to a non-wrapped state.
  [[nodiscard]] bool unwrap();

  /// Assigns contents from source string to the current one.
  /// If memory allocation fails during this operation, the contents of current string are unchanged.
  [[nodiscard]] bool assign(String const& source);

  /// Appends a character to current string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  String& append(char suffix);

  /// Appends a zero-terminated string to current one.
  /// Note: an unsuccessful memory allocation pollutes the string.
  String& append(char const* suffix);

  /// Appends another string to current one.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if the given string is polluted,
  /// the curent string becomes polluted as well.
  String& append(String const& suffix);

  /// Prepends a character to the start of current string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  String& prepend(char prefix);

  /// Prepends a zero-terminated string to the start of current one.
  /// Note: an unsuccessful memory allocation pollutes the string.
  String& prepend(char const* prefix);

  /// Prepends another string to the start of current one.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if another string is polluted,
  /// the current string becomes polluted as well.
  String& prepend(String const& prefix);

  /// Concatenates string and a character to current string overwriting its contents.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if prefix string is polluted,
  /// the current string becomes polluted as well.
  String& concatenate(String const& prefix, char suffix);

  /// Concatenates character and a string to current string overwriting its contents.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if suffix string is polluted,
  /// the current string becomes polluted as well.
  String& concatenate(char prefix, String const& suffix);

  /// Concatenates two strings and saves result to current string overwriting its contents.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if one of the strings is polluted,
  /// the current string becomes polluted as well.
  String& concatenate(String const& prefix, String const& suffix);

  /// Concatenates two strings and saves result to current string overwriting its contents.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if prefix string is polluted,
  /// the current string becomes polluted as well.
  String& concatenate(String const& prefix, char const* suffix);

  /// Concatenates two strings and saves result to current string overwriting its contents.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if suffix string is polluted,
  /// the current string becomes polluted as well.
  String& concatenate(char const* prefix, String const& suffix);

  /// Copies certain number of characters from source string to current one.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if the given source string is
  /// polluted, the current string becomes polluted as well.
  String& copy(String const& source, Length sourcePosition = 0, Length sourceLength = NotFound);

  /// Creates a string being a subset of current string.
  /// Note: an unsuccessful memory allocation pollutes the string. Also, if the given source string is
  /// polluted, the returned string is polluted as well.
  String substr(Length position = 0, Length length = NotFound) const;

  /// Replaces a certain portion of current string with a portion from source string.
  String& replace(String const& source, Length position = 0, Length length = NotFound,
    Length sourcePosition = 0, Length sourceLength = NotFound);

  /// Inserts a certain portion of source string into the current one.
  String& insert(String const& source, Length position, Length sourcePosition, Length sourceLength);

  /// Inserts a source string into the current one.
  String& insert(String const& source, Length position);

  /// Inserts a character into string at the given position.
  [[nodiscard]] bool insert(char charCode, Length position);

  /// Erases a certain portion of current string.
  String& erase(Length position, Length length = NotFound);

  /// Stores up to "destLen" characters from string to destination buffer. If the string is shorter than the
  /// destination length, then the remaining characters will be filled with zeroes. Returns the actual number
  /// of characters copied.
  Length store(void* dest, Length destLen) const;

protected:
  /// Appends a character to the end of current string.
  [[nodiscard]] bool internalAppend(char suffix);

  /// Appends another string to current one.
  [[nodiscard]] bool internalAppend(String const& suffix);

  /// Prepends a character to the start of string.
  [[nodiscard]] bool internalPrepend(char prefix);

  /// Prepends a prefix to the start of string.
  [[nodiscard]] bool internalPrepend(String const& prefix);

  /// Concatenates string and a character to current string overwriting its contents.
  [[nodiscard]] bool internalConcatenate(String const& prefix, char suffix);

  /// Concatenates character and a string to current string overwriting its contents.
  [[nodiscard]] bool internalConcatenate(char prefix, String const& suffix);

  /// Concatenates two strings and saves result to current string overwriting its contents.
  [[nodiscard]] bool internalConcatenate(String const& prefix, String const& suffix);

  /// Copies certain number of characters from source string to current one.
  [[nodiscard]] bool internalCopy(String const& source, Length sourcePosition, Length sourceLength);

  /// Replaces a certain portion of current string with another string.
  [[nodiscard]] bool internalReplace(String const& source, Length position, Length length,
    Length sourcePosition, Length sourceLength);

  /// Inserts a certain portion of source string into the current one.
  [[nodiscard]] bool internalInsert(String const& source, Length position, Length sourcePosition,
    Length sourceLength);

private:
  // Length of short string that can be stored on the stack.
#ifdef __PLATFORM_X64
  static Length constexpr const ShortLength = 24;
#else
  static Length constexpr const ShortLength = 12;
#endif

  // Practical capacity of short string.
  static Length constexpr const ShortCapacity = ShortLength - 1;

  // Byte offset for "short" length.
  static uint8_t constexpr const ShortLengthOffset = ShortLength - 1;

  // Bit mask that contains short string length.
  static uint8_t constexpr const ShortLengthMask = 0x3Fu;

  // Bit that designates a long string.
  static uint8_t constexpr const ShortLongBit = 0x80u;

  // Bit that designates an allocation error during string operation in short string representation.
  static uint8_t constexpr const ShortPolluteBit = 0x40u;

  // Bits that designate long string length.
#ifdef __PLATFORM_X64
  static Size constexpr const LongLengthMask = 0x7FFFFFFFFFFFFFFFull;
#else
  static Size constexpr const LongLengthMask = 0x7FFFFFFFul;
#endif

  // Bit that designates either a long string or an error (pollute) bit.
#ifdef __PLATFORM_X64
  static Size constexpr const LongBit = 0x8000000000000000ull;
#else
  static Size constexpr const LongBit = 0x80000000ul;
#endif

  // Length of null terminating character.
  static Length constexpr const NullLength = 1;

  // String container.
  union {
    struct {
      // Pointer to a memory location containing the string.
      char* _chars;

      // Capacity of the string, except the last (most significant) bit, which is considered a "pollute bit".
      // Also, when "long bit" is set, and capacity is zero, then the string is treated as "wrapped".
      Size _capacity;

      // Length of the string, except the last (most significant) bit, which is considered a "long bit" and
      // its presence indicates that the string is dynamically allocated.
      Size _length;
    };
    // Raw string bytes.
    uint8_t _bytes[ShortLength];
  };

  // Increases string capacity as necessary to match the desired one, without overflow check.
  // The actual capacity is always one character bigger than the requested one to contain null character.
  bool internalCapacity(Length capacity);

  /// Reallocates string to a new capacity. Optionally copies null terminating character during
  /// reallocations. \c copyNullLength parameter must be either 0 or 1.
  bool reallocate(Length capacity, Length copyNullLength = NullLength);

  // Writes a new length value.
  void writeLength(Length length);

  // Returns the length of long string.
  Length longLength() const;

  // Returns the length of short string.
  Length shortLength() const;

  // Tests whether a string is wrapped.
  // Note: This call doesn't check if string is short and assumes that it doesn't.
  bool wrappedBit() const;

  // Tests whether short bit is set in the string.
  bool longBit() const;

  // Loads length field endian-aware.
  Size endianLength() const;

  // Stores length field endian-aware.
  void endianLength(Size length);

  // Loads capacity field endian-aware.
  Size endianCapacity() const;

  // Stores capacity field endian-aware.
  void endianCapacity(Size capacity);

  // Securely erases the contents of string.
  static void secureErase(char* string, Length length);
};

/// UTF-16 string class implementation, commonly required on Windows for WinAPI calls. This implementation
/// always appends null terminating character to the end of string, ensuring that \c WideString::data()
/// always points to a null-terminated string.
class WideString
{
public:
  using WideChar = String::WideChar;
  using Size = String::Size;
  using Length = String::Length;

  /// Maximum available string length.
#ifdef __PLATFORM_X64
  static Length constexpr const MaxLength = INT64_MAX - 1;
#else
  static Length constexpr const MaxLength = INT32_MAX - 1;
#endif

  /// Constant that indicates index not found.
  static Length constexpr const NotFound = String::NotFound;

  /// Creates an empty string.
  WideString();

  /// Creates a new string instance from the contents of an UTF-8 string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  explicit WideString(String const& string);

  /// Creates a new string instance from the contents of an existing null-terminated UTF-8 C string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  explicit WideString(char const* string);

#if __cplusplus > 201811L
  /// Creates a new string instance from the contents of an existing null-terminated UTF-8 C string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  explicit WideString(char8_t const* string);
#endif

  /// Creates a new string instance from the contents of an existing null-terminated UTF-16 C string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  WideString(WideChar const* string);

  /// Creates a new string instance being a copy of another string.
  /// Note: an unsuccessful memory allocation pollutes the string.
  WideString(WideString const& source);

  /// Creates a new string instance and moves into it the contents of source string.
  WideString(WideString&& source) noexcept;

  /// Creates a new string instance copying contents from an existing buffer containing a string that
  /// may optionally be null-terminated. If source buffer is not null-terminated, then up to \c length
  /// characters will be copied. Note: an unsuccessful memory allocation pollutes the string.
  [[nodiscard]] static WideString FromBuffer(WideChar const* buffer, Length length);

  /// Creates a new string instance copying contents from an existing buffer containing a string that
  /// may optionally be null-terminated. If source buffer is not null-terminated, then up to \c length
  /// characters will be copied. A byte-swap is performed while copying the buffer.
  /// Note: an unsuccessful memory allocation pollutes the string.
  [[nodiscard]] static WideString FromBufferByteSwap(WideChar const* buffer, Length length);

  /// Creates a new string instance filled by a number of consecutive copies of a single character.
  /// Note: an unsuccessful memory allocation pollutes the string.
  [[nodiscard]] static WideString Fill(Length length, WideChar fill = 0);

  /// Wraps an existing zero-terminated string.
  [[nodiscard]] static WideString Wrap(WideChar const* string);

  /// Copies contents of source string to the current one.
  /// Note: an unsuccessful memory allocation pollutes the string.
  WideString& operator = (WideString const& source);

  /// Moves contents of source string to the current one.
  WideString& operator = (WideString&& source) noexcept;

  /// Default string destructor.
  ~WideString();

  /// Returns pointer to string contents.
  /// Note: using this function to access a wrapped string results in an undefined behavior.
  [[nodiscard]] WideChar* data();

  /// Returns a constant pointer to string contents.
  [[nodiscard]] WideChar const* data() const;

  /// Provides addressing of string as if it was an array (without bounds checking).
  /// Note: using this function to access a wrapped string results in an undefined behavior.
  [[nodiscard]] WideChar& operator [] (Length index);

  /// Provides addressing of string as if it was a constant array (without bounds checking).
  [[nodiscard]] WideChar const& operator [] (Length index) const;

  /// Returns reference to first element in the string.
  /// Note: using this function to access a wrapped string results in an undefined behavior.
  [[nodiscard]] WideChar& first();

  /// Returns constant reference to first element in the string.
  [[nodiscard]] WideChar const& first() const;

  /// Returns reference to last element in the string.
  /// Note: using this function to access a wrapped string results in an undefined behavior.
  [[nodiscard]] WideChar& last();

  /// Returns constant reference to last element in the string.
  /// Also, using this function to access a wrapped string results in an undefined behavior.
  [[nodiscard]] WideChar const& last() const;

  /// Returns constant pointer to the first character in the string.
  /// Note: this function is provided to enable C++11 ranged for and should not be used otherwise.
  /// Also, using this function to access a wrapped string results in an undefined behavior.
  WideChar const* begin() const;

  /// Returns constant pointer to one character past last in the string.
  /// Note: this function is provided to enable C++11 ranged for and should not be used otherwise.
  /// Also, using this function to access a wrapped string results in an undefined behavior.
  WideChar const* end() const;

  /// Returns pointer to the first character in the string.
  /// Note: this function is provided to enable C++11 ranged for and should not be used otherwise.
  /// Also, using this function to access a wrapped string results in an undefined behavior.
  WideChar* begin();

  /// Returns pointer to one character past last in the string.
  /// Note: this function is provided to enable C++11 ranged for and should not be used otherwise.
  /// Also, using this function to access a wrapped string results in an undefined behavior.
  WideChar* end();

  /// Tests whether a string is not polluted. A polluted string has an error bit set after a memory
  /// allocation failure. A string operation where at least one string parameter is polluted will result
  /// in a polluted string. Therefore, this polluted state propagates and persists until is validated
  /// and/or string is cleared.
  [[nodiscard]] explicit operator bool() const;

  /// Returns string length.
  [[nodiscard]] Length length() const;

  /// Changes string length.
  [[nodiscard]] bool length(Length length);

  /// Tests whether string is empty.
  [[nodiscard]] bool empty() const;

  /// Sets an error bit in the string, marking it as polluted.
  void pollute();

  /// Releases the string and sets its length to zero.
  void clear();

  /// Clears securely the contents of the string.
  bool burn();

  /// Indicates whether a string is wrapped.
  [[nodiscard]] bool wrapped() const;

protected:
  // Resizes string to a new length.
  bool resize(Length length);

private:
  // Bits that designate string length.
#ifdef __PLATFORM_X64
  static Size constexpr const LongLengthMask = 0x7FFFFFFFFFFFFFFFull;
#else
  static Size constexpr const LongLengthMask = 0x7FFFFFFFul;
#endif

  // Bit that designates an error (pollute) bit.
#ifdef __PLATFORM_X64
  static Size constexpr const LongPolluteBit = 0x8000000000000000ull;
#else
  static Size constexpr const LongPolluteBit = 0x80000000ul;
#endif

  // Pointer to a memory location containing the string.
  WideChar* _chars;

  // Length of the string (except last bit, which is an error bit).
  Size _length;

  // Returns the actual length of long string (without status bits).
  Length readLength() const;

  // Securely erases the contents of string.
  static void secureErase(WideChar* string, Length length);

  // Calculates number of characters in a null-terminated string up to the given length, if specified.
  // Note: this assumes string is a valid pointer.
  static Length calculateLength(WideChar const* string, Length length = 0);
};

// String helper operators.

/// Tests whether first string is lexicographically less than the second one.
extern bool operator < (String const& left, String const& right);

/// Tests whether first string is lexicographically greater than the second one.
extern bool operator > (String const& left, String const& right);

/// Tests whether first string is lexicographically less than or equal to the second one.
extern bool operator <= (String const& left, String const& right);

/// Tests whether first string is lexicographically greater than or equal to the second one.
extern bool operator >= (String const& left, String const& right);

/// Tests whether first string is lexicographically equal to the second one.
extern bool operator == (String const& left, String const& right);

/// Tests whether first string is lexicographically different to the second one.
extern bool operator != (String const& left, String const& right);

namespace utility {

// Character utilities.

/// Converts the specified ANSI character code to upper case.
extern unsigned char upperCase(unsigned char charCode);

/// Converts the specified ANSI character code to lower case.
extern unsigned char lowerCase(unsigned char charCode);

// String utilities.

/// Calculates number of characters in the stream up to the given length.
/// Note: this assumes string is a valid pointer and length is greater than zero.
extern String::Length calculateLength(char const* string, String::Length length = 0);

/// Compares two null-terminated strings lexicographically. If \c length parameter is specified, then up to
/// that number of characters will be compared.
extern String::Length compareStr(char const* left, char const* right, String::Length length = 0);

/// Tests whether two strings are same lexicographically. If \c length parameter is specified, then up to
/// that number of characters will be compared.
extern bool sameStr(char const* left, char const* right, String::Length length = 0);

/// Compares two strings lexicographically. If \c length parameter is specified, then up to that number of
/// characters will be compared.
extern String::Length compareStr(String const& left, String const& right, String::Length length = 0);

/// Tests whether two strings are the same lexicographically. If \c length parameter is specified, then up
/// to that number of characters will be compared.
extern bool sameStr(String const& left, String const& right, String::Length length = 0);

/// Compares two null-terminated text strings lexicographically without case-sensitivity (this only applies
/// to ASCII characters). If \c length parameter is specified, then up to that number of characters will
/// be compared.
extern String::Length compareText(char const* left, char const* right, String::Length length = 0);

/// Tests whether two null-terminated strings are the same lexicographically. If \c length parameter is
/// specified, then up to that number of characters will be compared.
extern bool sameText(char const* left, char const* right, String::Length length = 0);

/// Compares two text strings lexicographically without case-sensitivity (this only applies to ASCII
/// characters). If \c length parameter is specified, then up to that number of characters will be compared.
extern String::Length compareText(String const& leftText, String const& rightText,
  String::Length length = 0);

/// Tests whether two strings are the same lexicographically without case-sensitivity (this only applies
/// to ASCII characters). If \c length parameter is specified, then up to that number of characters will
/// be  compared.
extern bool sameText(String const& leftText, String const& rightText, String::Length length = 0);

/// Compares two null-terminated text strings with a known length lexicographically without
/// case-sensitivity (this only applies to ASCII characters). If \c length parameter is specified, then up
/// to that number of characters will be compared.
extern String::Length compareText(char const* left, String::Length lengthLeft, char const* right,
  String::Length lengthRight, String::Length length = 0);

/// Tests whether two null-terminated text strings with a known length are the same lexicographically
/// without case-sensitivity (this only applies to ASCII characters). If \c length parameter is specified,
/// then up to that number of characters will be compared.
extern bool sameText(char const* left, String::Length lengthLeft, char const* right,
  String::Length lengthRight, String::Length length = 0);

/// Searches for a string match in a given string starting at the given position and length, if such are
/// provided. Returns String::NotFound if match is not found.
extern String::Length findStr(String const& string, String const& match, String::Length position = 0,
  String::Length length = 0);

/// Searches for a null-terminated string match in a given string starting at the given position and length,
/// if such are provided. Returns String::NotFound if match is not found.
extern String::Length findStr(String const& string, char const* match, String::Length position = 0,
  String::Length length = 0);

/// Searches for last match of a string in another string starting at the given position and length,
/// if such is provided. Returns String::NotFound if match is not found.
extern String::Length findStrLast(String const& string, String const& match, String::Length position = 0,
  String::Length length = 0);

/// Searches for last match of a null-terminated string in another string  starting at the given position
/// and length, if such is provided. Returns String::NotFound if match is not found.
extern String::Length findStrLast(String const& string, char const* match, String::Length position = 0,
  String::Length length = 0);

/// Searches for a string match in a given string without case-sensitivity (this only applies to ASCII
/// characters) starting at the given position and length, if such are provided. Returns String::NotFound
/// if match is not found.
extern String::Length findText(String const& string, String const& match, String::Length position = 0,
  String::Length length = 0);

/// Searches for a null-terminated string match in a given string without case-sensitivity (this only
/// applies to ASCII characters) starting at the given position and length, if such are provided.
/// Returns String::NotFound if match is not found.
extern String::Length findText(String const& string, char const* match, String::Length position = 0,
  String::Length length = 0);

/// Searches for last match of a string in another string without case-sensitivity (this only applies to
/// ASCII characters) starting at the given position and length, if such is provided.
/// Returns String::NotFound if match is not found.
extern String::Length findTextLast(String const& string, String const& match, String::Length position = 0,
  String::Length length = 0);

/// Searches for last match of a null-terminated string in another string without case-sensitivity (this
/// only applies to ASCII characters) starting at the given position and length, if such is provided.
/// Returns String::NotFound if match is not found.
extern String::Length findTextLast(String const& string, char const* match, String::Length position = 0,
  String::Length length = 0);

/// Searches for a character match in given string starting at the given position and length,
/// if such are provided. Returns String::NotFound if match is not found.
extern String::Length findChar(String const& string, char charCode, String::Length position = 0,
  String::Length length = 0);

/// Searches for last character match in current string starting at the given position and length,
/// if such are provided. Returns String::NotFound if match is not found.
extern String::Length findCharLast(String const& string, char charCode, String::Length position = 0,
  String::Length length = 0);

/// Tests whether a given string contains match of a source string starting at the given position and
/// length, if such are provided.
extern bool containsStr(String const& string, String const& match, String::Length position = 0,
  String::Length length = 0);

/// Tests whether a given string contains match of a null-terminated source string starting at the given
/// position and length, if such are provided.
extern bool containsStr(String const& string, char const* match, String::Length position = 0,
  String::Length length = 0);

/// Tests whether a given string contains match of a source string without case-sensitivity (this only
/// applies to ASCII characters) starting at the given position and length, if such are provided.
extern bool containsText(String const& string, String const& match, String::Length position = 0,
  String::Length length = 0);

/// Tests whether a given string contains match of a null-terminated source string without case-sensitivity
/// (this only  applies to ASCII characters) starting at the given position and length, if such
/// are provided.
extern bool containsText(String const& string, char const* match, String::Length position = 0,
  String::Length length = 0);

/// Tests whether a given string has a match of a source string starting at the given position,
/// if such is provided.
extern bool startsWith(String const& string, String const& match, String::Length position = 0);

/// Tests whether a given string has a match of a null-terminated source string starting at the given
/// position, if such is provided.
extern bool startsWith(String const& string, char const* match, String::Length position = 0);

/// Tests whether a given string has a match of a source string without case-sensitivity (this only applies
/// to ASCII characters) starting at the given position, if such is provided.
extern bool startsWithText(String const& string, String const& match, String::Length position = 0);

/// Tests whether a given string has a match of a source null-terminated string without case-sensitivity
/// (this only applies to ASCII characters) starting at the given position, if such is provided.
extern bool startsWithText(String const& string, char const* match, String::Length position = 0);

/// Tests whether a given string ends with a given match of characters.
extern bool endsWith(String const& string, String const& match);

/// Tests whether a given string ends with a given match of a null-terminated characters.
extern bool endsWith(String const& string, char const* match);

/// Tests whether a given string ends with a given match of characters without case-sensitivity (this only
/// applies to ASCII characters).
extern bool endsWithText(String const& string, String const& match);

/// Tests whether a given string ends with a given match of a null-terminated characters without
/// case-sensitivity (this only applies to ASCII characters).
extern bool endsWithText(String const& string, char const* match);

/// Finds in a given string an instance of the specified character and replaces it with another character.
/// If a memory allocation failure occurs (when trying to unwrap a wrapped string), then an error bit will
/// be set and the given string will be polluted. Returns the same string that was passed as first parameter
/// for chaining.
extern String& searchReplace(String& string, char match, char replacement);

/// Finds in a given string any instances of the specified character and replaces each of them with another
/// character. If a memory allocation failure occurs (when trying to unwrap a wrapped string), then an error
/// bit will be set and the given string will be polluted. Returns the same string that was passed as first
/// parameter for chaining.
extern String& searchReplaceAll(String& string, char match, char replacement);

/// Finds in a given string an instance of the specified match and replaces it with the given replacement
/// string. If a memory allocation failure occurs, then an error bit will be set and the given string
/// will be polluted. Returns the same string that was passed as first parameter for chaining.
extern String& searchReplace(String& string, String const& match, String const& replacement);

/// Finds in a given string any instances of the specified match and replaces each of them with the given
/// replacement string. If a memory allocation failure occurs, then an error bit will be set and the given
/// string will be polluted. Returns the same string that was passed as first parameter for chaining.
extern String& searchReplaceAll(String& string, String const& match, String const& replacement);

/// Finds in a given string an instance of the specified match without case-sensitivity (this only
/// applies to ASCII characters) and replaces it with the given replacement string. If a memory allocation
/// failure occurs, then an error bit will be set and the given string will be polluted.
/// Returns the same string that was passed as first parameter for chaining.
extern String& searchReplaceText(String& string, String const& match, String const& replacement);

/// Finds in a given string any instances of the specified match without case-sensitivity (this only
/// applies to ASCII characters) and replaces each of them with the given replacement string. If a memory
/// allocation failure occurs, then an error bit will be set and the given string will be polluted.
/// Returns the same string that was passed as first parameter for chaining.
extern String& searchReplaceTextAll(String& string, String const& match, String const& replacement);

/// Finds and erases in a given string any instances of the specified match. Returns the same string that
/// was passed as first parameter for chaining.
extern String& searchEraseAll(String& string, String const& match);

/// Parses a given string and converts it to 64-bit integer.
extern bool strToInt(int64_t& dest, String const& string, int32_t base = 0);

/// Parses a given string and converts it to 64-bit integer. If the string contains invalid character
/// sequence, returns the specified default value instead.
extern int64_t strToInt(String const& string, int64_t defaultValue = 0, int32_t base = 0);

/// Creates a new string instance representing a 64-bit integer value.
extern String intToStr(int64_t value, int32_t base = 10);

/// Parses a given string and converts it to 32-bit floating-point number.
extern bool strToFloat(float& dest, String const& string);

/// Parses a given string and converts it to 32-bit floating-point number.
extern float strToFloat(String const& string, float defaultValue = 0.0f);

/// Creates a new string instance representing a 32-bit floating-point number.
extern String floatToStr(float value);

/// Parses a given string and converts it to 32-bit floating-point number.
extern bool strToDouble(double& dest, String const& string);

/// Parses a given string and converts it to 32-bit floating-point number.
extern double strToDouble(String const& string, double defaultValue = 0.0);

/// Creates a new string instance representing a 32-bit floating-point number.
extern String doubleToStr(double value);

/// Converts the specified ANSI-based string character codes to upper case.
extern String upperCase(String const& string);

/// Converts the specified ANSI-based string character codes to lower case.
extern String lowerCase(String const& string);

// Unicode conversion utilities.

/// Converts source UTF-16 text to UTF-8 with validation. Returns the actual number of bytes written.
/// If \c dest is NULL, then just calculates number of characters required.
/// In case of overflow, this function will not write beyond String::MaxLength bytes and will
/// return resulting length that is greater than MaxLength.
extern String::Length convertUTF16ToUTF8(char* dest, WideString::WideChar const* source,
  WideString::Length sourceLength);

/// Converts source UTF-8 text to UTF-16 with validation. Returns the actual number of bytes written.
/// If \c dest is NULL, then just calculates number of characters required.
/// In case of overflow, this function will not write beyond WideString::MaxLength bytes and will
/// return resulting length that is greater than WideString::MaxLength.
extern WideString::Length convertUTF8ToUTF16(WideString::WideChar* dest, char const* source,
  String::Length sourceLength);

// File path utilities.

/// Platform-specific path delimiter.
#ifdef _WIN32
  char constexpr const PathDelimeter = '\\';
  constexpr char const* EndLine = "\r\n";
#else
  char constexpr const PathDelimeter = '/';
  constexpr char const* EndLine = "\n";
#endif

/// Replaces any instances of wrong path delimiter with the correct one.
extern String fixFilePath(String filePath);

/// Appends a path deliminer (if such is not present already) and a sub-path to an existing path.
extern String appendFileSubPath(String filePath, String const& subPath);

/// Extracts pure file name from the given file path.
extern String extractFileName(String const& filePath);

/// Extracts file path (which may or may not include trailing path delimiter) from the given file name.
extern String extractFilePath(String const& filePath);

/// Extracts file extension from the given file name (which may or may not include path). The returned
/// extension will contain dot, for example ".exe". If the given file name doesn't have extension,
/// an empty string will be returned.
extern String extractFileExtension(String const& filePath);

/// Replaces extension in the given file name (which may or may not include path) with a new one.
/// Note that new extension should contain dot "." as the first character, otherwise the behavior may be
/// undefined. If new extension is an empty string, then new file name will have its extension
/// stripped out.
extern String changeFileExtension(String const& filePath, String const& extension);

// Service functors.

/// Case-sensitive string comparer.
struct StringComparer
{
  /// Compares two strings with case-sensitivity.
  String::Length operator () (String const& left, String const& right) const;
};

/// Case-insensitive string comparer.
struct TextComparer
{
  /// Compares two strings without case-sensitivity.
  String::Length operator () (String const& left, String const& right) const;
};

} // namespace utility
} // namespace trl