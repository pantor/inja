#ifndef PANTOR_INJA_RENDERER_HPP
#define PANTOR_INJA_RENDERER_HPP

#include <algorithm>
#include <numeric>

#include <nlohmann/json.hpp>

#include "bytecode.hpp"
#include "template.hpp"
#include "utils.hpp"


namespace inja {

inline bool truthy(const json& var) {
  if (var.empty()) {
    return false;
  } else if (var.is_number()) {
    return (var != 0);
  } else if (var.is_string()) {
    return !var.empty();
  }

  try {
    return var.get<bool>();
  } catch (json::type_error& e) {
    inja_throw("json_error", e.what());
    throw;
  }
}

inline std::string_view convert_dot_to_json_pointer(std::string_view dot, std::string& out) {
  out.clear();
  do {
    std::string_view part;
    std::tie(part, dot) = string_view::split(dot, '.');
    out.push_back('/');
    out.append(part.begin(), part.end());
  } while (!dot.empty());
  return std::string_view(out.data(), out.size());
}

class Renderer {
  std::vector<const json*>& get_args(const Bytecode& bc) {
    m_tmpArgs.clear();

    bool hasImm = ((bc.flags & Bytecode::Flag::ValueMask) != Bytecode::Flag::ValuePop);

    // get args from stack
    unsigned int popArgs = bc.args;
    if (hasImm) --popArgs;


    for (auto i = std::prev(m_stack.end(), popArgs); i != m_stack.end(); i++) {
      m_tmpArgs.push_back(&(*i));
    }

    // get immediate arg
    if (hasImm) {
      m_tmpArgs.push_back(get_imm(bc));
    }

    return m_tmpArgs;
  }

  void pop_args(const Bytecode& bc) {
    unsigned int popArgs = bc.args;
    if ((bc.flags & Bytecode::Flag::ValueMask) != Bytecode::Flag::ValuePop)
      --popArgs;
    for (unsigned int i = 0; i < popArgs; ++i) m_stack.pop_back();
  }

  const json* get_imm(const Bytecode& bc) {
    std::string ptrBuf;
    std::string_view ptr;
    switch (bc.flags & Bytecode::Flag::ValueMask) {
      case Bytecode::Flag::ValuePop:
        return nullptr;
      case Bytecode::Flag::ValueImmediate:
        return &bc.value;
      case Bytecode::Flag::ValueLookupDot:
        ptr = convert_dot_to_json_pointer(bc.str, ptrBuf);
        break;
      case Bytecode::Flag::ValueLookupPointer:
        ptrBuf += '/';
        ptrBuf += bc.str;
        ptr = ptrBuf;
        break;
    }
    try {
      return &m_data->at(json::json_pointer(ptr.data()));
    } catch (std::exception&) {
      // try to evaluate as a no-argument callback
      if (auto callback = m_callbacks.find_callback(bc.str, 0)) {
        std::vector<const json*> asdf{};
        // m_tmpVal = cb(asdf, *m_data);
        m_tmpVal = callback(asdf);
        return &m_tmpVal;
      }
      inja_throw("render_error", "variable '" + static_cast<std::string>(bc.str) + "' not found");
      return nullptr;
    }
  }

  void update_loop_data()  {
    LoopLevel& level = m_loopStack.back();
    if (level.keyName.empty()) {
      level.data[static_cast<std::string>(level.valueName)] = *level.it;
      auto& loopData = level.data["loop"];
      loopData["index"] = level.index;
      loopData["index1"] = level.index + 1;
      loopData["is_first"] = (level.index == 0);
      loopData["is_last"] = (level.index == level.size - 1);
    } else {
      level.data[static_cast<std::string>(level.keyName)] = level.mapIt->first;
      level.data[static_cast<std::string>(level.valueName)] = *level.mapIt->second;
    }
  }

  const TemplateStorage& m_included_templates;
  const FunctionStorage& m_callbacks;

  std::vector<json> m_stack;

  struct LoopLevel {
    std::string_view keyName;       // variable name for keys
    std::string_view valueName;     // variable name for values
    json data;                      // data with loop info added

    json values;                    // values to iterate over

    // loop over list
    json::iterator it;              // iterator over values
    size_t index;                   // current list index
    size_t size;                    // length of list

    // loop over map
    using KeyValue = std::pair<std::string_view, json*>;
    using MapValues = std::vector<KeyValue>;
    MapValues mapValues;            // values to iterate over
    MapValues::iterator mapIt;      // iterator over values
  };

  std::vector<LoopLevel> m_loopStack;
  const json* m_data;

