/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "inja/inja.h"
#include "inja/inja_internal.h"
#include "inja/inja_renderer.h"


using namespace wpi;
using namespace inja;

static bool Truthy(const json& var) {
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

StringRef inja::ConvertDotToJsonPointer(StringRef dot,
                                        SmallVectorImpl<char>& out) {
  out.clear();
  do {
    StringRef part;
    std::tie(part, dot) = dot.split('.');
    out.push_back('/');
    out.append(part.begin(), part.end());
  } while (!dot.empty());
  return StringRef(out.data(), out.size());
}

void Renderer::RenderTo(std::stringstream& os, const Template& tmpl, const json& data) {
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
        const json& val = *GetArgs(bc)[0];
        if (val.is_string())
          os << val.get_ref<const std::string&>();
        else
          os << val.dump();
          // val.dump(os);
        PopArgs(bc);
        break;
      }
      case Bytecode::Op::Push:
        m_stack.emplace_back(*GetImm(bc));
        break;
      case Bytecode::Op::Upper: {
        auto result = GetArgs(bc)[0]->get<std::string>();
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        PopArgs(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Bytecode::Op::Lower: {
        auto result = GetArgs(bc)[0]->get<std::string>();
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        PopArgs(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Bytecode::Op::Range: {
        int number = GetArgs(bc)[0]->get<int>();
        std::vector<int> result(number);
        std::iota(std::begin(result), std::end(result), 0);
        PopArgs(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Bytecode::Op::Length: {
        auto result = GetArgs(bc)[0]->size();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Sort: {
        auto result = GetArgs(bc)[0]->get<std::vector<json>>();
        std::sort(result.begin(), result.end());
        PopArgs(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Bytecode::Op::First: {
        auto result = GetArgs(bc)[0]->front();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Last: {
        auto result = GetArgs(bc)[0]->back();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Round: {
        auto args = GetArgs(bc);
        double number = args[0]->get<double>();
        int precision = args[1]->get<int>();
        PopArgs(bc);
        m_stack.emplace_back(std::round(number * std::pow(10.0, precision)) /
                             std::pow(10.0, precision));
        break;
      }
      case Bytecode::Op::DivisibleBy: {
        auto args = GetArgs(bc);
        int number = args[0]->get<int>();
        int divisor = args[1]->get<int>();
        PopArgs(bc);
        m_stack.emplace_back((divisor != 0) && (number % divisor == 0));
        break;
      }
      case Bytecode::Op::Odd: {
        int number = GetArgs(bc)[0]->get<int>();
        PopArgs(bc);
        m_stack.emplace_back(number % 2 != 0);
        break;
      }
      case Bytecode::Op::Even: {
        int number = GetArgs(bc)[0]->get<int>();
        PopArgs(bc);
        m_stack.emplace_back(number % 2 == 0);
        break;
      }
      case Bytecode::Op::Max: {
        auto args = GetArgs(bc);
        auto result = *std::max_element(args[0]->begin(), args[0]->end());
        PopArgs(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Bytecode::Op::Min: {
        auto args = GetArgs(bc);
        auto result = *std::min_element(args[0]->begin(), args[0]->end());
        PopArgs(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Bytecode::Op::Not: {
        bool result = !Truthy(*GetArgs(bc)[0]);
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::And: {
        auto args = GetArgs(bc);
        bool result = Truthy(*args[0]) && Truthy(*args[1]);
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Or: {
        auto args = GetArgs(bc);
        bool result = Truthy(*args[0]) || Truthy(*args[1]);
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::In: {
        auto args = GetArgs(bc);
        bool result = std::find(args[1]->begin(), args[1]->end(), *args[0]) !=
                      args[1]->end();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Equal: {
        auto args = GetArgs(bc);
        bool result = (*args[0] == *args[1]);
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Greater: {
        auto args = GetArgs(bc);
        bool result = (*args[0] > *args[1]);
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Less: {
        auto args = GetArgs(bc);
        bool result = (*args[0] < *args[1]);
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::GreaterEqual: {
        auto args = GetArgs(bc);
        bool result = (*args[0] >= *args[1]);
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::LessEqual: {
        auto args = GetArgs(bc);
        bool result = (*args[0] <= *args[1]);
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Different: {
        auto args = GetArgs(bc);
        bool result = (*args[0] != *args[1]);
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Float: {
        double result =
            std::stod(GetArgs(bc)[0]->get_ref<const std::string&>());
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Int: {
        int result = std::stoi(GetArgs(bc)[0]->get_ref<const std::string&>());
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Exists: {
        auto&& name = GetArgs(bc)[0]->get_ref<const std::string&>();
        bool result = (data.find(name) != data.end());
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::ExistsInObject: {
        auto args = GetArgs(bc);
        auto&& name = args[1]->get_ref<const std::string&>();
        bool result = (args[0]->find(name) != args[0]->end());
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::IsBoolean: {
        bool result = GetArgs(bc)[0]->is_boolean();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::IsNumber: {
        bool result = GetArgs(bc)[0]->is_number();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::IsInteger: {
        bool result = GetArgs(bc)[0]->is_number_integer();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::IsFloat: {
        bool result = GetArgs(bc)[0]->is_number_float();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::IsObject: {
        bool result = GetArgs(bc)[0]->is_object();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::IsArray: {
        bool result = GetArgs(bc)[0]->is_array();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::IsString: {
        bool result = GetArgs(bc)[0]->is_string();
        PopArgs(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Bytecode::Op::Default: {
        // default needs to be a bit "magic"; we can't evaluate the first
        // argument during the push operation, so we swap the arguments during
        // the parse phase so the second argument is pushed on the stack and
        // the first argument is in the immediate
        try {
          const json* imm = GetImm(bc);
          // if no exception was raised, replace the stack value with it
          m_stack.back() = *imm;
        } catch (std::exception&) {
          // couldn't read immediate, just leave the stack as is
        }
        break;
      }
      case Bytecode::Op::Include:
        Renderer(m_includedTemplates, m_callbacks).RenderTo(os, m_includedTemplates.find(GetImm(bc)->get_ref<const std::string&>())->second, data);
        break;
      case Bytecode::Op::Callback: {
        auto cb = m_callbacks.FindCallback(bc.str, bc.args);
        if (!cb)
          inja_throw("render_error", "function '" + bc.str.str() + "' (" + std::to_string(static_cast<unsigned int>(bc.args)) + ") not found");
        json result = cb(GetArgs(bc), data);
        PopArgs(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Bytecode::Op::Jump:
        i = bc.args - 1;  // -1 due to ++i in loop
        break;
      case Bytecode::Op::ConditionalJump: {
        if (!Truthy(m_stack.back()))
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
               it != end; ++it)
            level.mapValues.emplace_back(it.key(), &it.value());
          std::sort(
              level.mapValues.begin(), level.mapValues.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
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
        UpdateLoopData();
        break;
      }
      case Bytecode::Op::EndLoop: {
        if (m_loopStack.empty())
          inja_throw("render_error", "unexpected state in renderer");
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

        UpdateLoopData();

        // jump back to start of loop
        i = bc.args - 1;  // -1 due to ++i in loop
        break;
      }
      default:
        inja_throw("render_error", "unknown op in renderer: " +
                                       std::to_string(static_cast<unsigned int>(bc.op)));
    }
  }
}

void Renderer::UpdateLoopData() {
  LoopLevel& level = m_loopStack.back();
  if (level.keyName.empty()) {
    level.data[level.valueName] = *level.it;
    auto& loopData = level.data["loop"];
    loopData["index"] = level.index;
    loopData["index1"] = level.index + 1;
    loopData["is_first"] = (level.index == 0);
    loopData["is_last"] = (level.index == level.size - 1);
  } else {
    level.data[level.keyName] = level.mapIt->first;
    level.data[level.valueName] = *level.mapIt->second;
  }
}

ArrayRef<const json*> Renderer::GetArgs(const Bytecode& bc) {
  m_tmpArgs.clear();

  bool hasImm =
      ((bc.flags & Bytecode::kFlagValueMask) != Bytecode::kFlagValuePop);

  // get args from stack
  unsigned int popArgs = bc.args;
  if (hasImm) --popArgs;
  for (auto&& i : makeArrayRef(m_stack).take_back(popArgs))
    m_tmpArgs.push_back(&i);

  // get immediate arg
  if (hasImm) m_tmpArgs.push_back(GetImm(bc));

  return m_tmpArgs;
}

void Renderer::PopArgs(const Bytecode& bc) {
  unsigned int popArgs = bc.args;
  if ((bc.flags & Bytecode::kFlagValueMask) != Bytecode::kFlagValuePop)
    --popArgs;
  for (unsigned int i = 0; i < popArgs; ++i) m_stack.pop_back();
}

const json* Renderer::GetImm(const Bytecode& bc) {
  SmallString<64> ptrBuf;
  StringRef ptr;
  switch (bc.flags & Bytecode::kFlagValueMask) {
    case Bytecode::kFlagValuePop:
      return nullptr;
    case Bytecode::kFlagValueImmediate:
      return &bc.value;
    case Bytecode::kFlagValueLookupDot:
      ptr = ConvertDotToJsonPointer(bc.str, ptrBuf);
      break;
    case Bytecode::kFlagValueLookupPointer:
      ptrBuf += '/';
      ptrBuf += bc.str;
      ptr = ptrBuf;
      break;
  }
  try {
    return &m_data->at(json::json_pointer(ptr));
  } catch (std::exception&) {
    // try to evaluate as a no-argument callback
    if (auto cb = m_callbacks.FindCallback(bc.str, 0)) {
      m_tmpVal = cb(ArrayRef<const json*>{}, *m_data);
      return &m_tmpVal;
    }
    inja_throw("render_error", "variable '" + bc.str.str() + "' not found");
    return nullptr;
  }
}
