//===- StringRef.h - Constant String Reference Wrapper ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef WPIUTIL_WPI_STRINGREF_H
#define WPIUTIL_WPI_STRINGREF_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "wpi/Compiler.h"


namespace wpi {

  class StringRef;

  /// Helper functions for StringRef::getAsInteger.
  bool getAsUnsignedInteger(StringRef Str, unsigned Radix,
                            unsigned long long &Result) noexcept;

  bool getAsSignedInteger(StringRef Str, unsigned Radix, long long &Result) noexcept;

  bool consumeUnsignedInteger(StringRef &Str, unsigned Radix,
                              unsigned long long &Result) noexcept;
  bool consumeSignedInteger(StringRef &Str, unsigned Radix, long long &Result) noexcept;

  /// StringRef - Represent a constant reference to a string, i.e. a character
  /// array and a length, which need not be null terminated.
  ///
  /// This class does not own the string data, it is expected to be used in
  /// situations where the character data resides in some other buffer, whose
  /// lifetime extends past that of the StringRef. For this reason, it is not in
  /// general safe to store a StringRef.
  class StringRef {
  public:
    static const size_t npos = ~size_t(0);

    using iterator = const char *;
    using const_iterator = const char *;
    using size_type = size_t;

  private:
    /// The start of the string, in an external buffer.
    const char *Data = nullptr;

    /// The length of the string.
    size_t Length = 0;

    // Workaround memcmp issue with null pointers (undefined behavior)
    // by providing a specialized version
    inline static int compareMemory(const char *Lhs, const char *Rhs, size_t Length) noexcept {
      if (Length == 0) { return 0; }
      return ::memcmp(Lhs,Rhs,Length);
    }

  public:
    /// @name Constructors
    /// @{

    /// Construct an empty string ref.
    /*implicit*/ StringRef() = default;

    /// Disable conversion from nullptr.  This prevents things like
    /// if (S == nullptr)
    StringRef(std::nullptr_t) = delete;

    /// Construct a string ref from a cstring.
    /*implicit*/ inline StringRef(const char *Str)
        : Data(Str), Length(Str ? ::strlen(Str) : 0) {}

    /// Construct a string ref from a pointer and length.
    /*implicit*/ inline constexpr StringRef(const char *data, size_t length)
        : Data(data), Length(length) {}

    /// Construct a string ref from an std::string.
    /*implicit*/ inline StringRef(const std::string &Str)
      : Data(Str.data()), Length(Str.length()) {}

    static StringRef withNullAsEmpty(const char *data) {
      return StringRef(data ? data : "");
    }

    /// @}
    /// @name Iterators
    /// @{

    iterator begin() const noexcept { return Data; }

    iterator end() const noexcept { return Data + Length; }

    const unsigned char *bytes_begin() const noexcept {
      return reinterpret_cast<const unsigned char *>(begin());
    }
    const unsigned char *bytes_end() const noexcept {
      return reinterpret_cast<const unsigned char *>(end());
    }

    /// @}
    /// @name String Operations
    /// @{

    /// data - Get a pointer to the start of the string (which may not be null
    /// terminated).

    inline const char *data() const noexcept { return Data; }

    /// empty - Check if the string is empty.

    inline bool empty() const noexcept { return size() == 0; }

    /// size - Get the string size.

    inline size_t size() const noexcept { return Length; }

    /// front - Get the first character in the string.

    char front() const noexcept {
      assert(!empty());
      return Data[0];
    }

    /// back - Get the last character in the string.

    char back() const noexcept {
      assert(!empty());
      return Data[Length-1];
    }

    // copy - Allocate copy in Allocator and return StringRef to it.
    template <typename Allocator>
     StringRef copy(Allocator &A) const {
      // Don't request a length 0 copy from the allocator.
      if (empty())
        return StringRef();
      char *S = A.template Allocate<char>(Length);
      std::copy(begin(), end(), S);
      return StringRef(S, Length);
    }

    /// equals - Check for string equality, this is more efficient than
    /// compare() when the relative ordering of inequal strings isn't needed.

