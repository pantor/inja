// Copyright (c) 2021 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_INJA_HPP_
#define INCLUDE_INJA_INJA_HPP_

#include <nlohmann/json.hpp>

#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && !defined(INJA_NOEXCEPTION)
  #ifndef INJA_THROW
    #define INJA_THROW(exception) throw exception
  #endif
#else
  #include <cstdlib>
  #ifndef INJA_THROW
    #define INJA_THROW(exception) std::abort(); std::ignore = exception
  #endif
  #ifndef INJA_NOEXCEPTION
    #define INJA_NOEXCEPTION
  #endif
#endif

#include "environment.hpp"
#include "exceptions.hpp"
#include "parser.hpp"
#include "renderer.hpp"
#include "string_view.hpp"
#include "template.hpp"

#endif // INCLUDE_INJA_INJA_HPP_
