/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#ifndef WPIUTIL_INJA_INTERNAL_H_
#define WPIUTIL_INJA_INTERNAL_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include <wpi/StringRef.h>

#include <inja/inja.hpp>


namespace inja {

using namespace wpi;

void inja_throw(const std::string& type, const std::string& message);

class Bytecode {
 public:
  enum class Op : uint8_t {
    Nop,
    // print StringRef (always immediate)
    PrintText,
    // print value
    PrintValue,
    // push value onto stack (always immediate)
    Push,

    // builtin functions
    // result is pushed to stack
    // args specify number of arguments
    // all functions can take their "last" argument either immediate
    // or popped off stack (e.g. if immediate, it's like the immediate was
    // just pushed to the stack)
    Not,
    And,
    Or,
    In,
    Equal,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Different,
    DivisibleBy,
    Even,
    First,
    Float,
    Int,
    Last,
    Length,
    Lower,
    Max,
    Min,
    Odd,
    Range,
    Result,
    Round,
    Sort,
    Upper,
    Exists,
    ExistsInObject,
    IsBoolean,
    IsNumber,
    IsInteger,
    IsFloat,
    IsObject,
    IsArray,
    IsString,
    Default,

    // include another template
    // value is the template name
    Include,

    // callback function
    // str is the function name (this means it cannot be a lookup)
    // args specify number of arguments
    // as with builtin functions, "last" argument can be immediate
    Callback,

    // unconditional jump
    // args is the index of the bytecode to jump to.
    Jump,

    // conditional jump
    // value popped off stack is checked for truthyness
    // if false, args is the index of the bytecode to jump to.
    // if true, no action is taken (falls through)
    ConditionalJump,

    // start loop
    // value popped off stack is what is iterated over
    // args is index of bytecode after end loop (jumped to if iterable is
    // empty)
    // immediate value is key name (for maps)
    // str is value name
    StartLoop,

    // end a loop
    // args is index of the first bytecode in the loop body
    EndLoop
  };

  enum Flag {
    // location of value for value-taking ops (mask)
    kFlagValueMask = 0x03,
    // pop value off stack
    kFlagValuePop = 0x00,
    // value is immediate rather than on stack
    kFlagValueImmediate = 0x01,
    // lookup immediate str (dot notation)
    kFlagValueLookupDot = 0x02,
    // lookup immediate str (json pointer notation)
    kFlagValueLookupPointer = 0x03
  };

  Op op = Op::Nop;
  uint32_t args : 30;
  uint32_t flags : 2;

  json value;
  StringRef str;

  Bytecode() : args(0), flags(0) {}
  explicit Bytecode(Op op, unsigned int args = 0)
      : op(op), args(args), flags(0) {}
  Bytecode(Op op, StringRef str, unsigned int flags)
      : op(op), args(0), flags(flags), str(str) {}
  Bytecode(Op op, json&& value, unsigned int flags)
      : op(op), args(0), flags(flags), value(std::move(value)) {}
};

class FunctionStorage {
 public:
  void AddBuiltin(StringRef name, unsigned int numArgs, Bytecode::Op op) {
    auto& data = GetOrNew(name, numArgs);
    data.op = op;
  }

  void AddCallback(StringRef name, unsigned int numArgs,
                   const CallbackFunction& function) {
    auto& data = GetOrNew(name, numArgs);
    data.function = function;
  }

  Bytecode::Op FindBuiltin(StringRef name, unsigned int numArgs) const {
    if (auto ptr = Get(name, numArgs))
      return ptr->op;
    else
      return Bytecode::Op::Nop;
  }

  CallbackFunction FindCallback(StringRef name, unsigned int numArgs) const {
    if (auto ptr = Get(name, numArgs))
      return ptr->function;
    else
      return nullptr;
  }

 private:
  struct FunctionData {
    unsigned int numArgs = 0;
    Bytecode::Op op = Bytecode::Op::Nop;  // for builtins
    CallbackFunction function;  // for callbacks
  };

  FunctionData& GetOrNew(StringRef name, unsigned int numArgs);
  const FunctionData* Get(StringRef name, unsigned int numArgs) const;

  std::map<std::string, std::vector<FunctionData>> m_map;
};

using TemplateStorage = std::map<std::string, Template>;

}  // namespace inja

#endif  // WPIUTIL_INJA_INTERNAL_H_
