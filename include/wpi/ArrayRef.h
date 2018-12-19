//===- ArrayRef.h - Array Reference Wrapper ---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef WPIUTIL_WPI_ARRAYREF_H
#define WPIUTIL_WPI_ARRAYREF_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>


namespace wpi {
  /// ArrayRef - Represent a constant reference to an array (0 or more elements
  /// consecutively in memory), i.e. a start pointer and a length.  It allows
  /// various APIs to take consecutive elements easily and conveniently.
  ///
  /// This class does not own the underlying data, it is expected to be used in
  /// situations where the data resides in some other buffer, whose lifetime
  /// extends past that of the ArrayRef. For this reason, it is not in general
  /// safe to store an ArrayRef.
  ///
  /// This is intended to be trivially copyable, so it should be passed by
  /// value.
  template<typename T>
  class ArrayRef {
  public:
    using iterator = const T *;
    using const_iterator = const T *;
    using size_type = size_t;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using value_type = T;

  private:
    /// The start of the array, in an external buffer.
    const T *Data = nullptr;

    /// The number of elements.
    size_type Length = 0;

  public:
    /// @name Constructors
    /// @{

    /// Construct an empty ArrayRef.
    /*implicit*/ ArrayRef() = default;

    /// Construct an ArrayRef from a single element.
    /*implicit*/ ArrayRef(const T &OneElt)
      : Data(&OneElt), Length(1) {}

    /// Construct an ArrayRef from a pointer and length.
    /*implicit*/ ArrayRef(const T *data, size_t length)
      : Data(data), Length(length) {}

    /// Construct an ArrayRef from a range.
    ArrayRef(const T *begin, const T *end)
      : Data(begin), Length(end - begin) {}

    /// Construct an ArrayRef from a std::vector.
    template<typename A>
    /*implicit*/ ArrayRef(const std::vector<T, A> &Vec)
      : Data(Vec.data()), Length(Vec.size()) {}

    /// Construct an ArrayRef from a std::array
    template <size_t N>
    /*implicit*/ constexpr ArrayRef(const std::array<T, N> &Arr)
        : Data(Arr.data()), Length(N) {}

    /// Construct an ArrayRef from a C array.
    template <size_t N>
    /*implicit*/ constexpr ArrayRef(const T (&Arr)[N]) : Data(Arr), Length(N) {}

    /// Construct an ArrayRef from a std::initializer_list.
    /*implicit*/ ArrayRef(const std::initializer_list<T> &Vec)
    : Data(Vec.begin() == Vec.end() ? (T*)nullptr : Vec.begin()),
      Length(Vec.size()) {}

    /// Construct an ArrayRef<const T*> from ArrayRef<T*>. This uses SFINAE to
    /// ensure that only ArrayRefs of pointers can be converted.
    template <typename U>
    ArrayRef(
        const ArrayRef<U *> &A,
        typename std::enable_if<
           std::is_convertible<U *const *, T const *>::value>::type * = nullptr)
      : Data(A.data()), Length(A.size()) {}

    /// Construct an ArrayRef<const T*> from std::vector<T*>. This uses SFINAE
    /// to ensure that only vectors of pointers can be converted.
    template<typename U, typename A>
    ArrayRef(const std::vector<U *, A> &Vec,
             typename std::enable_if<
                 std::is_convertible<U *const *, T const *>::value>::type* = 0)
      : Data(Vec.data()), Length(Vec.size()) {}

    /// @}
    /// @name Simple Operations
    /// @{

    iterator begin() const { return Data; }
    iterator end() const { return Data + Length; }

    reverse_iterator rbegin() const { return reverse_iterator(end()); }
    reverse_iterator rend() const { return reverse_iterator(begin()); }

    /// empty - Check if the array is empty.
    bool empty() const { return Length == 0; }

    const T *data() const { return Data; }

    /// size - Get the array size.
    size_t size() const { return Length; }

    /// front - Get the first element.
    const T &front() const {
      assert(!empty());
      return Data[0];
    }

    /// back - Get the last element.
    const T &back() const {
      assert(!empty());
      return Data[Length-1];
    }