    inline bool equals(StringRef RHS) const noexcept {
      return (Length == RHS.Length &&
              compareMemory(Data, RHS.Data, RHS.Length) == 0);
    }

    /// equals_lower - Check for string equality, ignoring case.

    bool equals_lower(StringRef RHS) const noexcept {
      return Length == RHS.Length && compare_lower(RHS) == 0;
    }

    /// compare - Compare two strings; the result is -1, 0, or 1 if this string
    /// is lexicographically less than, equal to, or greater than the \p RHS.

    inline int compare(StringRef RHS) const noexcept {
      // Check the prefix for a mismatch.
      if (int Res = compareMemory(Data, RHS.Data, std::min(Length, RHS.Length)))
        return Res < 0 ? -1 : 1;

      // Otherwise the prefixes match, so we only need to check the lengths.
      if (Length == RHS.Length)
        return 0;
      return Length < RHS.Length ? -1 : 1;
    }

    /// compare_lower - Compare two strings, ignoring case.

    int compare_lower(StringRef RHS) const noexcept;

    /// compare_numeric - Compare two strings, treating sequences of digits as
    /// numbers.

    int compare_numeric(StringRef RHS) const noexcept;

    /// str - Get the contents as an std::string.

    std::string str() const {
      if (!Data) return std::string();
      return std::string(Data, Length);
    }

    /// @}
    /// @name Operator Overloads
    /// @{

    char operator[](size_t Index) const noexcept {
      assert(Index < Length && "Invalid index!");
      return Data[Index];
    }

    /// Disallow accidental assignment from a temporary std::string.
    ///
    /// The declaration here is extra complicated so that `stringRef = {}`
    /// and `stringRef = "abc"` continue to select the move assignment operator.
    template <typename T>
    typename std::enable_if<std::is_same<T, std::string>::value,
                            StringRef>::type &
    operator=(T &&Str) = delete;

    /// @}
    /// @name Type Conversions
    /// @{

    operator std::string() const {
      return str();
    }

    /// @}
    /// @name String Predicates
    /// @{

    /// Check if this string starts with the given \p Prefix.

    inline bool startswith(StringRef Prefix) const noexcept {
      return Length >= Prefix.Length &&
             compareMemory(Data, Prefix.Data, Prefix.Length) == 0;
    }

    /// @}
    /// @name String Searching
    /// @{

    /// Search for the first character \p C in the string.
    ///
    /// \returns The index of the first occurrence of \p C, or npos if not
    /// found.

    inline size_t find(char C, size_t From = 0) const noexcept {
      size_t FindBegin = std::min(From, Length);
      if (FindBegin < Length) { // Avoid calling memchr with nullptr.
        // Just forward to memchr, which is faster than a hand-rolled loop.
        if (const void *P = ::memchr(Data + FindBegin, C, Length - FindBegin))
          return static_cast<const char *>(P) - Data;
      }
      return npos;
    }

    /// Search for the first character \p C in the string, ignoring case.
    ///
    /// \returns The index of the first occurrence of \p C, or npos if not
    /// found.

    size_t find_lower(char C, size_t From = 0) const noexcept;

    /// Search for the first string \p Str in the string.
    ///
    /// \returns The index of the first occurrence of \p Str, or npos if not
    /// found.

    size_t find(StringRef Str, size_t From = 0) const noexcept;

    /// Search for the first string \p Str in the string, ignoring case.
    ///
    /// \returns The index of the first occurrence of \p Str, or npos if not
    /// found.

    size_t find_lower(StringRef Str, size_t From = 0) const noexcept;

    /// Find the first character in the string that is \p C, or npos if not
    /// found. Same as find.

    size_t find_first_of(char C, size_t From = 0) const noexcept {
      return find(C, From);
    }

    /// Find the first character in the string that is in \p Chars, or npos if
    /// not found.
    ///
    /// Complexity: O(size() + Chars.size())
    size_t find_first_of(StringRef Chars, size_t From = 0) const noexcept;

