// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_EXCEPTIONS_HPP_
#define INCLUDE_INJA_EXCEPTIONS_HPP_

#include <stdexcept>
#include <string>

namespace inja {

struct SourceLocation {
  size_t line;
  size_t column;
};

struct InjaError : public std::runtime_error {
  std::string type;
  std::string message;

  bool has_location {false};
  SourceLocation location;

  InjaError(const std::string &type, const std::string &message)
      : std::runtime_error("[inja.exception." + type + "] " + message), type(type), message(message) {}

  InjaError(const std::string &type, const std::string &message, SourceLocation location)
      : std::runtime_error("[inja.exception." + type + "] (at " + std::to_string(location.line) + ":" +
                           std::to_string(location.column) + ") " + message),
        type(type), message(message), has_location(true), location(location) {}
};

struct ParserError : public InjaError {
  ParserError(const std::string &message) : InjaError("parser_error", message) {}
  ParserError(const std::string &message, SourceLocation location) : InjaError("parser_error", message, location) {}
};

struct RenderError : public InjaError {
  RenderError(const std::string &message) : InjaError("render_error", message) {}
  RenderError(const std::string &message, SourceLocation location) : InjaError("render_error", message, location) {}
};

struct FileError : public InjaError {
  FileError(const std::string &message) : InjaError("file_error", message) {}
  FileError(const std::string &message, SourceLocation location) : InjaError("file_error", message, location) {}
};

struct JsonError : public InjaError {
  JsonError(const std::string &message) : InjaError("json_error", message) {}
  JsonError(const std::string &message, SourceLocation location) : InjaError("json_error", message, location) {}
};

} // namespace inja

#endif // INCLUDE_INJA_EXCEPTIONS_HPP_
