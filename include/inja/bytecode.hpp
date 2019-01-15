#ifndef PANTOR_INJA_BYTECODE_HPP
#define PANTOR_INJA_BYTECODE_HPP

#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>


namespace inja {

using namespace nlohmann;

#define SET_ARGS(x) ((x) & 0x3fffFFFF)
#define SET_FLAGS(x) ((x) & 0x3)

struct Bytecode {
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
    EndLoop,
  };

  enum Flag {
    // location of value for value-taking ops (mask)
    ValueMask = 0x03,
    // pop value off stack
    ValuePop = 0x00,
    // value is immediate rather than on stack
    ValueImmediate = 0x01,
    // lookup immediate str (dot notation)
    ValueLookupDot = 0x02,
    // lookup immediate str (json pointer notation)
    ValueLookupPointer = 0x03,
  };

  Op op {Op::Nop};
  uint32_t args: 30;
  uint32_t flags: 2;

  json value;
  std::string str;

  Bytecode(): args(0), flags(0) {}
  explicit Bytecode(Op op, unsigned int args = 0): op(op), args(SET_ARGS(args)), flags(0) {}
  explicit Bytecode(Op op, std::string_view str, unsigned int flags): op(op), args(0), flags(SET_FLAGS(flags)), str(str) {}
  explicit Bytecode(Op op, json&& value, unsigned int flags): op(op), args(0), flags(SET_FLAGS(flags)), value(std::move(value)) {}
};

}  // namespace inja

#endif  // PANTOR_INJA_BYTECODE_HPP