  std::vector<const json*> m_tmpArgs;
  json m_tmpVal;


 public:
  Renderer(const TemplateStorage& included_templates, const FunctionStorage& callbacks): m_included_templates(included_templates), m_callbacks(callbacks) {
    m_stack.reserve(16);
    m_tmpArgs.reserve(4);
  }

  void render_to(std::stringstream& os, const Template& tmpl, const json& data) {
    m_data = &data;

    for (size_t i = 0; i < tmpl.bytecodes.size(); ++i) {
      const auto& bc = tmpl.bytecodes[i];

      switch (bc.op) {
        case Bytecode::Op::Nop:
          break;
        case Bytecode::Op::PrintText:
          os << bc.str;
          break;
        case Bytecode::Op::PrintValue: {
          const json& val = *get_args(bc)[0];
          if (val.is_string())
            os << val.get_ref<const std::string&>();
          else
            os << val.dump();
            // val.dump(os);
          pop_args(bc);
          break;
        }
        case Bytecode::Op::Push:
          m_stack.emplace_back(*get_imm(bc));
          break;
        case Bytecode::Op::Upper: {
          auto result = get_args(bc)[0]->get<std::string>();
          std::transform(result.begin(), result.end(), result.begin(), ::toupper);
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Lower: {
          auto result = get_args(bc)[0]->get<std::string>();
          std::transform(result.begin(), result.end(), result.begin(), ::tolower);
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Range: {
          int number = get_args(bc)[0]->get<int>();
          std::vector<int> result(number);
          std::iota(std::begin(result), std::end(result), 0);
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Length: {
          auto result = get_args(bc)[0]->size();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Sort: {
          auto result = get_args(bc)[0]->get<std::vector<json>>();
          std::sort(result.begin(), result.end());
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::First: {
          auto result = get_args(bc)[0]->front();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Last: {
          auto result = get_args(bc)[0]->back();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Round: {
          auto args = get_args(bc);
          double number = args[0]->get<double>();
          int precision = args[1]->get<int>();
          pop_args(bc);
          m_stack.emplace_back(std::round(number * std::pow(10.0, precision)) / std::pow(10.0, precision));
          break;
        }
        case Bytecode::Op::DivisibleBy: {
          auto args = get_args(bc);
          int number = args[0]->get<int>();
          int divisor = args[1]->get<int>();
          pop_args(bc);
          m_stack.emplace_back((divisor != 0) && (number % divisor == 0));
          break;
        }
        case Bytecode::Op::Odd: {
          int number = get_args(bc)[0]->get<int>();
          pop_args(bc);
          m_stack.emplace_back(number % 2 != 0);
          break;
        }
        case Bytecode::Op::Even: {
          int number = get_args(bc)[0]->get<int>();
          pop_args(bc);
          m_stack.emplace_back(number % 2 == 0);
          break;
        }
        case Bytecode::Op::Max: {
          auto args = get_args(bc);
          auto result = *std::max_element(args[0]->begin(), args[0]->end());
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Min: {
          auto args = get_args(bc);
          auto result = *std::min_element(args[0]->begin(), args[0]->end());
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Not: {
          bool result = !truthy(*get_args(bc)[0]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::And: {
          auto args = get_args(bc);
          bool result = truthy(*args[0]) && truthy(*args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Or: {
          auto args = get_args(bc);
          bool result = truthy(*args[0]) || truthy(*args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::In: {
          auto args = get_args(bc);
          bool result = std::find(args[1]->begin(), args[1]->end(), *args[0]) !=
                        args[1]->end();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Equal: {
          auto args = get_args(bc);
          bool result = (*args[0] == *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Greater: {
          auto args = get_args(bc);
          bool result = (*args[0] > *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Less: {
          auto args = get_args(bc);
          bool result = (*args[0] < *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::GreaterEqual: {
          auto args = get_args(bc);
          bool result = (*args[0] >= *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::LessEqual: {
          auto args = get_args(bc);
          bool result = (*args[0] <= *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Different: {
          auto args = get_args(bc);
          bool result = (*args[0] != *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Float: {
          double result =
              std::stod(get_args(bc)[0]->get_ref<const std::string&>());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Int: {
          int result = std::stoi(get_args(bc)[0]->get_ref<const std::string&>());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Exists: {
          auto&& name = get_args(bc)[0]->get_ref<const std::string&>();
          bool result = (data.find(name) != data.end());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::ExistsInObject: {
          auto args = get_args(bc);
          auto&& name = args[1]->get_ref<const std::string&>();
          bool result = (args[0]->find(name) != args[0]->end());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsBoolean: {
          bool result = get_args(bc)[0]->is_boolean();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsNumber: {
          bool result = get_args(bc)[0]->is_number();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsInteger: {
          bool result = get_args(bc)[0]->is_number_integer();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsFloat: {
          bool result = get_args(bc)[0]->is_number_float();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsObject: {
          bool result = get_args(bc)[0]->is_object();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsArray: {
          bool result = get_args(bc)[0]->is_array();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsString: {
          bool result = get_args(bc)[0]->is_string();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Default: {
          // default needs to be a bit "magic"; we can't evaluate the first
          // argument during the push operation, so we swap the arguments during
          // the parse phase so the second argument is pushed on the stack and
          // the first argument is in the immediate
          try {
            const json* imm = get_imm(bc);
            // if no exception was raised, replace the stack value with it
            m_stack.back() = *imm;
          } catch (std::exception&) {
            // couldn't read immediate, just leave the stack as is
          }
          break;
        }
        case Bytecode::Op::Include:
          Renderer(m_included_templates, m_callbacks).render_to(os, m_included_templates.find(get_imm(bc)->get_ref<const std::string&>())->second, data);
          break;
        case Bytecode::Op::Callback: {
          auto callback = m_callbacks.find_callback(bc.str, bc.args);
          if (!callback) {
            inja_throw("render_error", "function '" + static_cast<std::string>(bc.str) + "' (" + std::to_string(static_cast<unsigned int>(bc.args)) + ") not found");
          }
          json result = callback(get_args(bc));
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Jump:
          i = bc.args - 1;  // -1 due to ++i in loop
          break;
        case Bytecode::Op::ConditionalJump: {
          if (!truthy(m_stack.back()))
            i = bc.args - 1;  // -1 due to ++i in loop
          m_stack.pop_back();
          break;
        }
        case Bytecode::Op::StartLoop: {
          // jump past loop body if empty
          if (m_stack.back().empty()) {
            m_stack.pop_back();
            i = bc.args;  // ++i in loop will take it past EndLoop
            break;
          }

          m_loopStack.emplace_back();
          LoopLevel& level = m_loopStack.back();
          level.valueName = bc.str;
          level.values = std::move(m_stack.back());
          level.data = data;
          m_stack.pop_back();

          if (bc.value.is_string()) {
            // map iterator
            if (!level.values.is_object()) {
              m_loopStack.pop_back();
              inja_throw("render_error", "for key, value requires object");
            }
            level.keyName = bc.value.get_ref<const std::string&>();

            // sort by key
            for (auto it = level.values.begin(), end = level.values.end();
                 it != end; ++it) {
              level.mapValues.emplace_back(it.key(), &it.value());
            }
            std::sort(
                level.mapValues.begin(), level.mapValues.end(),
                [](const LoopLevel::KeyValue& a, const LoopLevel::KeyValue& b) { return a.first < b.first; });
            level.mapIt = level.mapValues.begin();
          } else {
            // list iterator
            level.it = level.values.begin();
            level.index = 0;
            level.size = level.values.size();
          }

          // provide parent access in nested loop
          auto parentLoopIt = level.data.find("loop");
          if (parentLoopIt != level.data.end()) {
            json loopCopy = *parentLoopIt;
            (*parentLoopIt)["parent"] = std::move(loopCopy);
          }

          // set "current" data to loop data
          m_data = &level.data;
          update_loop_data();
          break;
        }
        case Bytecode::Op::EndLoop: {
          if (m_loopStack.empty()) {
            inja_throw("render_error", "unexpected state in renderer");
          }
          LoopLevel& level = m_loopStack.back();

          bool done;
          if (level.keyName.empty()) {
            ++level.it;
            ++level.index;
            done = (level.it == level.values.end());
          } else {
            ++level.mapIt;
            done = (level.mapIt == level.mapValues.end());
          }

          if (done) {
            m_loopStack.pop_back();
            // set "current" data to outer loop data or main data as appropriate
            if (!m_loopStack.empty())
              m_data = &m_loopStack.back().data;
            else
              m_data = &data;
            break;
          }

          update_loop_data();

          // jump back to start of loop
          i = bc.args - 1;  // -1 due to ++i in loop
          break;
        }
        default: {
          inja_throw("render_error", "unknown op in renderer: " + std::to_string(static_cast<unsigned int>(bc.op)));
        }
      }
    }
  }
};

}  // namespace inja

#endif  // PANTOR_INJA_RENDERER_HPP
