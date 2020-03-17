// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_UTILS_HPP_
#define INCLUDE_INJA_UTILS_HPP_

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "string_view.hpp"


namespace inja {

struct TextPosition {
  size_t line;
  size_t offset;
};

struct InjaError : public std::runtime_error {
  InjaError(const std::string& type, const std::string& message)
    : std::runtime_error("[inja.exception." + type + "] " + message) { }
};

inline void inja_throw(const std::string& type, const std::string& message) {
  throw InjaError(type, message);
}

inline void inja_throw(const std::string& type, const std::string& message, TextPosition pos) {
  throw InjaError(type, "(at " + std::to_string(pos.line) + ":" + std::to_string(pos.offset) + ") " + message);
}

inline std::ifstream open_file_or_throw(const std::string& path) {
  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try {
    file.open(path);
  } catch(const std::ios_base::failure& /*e*/) {
    inja_throw("file_error", "failed accessing file at '" + path + "'");
  }
  return file;
}

namespace string_view {
  inline nonstd::string_view slice(nonstd::string_view view, size_t start, size_t end) {
    start = std::min(start, view.size());
    end = std::min(std::max(start, end), view.size());
    return view.substr(start, end - start);  // StringRef(Data + Start, End - Start);
  }

  inline std::pair<nonstd::string_view, nonstd::string_view> split(nonstd::string_view view, char Separator) {
    size_t idx = view.find(Separator);
    if (idx == nonstd::string_view::npos) {
      return std::make_pair(view, nonstd::string_view());
    }
    return std::make_pair(slice(view, 0, idx), slice(view, idx + 1, nonstd::string_view::npos));
  }

  inline bool starts_with(nonstd::string_view view, nonstd::string_view prefix) {
    return (view.size() >= prefix.size() && view.compare(0, prefix.size(), prefix) == 0);
  }
}  // namespace string_view

}  // namespace inja

#endif  // INCLUDE_INJA_UTILS_HPP_
