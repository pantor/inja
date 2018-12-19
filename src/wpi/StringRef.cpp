//===-- StringRef.cpp - Lightweight String References ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <bitset>
#include <climits>
#include <ostream>

#include "wpi/StringRef.h"


using namespace wpi;

/// find - Search for the first string \arg Str in the string.
///
/// \return - The index of the first occurrence of \arg Str, or npos if not
/// found.
size_t StringRef::find(StringRef Str, size_t From) const noexcept {
  if (From > Length)
    return npos;

  const char *Start = Data + From;
  size_t Size = Length - From;

  const char *Needle = Str.data();
  size_t N = Str.size();
  if (N == 0)
    return From;
  if (Size < N)
    return npos;
  if (N == 1) {
    const char *Ptr = (const char *)::memchr(Start, Needle[0], Size);
    return Ptr == nullptr ? npos : Ptr - Data;
  }

  const char *Stop = Start + (Size - N + 1);

  // For short haystacks or unsupported needles fall back to the naive algorithm
  if (Size < 16 || N > 255) {
    do {
      if (std::memcmp(Start, Needle, N) == 0)
        return Start - Data;
      ++Start;
    } while (Start < Stop);
    return npos;
  }

  // Build the bad char heuristic table, with uint8_t to reduce cache thrashing.
  uint8_t BadCharSkip[256];
  std::memset(BadCharSkip, N, 256);
  for (unsigned i = 0; i != N-1; ++i)
    BadCharSkip[(uint8_t)Str[i]] = N-1-i;

  do {
    uint8_t Last = Start[N - 1];
    if (LLVM_UNLIKELY(Last == (uint8_t)Needle[N - 1]))
      if (std::memcmp(Start, Needle, N - 1) == 0)
        return Start - Data;

    // Otherwise skip the appropriate number of bytes.
    Start += BadCharSkip[Last];
  } while (Start < Stop);

  return npos;
}

/// find_first_of - Find the first character in the string that is in \arg
/// Chars, or npos if not found.
///
/// Note: O(size() + Chars.size())
StringRef::size_type StringRef::find_first_of(StringRef Chars,
                                              size_t From) const noexcept {
  std::bitset<1 << CHAR_BIT> CharBits;
  for (size_type i = 0; i != Chars.size(); ++i)
    CharBits.set((unsigned char)Chars[i]);

  for (size_type i = std::min(From, Length), e = Length; i != e; ++i)
    if (CharBits.test((unsigned char)Data[i]))
      return i;
  return npos;
}

/// find_last_of - Find the last character in the string that is in \arg C,
/// or npos if not found.
///
/// Note: O(size() + Chars.size())
StringRef::size_type StringRef::find_last_of(StringRef Chars,
                                             size_t From) const noexcept {
  std::bitset<1 << CHAR_BIT> CharBits;
  for (size_type i = 0; i != Chars.size(); ++i)
    CharBits.set((unsigned char)Chars[i]);

  for (size_type i = std::min(From, Length) - 1, e = -1; i != e; --i)
    if (CharBits.test((unsigned char)Data[i]))
      return i;
  return npos;
}

std::ostream &wpi::operator<<(std::ostream &os, StringRef string) {
  os.write(string.data(), string.size());
  return os;
}