    // copy - Allocate copy in Allocator and return ArrayRef<T> to it.
    template <typename Allocator> ArrayRef<T> copy(Allocator &A) {
      T *Buff = A.template Allocate<T>(Length);
      std::uninitialized_copy(begin(), end(), Buff);
      return ArrayRef<T>(Buff, Length);
    }

    /// equals - Check for element-wise equality.
    bool equals(ArrayRef RHS) const {
      if (Length != RHS.Length)
        return false;
      return std::equal(begin(), end(), RHS.begin());
    }

    /// slice(n, m) - Chop off the first N elements of the array, and keep M
    /// elements in the array.
    ArrayRef<T> slice(size_t N, size_t M) const {
      assert(N+M <= size() && "Invalid specifier");
      return ArrayRef<T>(data()+N, M);
    }

    /// slice(n) - Chop off the first N elements of the array.
    ArrayRef<T> slice(size_t N) const { return slice(N, size() - N); }

    /// Drop the first \p N elements of the array.
    ArrayRef<T> drop_front(size_t N = 1) const {
      assert(size() >= N && "Dropping more elements than exist");
      return slice(N, size() - N);
    }

    /// Drop the last \p N elements of the array.
    ArrayRef<T> drop_back(size_t N = 1) const {
      assert(size() >= N && "Dropping more elements than exist");
      return slice(0, size() - N);
    }

    /// Return a copy of *this with the first N elements satisfying the
    /// given predicate removed.
    template <class PredicateT> ArrayRef<T> drop_while(PredicateT Pred) const {
      return ArrayRef<T>(find_if_not(*this, Pred), end());
    }

    /// Return a copy of *this with the first N elements not satisfying
    /// the given predicate removed.
    template <class PredicateT> ArrayRef<T> drop_until(PredicateT Pred) const {
      return ArrayRef<T>(find_if(*this, Pred), end());
    }

    /// Return a copy of *this with only the first \p N elements.
    ArrayRef<T> take_front(size_t N = 1) const {
      if (N >= size())
        return *this;
      return drop_back(size() - N);
    }

    /// Return a copy of *this with only the last \p N elements.
    ArrayRef<T> take_back(size_t N = 1) const {
      if (N >= size())
        return *this;
      return drop_front(size() - N);
    }

    /// Return the first N elements of this Array that satisfy the given
    /// predicate.
    template <class PredicateT> ArrayRef<T> take_while(PredicateT Pred) const {
      return ArrayRef<T>(begin(), find_if_not(*this, Pred));
    }

    /// Return the first N elements of this Array that don't satisfy the
    /// given predicate.
    template <class PredicateT> ArrayRef<T> take_until(PredicateT Pred) const {
      return ArrayRef<T>(begin(), find_if(*this, Pred));
    }

    /// @}
    /// @name Operator Overloads
    /// @{
    const T &operator[](size_t Index) const {
      assert(Index < Length && "Invalid index!");
      return Data[Index];
    }

    /// Disallow accidental assignment from a temporary.
    ///
    /// The declaration here is extra complicated so that "arrayRef = {}"
    /// continues to select the move assignment operator.
    template <typename U>
    typename std::enable_if<std::is_same<U, T>::value, ArrayRef<T>>::type &
    operator=(U &&Temporary) = delete;

    /// Disallow accidental assignment from a temporary.
    ///
    /// The declaration here is extra complicated so that "arrayRef = {}"
    /// continues to select the move assignment operator.
    template <typename U>
    typename std::enable_if<std::is_same<U, T>::value, ArrayRef<T>>::type &
    operator=(std::initializer_list<U>) = delete;

    /// @}
    /// @name Expensive Operations
    /// @{
    std::vector<T> vec() const {
      return std::vector<T>(Data, Data+Length);
    }

    /// @}
    /// @name Conversion operators
    /// @{
    operator std::vector<T>() const {
      return std::vector<T>(Data, Data+Length);
    }

    /// @}
  };

  /// @name ArrayRef Convenience constructors
  /// @{

  /// Construct an ArrayRef from a std::vector.
  template<typename T>
  ArrayRef<T> makeArrayRef(const std::vector<T> &Vec) {
    return Vec;
  }

  /// @}

} // end namespace wpi

#endif // LLVM_ADT_ARRAYREF_H
