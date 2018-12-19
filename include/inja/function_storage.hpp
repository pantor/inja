#ifndef PANTOR_INJA_FUNCTION_STORAGE_HPP
#define PANTOR_INJA_FUNCTION_STORAGE_HPP

#include <string_view>

#include <inja/internal.hpp>


namespace inja {

class FunctionStorage {
 public:
  void AddBuiltin(std::string_view name, unsigned int numArgs, Bytecode::Op op) {
    auto& data = GetOrNew(name, numArgs);
    data.op = op;
  }

  void AddCallback(std::string_view name, unsigned int numArgs, const CallbackFunction& function) {
    auto& data = GetOrNew(name, numArgs);
    data.function = function;
  }

  Bytecode::Op FindBuiltin(std::string_view name, unsigned int numArgs) const {
    if (auto ptr = Get(name, numArgs))
      return ptr->op;
    else
      return Bytecode::Op::Nop;
  }

  CallbackFunction FindCallback(std::string_view name, unsigned int numArgs) const {
    if (auto ptr = Get(name, numArgs))
      return ptr->function;
    else
      return nullptr;
  }

 private:
  struct FunctionData {
    unsigned int numArgs = 0;
    Bytecode::Op op = Bytecode::Op::Nop; // for builtins
    CallbackFunction function; // for callbacks
  };

  FunctionData& GetOrNew(std::string_view name, unsigned int numArgs) {
    auto &vec = m_map[name.data()];
    for (auto &i : vec) {
      if (i.numArgs == numArgs) return i;
    }
    vec.emplace_back();
    vec.back().numArgs = numArgs;
    return vec.back();
  }

  const FunctionData* Get(std::string_view name, unsigned int numArgs) const {
    auto it = m_map.find(static_cast<std::string>(name));
    if (it == m_map.end()) return nullptr;
    for (auto &&i : it->second) {
      if (i.numArgs == numArgs) return &i;
    }
    return nullptr;
  }

  std::map<std::string, std::vector<FunctionData>> m_map;
};

}

#endif // PANTOR_INJA_FUNCTION_STORAGE_HPP