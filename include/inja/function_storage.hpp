// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_FUNCTION_STORAGE_HPP_
#define INCLUDE_INJA_FUNCTION_STORAGE_HPP_

#include <vector>

#include "node.hpp"
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
  constexpr static int VARIAIDC {-1};

  struct FunctionData {
    FunctionNode::Operation operation;

    CallbackFunction callback;
  };

  std::map<std::pair<std::string, int>, FunctionData> function_storage;

public:
  void add_function(nonstd::string_view name, int num_args, FunctionNode::Operation op) {
    function_storage.emplace(std::make_pair(name, num_args), FunctionData { op });
  }

  void add_callback(nonstd::string_view name, int num_args, const CallbackFunction &callback) {
    function_storage.emplace(std::make_pair(name, num_args), FunctionData { FunctionNode::Operation::Callback, callback });
  }

  FunctionData find_function(nonstd::string_view name, int num_args) const {
    auto it = function_storage.find(std::make_pair(static_cast<std::string>(name), num_args));
    if (it != function_storage.end()) {
      return it->second;
    
    // Find variadic function
    } else if (num_args > 0) {
      it = function_storage.find(std::make_pair(static_cast<std::string>(name), VARIAIDC));
      if (it != function_storage.end()) {
        return it->second;
      }
    }

    return { FunctionNode::Operation::None };
  }
};

} // namespace inja

#endif // INCLUDE_INJA_FUNCTION_STORAGE_HPP_
