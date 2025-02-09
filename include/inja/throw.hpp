#ifndef INCLUDE_INJA_THROW_HPP_
#define INCLUDE_INJA_THROW_HPP_

#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && !defined(INJA_NOEXCEPTION)
#ifndef INJA_THROW
#define INJA_THROW(exception) throw exception
#endif
#else
#include <cstdlib>
#ifndef INJA_THROW
#define INJA_THROW(exception) \
std::abort();                 \
    std::ignore = exception
#endif
#ifndef INJA_NOEXCEPTION
#define INJA_NOEXCEPTION
#endif
#endif

#endif // INCLUDE_INJA_THROW_HPP_
