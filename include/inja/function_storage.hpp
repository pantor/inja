// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_FUNCTION_STORAGE_HPP_
#define INCLUDE_INJA_FUNCTION_STORAGE_HPP_

#include <vector>

#include "string_view.hpp"

namespace inja {

using json = nlohmann::json;

using Arguments = std::vector<const json *>;
using CallbackFunction = std::function<json(Arguments &args)>;

/*!
 * \brief Class for builtin functions and user-defined callbacks.
 */
class FunctionStorage {
public:
  enum class Operation {
    Not,
    And,
    Or,
    In,
    Equal,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Add,
    Subtract,
    Multiplication,
    Division,
    Power,
    Modulo,
    At,
    Default,
    DivisibleBy,
    Even,
    Exists,
    ExistsInObject,
    First,
    Float,
    Int,
    IsArray,
    IsBoolean,
    IsFloat,
    IsInteger,
    IsNumber,
    IsObject,
    IsString,
    Last,
    Length,
    Lower,
    Max,
    Min,
    Odd,
    Range,
    Round,
    Sort,
    Upper,
    Callback,
    ParenLeft,
    ParenRight,
    None,
  };

  const int VARIADIC {-1};

  struct FunctionData {
    Operation operation;

    CallbackFunction callback;
  };

  std::map<std::pair<std::string, int>, FunctionData> function_storage;

  // std::map<std::pair<std::string, int>, Operation> builtin_storage;
  // std::map<std::pair<std::string, int>, CallbackFunction> callback_storage;

public:
  void add_builtin(nonstd::string_view name, int num_args, Operation op) {
    function_storage.emplace(std::make_pair(name, num_args), FunctionData { op });
  }

  void add_callback(nonstd::string_view name, int num_args, const CallbackFunction &callback) {
    function_storage.emplace(std::make_pair(name, num_args), FunctionData { Operation::Callback, callback });
  }

  FunctionData find_function(nonstd::string_view name, int num_args) const {
    auto it = function_storage.find(std::make_pair(static_cast<std::string>(name), num_args));
    if (it != function_storage.end()) {
      return it->second;

    // Find variadic function
    } else if (num_args > 0) {
      it = function_storage.find(std::make_pair(static_cast<std::string>(name), VARIADIC));
      if (it != function_storage.end()) {
        return it->second;
      }
    }

    return { Operation::None };
  }
};

} // namespace inja

#endif // INCLUDE_INJA_FUNCTION_STORAGE_HPP_
