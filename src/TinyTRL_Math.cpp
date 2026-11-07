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

#include "TinyTRL_Math.h"

#include <math.h>

namespace trl {
namespace math {

float fmod(float const x, float const y) noexcept
{
  return ::fmodf(x, y);
}

double fmod(double const x, double const y) noexcept
{
  return ::fmod(x, y);
}

float fma(float const x, float const y, float const z) noexcept
{
  return ::fmaf(x, y, z);
}

double fma(double const x, double const y, double const z) noexcept
{
  return ::fma(x, y, z);
}

} // namespace math
namespace utility {

// Shared bits between single-precision float and integer.
union FloatIntBits
{
  float valueFloat;
  uint32_t valueInt;
};

// Shared bits between double-precision and integer.
union DoubleIntBits
{
  double valueDouble;
  uint64_t valueInt;
};

uint32_t floatBitsAsUint(float const value)
{
  FloatIntBits bits;
  bits.valueFloat = value;
  return bits.valueInt;
}

float uintBitsAsFloat(uint32_t const value)
{
  FloatIntBits bits;
  bits.valueInt = value;
  return bits.valueFloat;
}

uint64_t doubleBitsAsUint(double const value)
{
  DoubleIntBits bits;
  bits.valueDouble = value;
  return bits.valueInt;
}

extern double uintBitsAsDouble(uint64_t const value)
{
  DoubleIntBits bits;
  bits.valueInt = value;
  return bits.valueDouble;
}

void assignNAN(float& dest)
{
  dest = uintBitsAsFloat(0x7FC00000u);
}

bool isInfinityOrNAN(float const value)
{
  uint32_t const bits = floatBitsAsUint(value);
  return (bits & 0x7F800000u) == 0x7F800000u;
}

bool isPlusOrMinusMaxFloat(float const value)
{
  uint32_t const bits = floatBitsAsUint(value);
  return bits == 0x7F7FFFFFu || bits == 0xFF7FFFFFu;
}

void assignNAN(double& dest)
{
  dest = uintBitsAsDouble(0x7FF8000000000000ull);
}

bool isInfinityOrNAN(double const value)
{
  uint64_t const bits = doubleBitsAsUint(value);
  return (bits & 0x7FF0000000000000ull) == 0x7FF0000000000000ull;
}

} // namespace utility
namespace math {

#ifdef __PLATFORM_X64
int32_t log2(uint64_t value) noexcept
{
#if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
  return value ? 8 * sizeof(unsigned long long) - __builtin_clzll(value) - 1 : 0;
#elif defined(_MSC_VER)
  unsigned long res;
  if (_BitScanReverse64(&res, value))
    return static_cast<int32_t>(res);
  else
    return 0;
#else
  #error "Unsupported compiler."
#endif
}
#else
int32_t log2(uint32_t value) noexcept
{
#if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
  return value ? 8 * sizeof(unsigned long) - __builtin_clzl(value) - 1 : 0;
#elif defined(_MSC_VER)
  unsigned long res;
  if (_BitScanReverse(&res, value))
    return static_cast<int32_t>(res);
  else
    return 0;
#else
  #error "Unsupported compiler."
#endif
}
#endif

size_t average(size_t value1, size_t value2) noexcept
{
  return (value1 & value2) + ((value1 ^ value2) >> 1);
}

} // namespace math
} // namespace trl