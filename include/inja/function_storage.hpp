// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_FUNCTION_STORAGE_HPP_
#define INCLUDE_INJA_FUNCTION_STORAGE_HPP_

#include <vector>

#include "bytecode.hpp"
#include "string_view.hpp"


namespace inja {

using json = nlohmann::json;

using Arguments = std::vector<const json*>;
using CallbackFunction = std::function<json(Arguments& args)>;

#define INJA_VARARGS (unsigned int) (~0) // use special number for VARARGS functions

/*!
 * \brief Class for builtin functions and user-defined callbacks.
 */
class FunctionStorage {
 public:
  void add_builtin(nonstd::string_view name, unsigned int num_args, Bytecode::Op op) {
    auto& data = get_or_new(name, num_args);
    data.op = op;
  }

  void add_callback(nonstd::string_view name, unsigned int num_args, const CallbackFunction& function) {
    auto& data = get_or_new(name, num_args);
    data.function = function;
  }

  Bytecode::Op find_builtin(nonstd::string_view name, unsigned int num_args) const {
    if (auto ptr = get(name, num_args)) {
      return ptr->op;
    }
    return Bytecode::Op::Nop;
  }

  CallbackFunction find_callback(nonstd::string_view name, unsigned int num_args) const {
    if (auto ptr = get(name, num_args)) {
      return ptr->function;
    }
    return nullptr;
  }

 private:
  struct FunctionData {
    unsigned int num_args {0};
    Bytecode::Op op {Bytecode::Op::Nop}; // for builtins
    CallbackFunction function; // for callbacks
  };

  FunctionData& get_or_new(nonstd::string_view name, unsigned int num_args) {
    auto &vec = m_map[static_cast<std::string>(name)];
    for (auto &i: vec) {
      if (i.num_args == num_args) return i;
    }
    vec.emplace_back();
    vec.back().num_args = num_args;
    return vec.back();
  }

  const FunctionData* get(nonstd::string_view name, unsigned int num_args) const {
    auto it = m_map.find(static_cast<std::string>(name));
    if (it == m_map.end()) return nullptr;
    const FunctionData* var_func = nullptr;
    for (auto &&i: it->second) {
      if (i.num_args == num_args) return &i; // function with precise number of argument(s) should always be used first
      else if (i.num_args == INJA_VARARGS) var_func = &i; // store the VARARGS function for later use
    }
    return var_func;
  }

  std::map<std::string, std::vector<FunctionData>> m_map;
};

}

#endif  // INCLUDE_INJA_FUNCTION_STORAGE_HPP_
