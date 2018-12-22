#ifndef PANTOR_INJA_POLYFILL_HPP
#define PANTOR_INJA_POLYFILL_HPP


#if __cplusplus < 201402L

#include <cstddef>
#include <type_traits>
#include <utility>


namespace stdinja {
  template<class T> struct _Unique_if {
    typedef std::unique_ptr<T> _Single_object;
  };

  template<class T> struct _Unique_if<T[]> {
    typedef std::unique_ptr<T[]> _Unknown_bound;
  };

  template<class T, size_t N> struct _Unique_if<T[N]> {
    typedef void _Known_bound;
  };

  template<class T, class... Args>
  typename _Unique_if<T>::_Single_object
  make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }

  template<class T>
  typename _Unique_if<T>::_Unknown_bound
  make_unique(size_t n) {
    typedef typename std::remove_extent<T>::type U;
    return std::unique_ptr<T>(new U[n]());
  }

  template<class T, class... Args>
  typename _Unique_if<T>::_Known_bound
  make_unique(Args&&...) = delete;
}

#else

namespace stdinja = std;

#endif // memory */


#endif // PANTOR_INJA_POLYFILL_HPP
