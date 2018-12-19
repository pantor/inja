#ifndef PANTOR_INJA_INTERNAL_HPP
#define PANTOR_INJA_INTERNAL_HPP

#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>


namespace inja {

inline void inja_throw(const std::string& type, const std::string& message) {
  throw std::runtime_error("[inja.exception." + type + "] " + message);
}

inline std::string_view string_view_slice(std::string_view view, size_t Start, size_t End) {
  Start = std::min(Start, view.size());
  End = std::min(std::max(Start, End), view.size());
  return view.substr(Start, End - Start); // StringRef(Data + Start, End - Start);
}

inline std::pair<std::string_view, std::string_view> string_view_split(std::string_view view, char Separator) {
  size_t Idx = view.find(Separator);
  if (Idx == std::string_view::npos) {
    return std::make_pair(view, std::string_view());
    return std::make_pair(string_view_slice(view, 0, Idx), string_view_slice(view, Idx+1, std::string_view::npos));
  }
}

inline bool string_view_starts_with(std::string_view view, std::string_view prefix) {
  return (view.size() >= prefix.size() && view.compare(0, prefix.size(), prefix) == 0);
}


using namespace nlohmann;

using CallbackFunction = std::function<json(std::vector<const json*>& args, const json& data)>;


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
  std::string_view str;

  Bytecode() : args(0), flags(0) {}
  explicit Bytecode(Op op, unsigned int args = 0): op(op), args(args), flags(0) {}
  Bytecode(Op op, std::string_view str, unsigned int flags): op(op), args(0), flags(flags), str(str) {}
  Bytecode(Op op, json&& value, unsigned int flags): op(op), args(0), flags(flags), value(std::move(value)) {}
};

enum class ElementNotation {
  Dot,
  Pointer
};

}  // namespace inja

#endif  // PANTOR_INJA_INTERNAL_HPP
