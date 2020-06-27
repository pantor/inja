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
  struct FunctionData {
    unsigned int num_args {0};
    Node::Op op {Node::Op::Nop}; // for builtins
    CallbackFunction function; // for callbacks
  };

  std::map<std::string, std::vector<FunctionData>> storage;

  FunctionData &get_or_new(nonstd::string_view name, unsigned int num_args) {
    auto &vec = storage[static_cast<std::string>(name)];
    for (auto &i : vec) {
      if (i.num_args == num_args) {
        return i;
      }
    }
    vec.emplace_back();
    vec.back().num_args = num_args;
    return vec.back();
  }

  const FunctionData *get(nonstd::string_view name, unsigned int num_args) const {
    auto it = storage.find(static_cast<std::string>(name));
    if (it == storage.end()) {
      return nullptr;
    }

    for (auto &&i : it->second) {
      if (i.num_args == num_args) {
        return &i;
      }
    }
    return nullptr;
  }

public:
  void add_builtin(nonstd::string_view name, unsigned int num_args, Node::Op op) {
    auto &data = get_or_new(name, num_args);
    data.op = op;
  }

  void add_callback(nonstd::string_view name, unsigned int num_args, const CallbackFunction &function) {
    auto &data = get_or_new(name, num_args);
    data.function = function;
  }

  Node::Op find_builtin(nonstd::string_view name, unsigned int num_args) const {
    if (auto ptr = get(name, num_args)) {
      return ptr->op;
    }
    return Node::Op::Nop;
  }

  CallbackFunction find_callback(nonstd::string_view name, unsigned int num_args) const {
    if (auto ptr = get(name, num_args)) {
      return ptr->function;
    }
    return nullptr;
  }
};

} // namespace inja

#endif // INCLUDE_INJA_FUNCTION_STORAGE_HPP_