    /// Find the last character in the string that is in \p C, or npos if not
    /// found.
    ///
    /// Complexity: O(size() + Chars.size())
    size_t find_last_of(StringRef Chars, size_t From = npos) const noexcept;

    /// @}
    /// @name Substring Operations
    /// @{

    /// Return a reference to the substring from [Start, Start + N).
    ///
    /// \param Start The index of the starting character in the substring; if
    /// the index is npos or greater than the length of the string then the
    /// empty substring will be returned.
    ///
    /// \param N The number of characters to included in the substring. If N
    /// exceeds the number of characters remaining in the string, the string
    /// suffix (starting with \p Start) will be returned.
    inline StringRef substr(size_t Start, size_t N = npos) const noexcept {
      Start = std::min(Start, Length);
      return StringRef(Data + Start, std::min(N, Length - Start));
    }

    /// Return a reference to the substring from [Start, End).
    ///
    /// \param Start The index of the starting character in the substring; if
    /// the index is npos or greater than the length of the string then the
    /// empty substring will be returned.
    ///
    /// \param End The index following the last character to include in the
    /// substring. If this is npos or exceeds the number of characters
    /// remaining in the string, the string suffix (starting with \p Start)
    /// will be returned. If this is less than \p Start, an empty string will
    /// be returned.
    StringRef slice(size_t Start, size_t End) const noexcept {
      Start = std::min(Start, Length);
      End = std::min(std::max(Start, End), Length);
      return StringRef(Data + Start, End - Start);
    }

    /// Split into two substrings around the first occurrence of a separator
    /// character.
    ///
    /// If \p Separator is in the string, then the result is a pair (LHS, RHS)
    /// such that (*this == LHS + Separator + RHS) is true and RHS is
    /// maximal. If \p Separator is not in the string, then the result is a
    /// pair (LHS, RHS) where (*this == LHS) and (RHS == "").
    ///
    /// \param Separator The character to split on.
    /// \returns The split substrings.
    std::pair<StringRef, StringRef> split(char Separator) const {
      size_t Idx = find(Separator);
      if (Idx == npos)
        return std::make_pair(*this, StringRef());
      return std::make_pair(slice(0, Idx), slice(Idx+1, npos));
    }

    /// @}
  };

  /// @name StringRef Comparison Operators
  /// @{

  inline bool operator==(StringRef LHS, StringRef RHS) noexcept {
    return LHS.equals(RHS);
  }

  inline bool operator!=(StringRef LHS, StringRef RHS) noexcept {
    return !(LHS == RHS);
  }

  inline bool operator<(StringRef LHS, StringRef RHS) noexcept {
    return LHS.compare(RHS) == -1;
  }

  inline bool operator<=(StringRef LHS, StringRef RHS) noexcept {
    return LHS.compare(RHS) != 1;
  }

  inline bool operator>(StringRef LHS, StringRef RHS) noexcept {
    return LHS.compare(RHS) == 1;
  }

  inline bool operator>=(StringRef LHS, StringRef RHS) noexcept {
    return LHS.compare(RHS) != -1;
  }

  inline bool operator==(StringRef LHS, const char *RHS) noexcept {
    return LHS.equals(StringRef(RHS));
  }

  inline bool operator!=(StringRef LHS, const char *RHS) noexcept {
    return !(LHS == StringRef(RHS));
  }

  std::ostream &operator<<(std::ostream &os, StringRef string);

  /// @}

  // StringRefs can be treated like a POD type.
  template <typename T> struct isPodLike;
  template <> struct isPodLike<StringRef> { static const bool value = true; };

  /// HashString - Hash function for strings.
  ///
  /// This is the Bernstein hash function.
  //
  // FIXME: Investigate whether a modified bernstein hash function performs
  // better: http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
  //   X*33+c -> X*33^c
  static inline unsigned HashString(StringRef Str, unsigned Result = 0) {
    for (StringRef::size_type i = 0, e = Str.size(); i != e; ++i)
      Result = Result * 33 + (unsigned char)Str[i];
    return Result;
  }

} // end namespace wpi

#endif // LLVM_ADT_STRINGREF_H
