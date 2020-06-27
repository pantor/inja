// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_RENDERER_HPP_
#define INCLUDE_INJA_RENDERER_HPP_

#include <algorithm>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "exceptions.hpp"
#include "node.hpp"
#include "template.hpp"
#include "utils.hpp"

namespace inja {

inline nonstd::string_view convert_dot_to_json_pointer(nonstd::string_view dot, std::string &out) {
  out.clear();
  do {
    nonstd::string_view part;
    std::tie(part, dot) = string_view::split(dot, '.');
    out.push_back('/');
    out.append(part.begin(), part.end());
  } while (!dot.empty());
  return nonstd::string_view(out.data(), out.size());
}

/*!
 * \brief Class for rendering a Template with data.
 */
class Renderer {
  std::vector<const json *> &get_args(const Node &bc) {
    m_tmp_args.clear();

    bool has_imm = ((bc.flags & Node::Flag::ValueMask) != Node::Flag::ValuePop);

    // get args from stack
    unsigned int pop_args = bc.args;
    if (has_imm) {
      pop_args -= 1;
    }

    for (auto i = std::prev(m_stack.end(), pop_args); i != m_stack.end(); i++) {
      m_tmp_args.push_back(&(*i));
    }

    // get immediate arg
    if (has_imm) {
      m_tmp_args.push_back(get_imm(bc));
    }

    return m_tmp_args;
  }

  void pop_args(const Node &bc) {
    unsigned int pop_args = bc.args;
    if ((bc.flags & Node::Flag::ValueMask) != Node::Flag::ValuePop) {
      pop_args -= 1;
    }
    for (unsigned int i = 0; i < pop_args; ++i) {
      m_stack.pop_back();
    }
  }

  const json *get_imm(const Node &bc) {
    std::string ptr_buffer;
    nonstd::string_view ptr;
    switch (bc.flags & Node::Flag::ValueMask) {
    case Node::Flag::ValuePop:
      return nullptr;
    case Node::Flag::ValueImmediate:
      return &bc.value;
    case Node::Flag::ValueLookupDot:
      ptr = convert_dot_to_json_pointer(bc.str, ptr_buffer);
      break;
    case Node::Flag::ValueLookupPointer:
      ptr_buffer += '/';
      ptr_buffer += bc.str;
      ptr = ptr_buffer;
      break;
    }
    
    json::json_pointer json_ptr(ptr.data());
    try {
      // first try to evaluate as a loop variable
      // Using contains() is faster than unsucessful at() and throwing an exception
      if (m_loop_data && m_loop_data->contains(json_ptr)) {
        return &m_loop_data->at(json_ptr);
      }
      return &m_data->at(json_ptr);
    } catch (std::exception &) {
      // try to evaluate as a no-argument callback
      if (auto callback = function_storage.find_callback(bc.str, 0)) {
        std::vector<const json *> arguments {};
        m_tmp_val = callback(arguments);
        return &m_tmp_val;
      }

      throw RenderError("variable '" + static_cast<std::string>(bc.str) + "' not found");
      return nullptr;
    }
  }

  bool truthy(const json &var) const {
    if (var.empty()) {
      return false;
    } else if (var.is_number()) {
      return (var != 0);
    } else if (var.is_string()) {
      return !var.empty();
    }

    try {
      return var.get<bool>();
    } catch (json::type_error &e) {
      throw JsonError(e.what());
    }
  }

  void update_loop_data() {
    LoopLevel &level = m_loop_stack.back();

    if (level.loop_type == LoopLevel::Type::Array) {
      level.data[static_cast<std::string>(level.value_name)] = level.values.at(level.index); // *level.it;
    } else {
      level.data[static_cast<std::string>(level.key_name)] = level.map_it->first;
      level.data[static_cast<std::string>(level.value_name)] = *level.map_it->second;
    }
    auto &loop_data = level.data["loop"];
    loop_data["index"] = level.index;
    loop_data["index1"] = level.index + 1;
    loop_data["is_first"] = (level.index == 0);
    loop_data["is_last"] = (level.index == level.size - 1);
  }

  struct LoopLevel {
    enum class Type { Map, Array };

    Type loop_type;
    nonstd::string_view key_name;   // variable name for keys
    nonstd::string_view value_name; // variable name for values
    json data;                      // data with loop info added

    json values; // values to iterate over

    // loop over list
    size_t index; // current list index
    size_t size;  // length of list

    // loop over map
    using KeyValue = std::pair<nonstd::string_view, json *>;
    using MapValues = std::vector<KeyValue>;
    MapValues map_values;       // values to iterate over
    MapValues::iterator map_it; // iterator over values
  };

  const TemplateStorage &template_storage;
  const FunctionStorage &function_storage;

  std::vector<json> m_stack;
  std::vector<LoopLevel> m_loop_stack;
  json *m_loop_data;

  const json *m_data;
  std::vector<const json *> m_tmp_args;
  json m_tmp_val;

public:
  Renderer(const TemplateStorage &included_templates, const FunctionStorage &callbacks)
      : template_storage(included_templates), function_storage(callbacks) {
    m_stack.reserve(16);
    m_tmp_args.reserve(4);
    m_loop_stack.reserve(16);
  }

  void render_to(std::ostream &os, const Template &tmpl, const json &data, json *loop_data = nullptr) {
    m_data = &data;
    m_loop_data = loop_data;

    for (size_t i = 0; i < tmpl.nodes.size(); ++i) {
      const auto &bc = tmpl.nodes[i];

      switch (bc.op) {
      case Node::Op::Nop: {
        break;
      }
      case Node::Op::PrintText: {
        os << bc.str;
        break;
      }
      case Node::Op::PrintValue: {
        const json &val = *get_args(bc)[0];
        if (val.is_string()) {
          os << val.get_ref<const std::string &>();
        } else {
          os << val.dump();
        }
        pop_args(bc);
        break;
      }
      case Node::Op::Push: {
        m_stack.emplace_back(*get_imm(bc));
        break;
      }
      case Node::Op::Upper: {
        auto result = get_args(bc)[0]->get<std::string>();
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        pop_args(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Node::Op::Lower: {
        auto result = get_args(bc)[0]->get<std::string>();
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        pop_args(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Node::Op::Range: {
        int number = get_args(bc)[0]->get<int>();
        std::vector<int> result(number);
        std::iota(std::begin(result), std::end(result), 0);
        pop_args(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Node::Op::Length: {
        const json &val = *get_args(bc)[0];

        size_t result;
        if (val.is_string()) {
          result = val.get_ref<const std::string &>().length();
        } else {
          result = val.size();
        }

        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Sort: {
        auto result = get_args(bc)[0]->get<std::vector<json>>();
        std::sort(result.begin(), result.end());
        pop_args(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Node::Op::At: {
        auto args = get_args(bc);
        auto result = args[0]->at(args[1]->get<int>());
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::First: {
        auto result = get_args(bc)[0]->front();
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Last: {
        auto result = get_args(bc)[0]->back();
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Round: {
        auto args = get_args(bc);
        double number = args[0]->get<double>();
        int precision = args[1]->get<int>();
        pop_args(bc);
        m_stack.emplace_back(std::round(number * std::pow(10.0, precision)) / std::pow(10.0, precision));
        break;
      }
      case Node::Op::DivisibleBy: {
        auto args = get_args(bc);
        int number = args[0]->get<int>();
        int divisor = args[1]->get<int>();
        pop_args(bc);
        m_stack.emplace_back((divisor != 0) && (number % divisor == 0));
        break;
      }
      case Node::Op::Odd: {
        int number = get_args(bc)[0]->get<int>();
        pop_args(bc);
        m_stack.emplace_back(number % 2 != 0);
        break;
      }
      case Node::Op::Even: {
        int number = get_args(bc)[0]->get<int>();
        pop_args(bc);
        m_stack.emplace_back(number % 2 == 0);
        break;
      }
      case Node::Op::Max: {
        auto args = get_args(bc);
        auto result = *std::max_element(args[0]->begin(), args[0]->end());
        pop_args(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Node::Op::Min: {
        auto args = get_args(bc);
        auto result = *std::min_element(args[0]->begin(), args[0]->end());
        pop_args(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Node::Op::Not: {
        bool result = !truthy(*get_args(bc)[0]);
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::And: {
        auto args = get_args(bc);
        bool result = truthy(*args[0]) && truthy(*args[1]);
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Or: {
        auto args = get_args(bc);
        bool result = truthy(*args[0]) || truthy(*args[1]);
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::In: {
        auto args = get_args(bc);
        bool result = std::find(args[1]->begin(), args[1]->end(), *args[0]) != args[1]->end();
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Equal: {
        auto args = get_args(bc);
        bool result = (*args[0] == *args[1]);
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Greater: {
        auto args = get_args(bc);
        bool result = (*args[0] > *args[1]);
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Less: {
        auto args = get_args(bc);
        bool result = (*args[0] < *args[1]);
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::GreaterEqual: {
        auto args = get_args(bc);
        bool result = (*args[0] >= *args[1]);
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::LessEqual: {
        auto args = get_args(bc);
        bool result = (*args[0] <= *args[1]);
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Different: {
        auto args = get_args(bc);
        bool result = (*args[0] != *args[1]);
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Float: {
        double result = std::stod(get_args(bc)[0]->get_ref<const std::string &>());
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Int: {
        int result = std::stoi(get_args(bc)[0]->get_ref<const std::string &>());
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Exists: {
        auto &&name = get_args(bc)[0]->get_ref<const std::string &>();
        bool result = (data.find(name) != data.end());
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::ExistsInObject: {
        auto args = get_args(bc);
        auto &&name = args[1]->get_ref<const std::string &>();
        bool result = (args[0]->find(name) != args[0]->end());
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::IsBoolean: {
        bool result = get_args(bc)[0]->is_boolean();
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::IsNumber: {
        bool result = get_args(bc)[0]->is_number();
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::IsInteger: {
        bool result = get_args(bc)[0]->is_number_integer();
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::IsFloat: {
        bool result = get_args(bc)[0]->is_number_float();
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::IsObject: {
        bool result = get_args(bc)[0]->is_object();
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::IsArray: {
        bool result = get_args(bc)[0]->is_array();
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::IsString: {
        bool result = get_args(bc)[0]->is_string();
        pop_args(bc);
        m_stack.emplace_back(result);
        break;
      }
      case Node::Op::Default: {
        // default needs to be a bit "magic"; we can't evaluate the first
        // argument during the push operation, so we swap the arguments during
        // the parse phase so the second argument is pushed on the stack and
        // the first argument is in the immediate
        try {
          const json *imm = get_imm(bc);
          // if no exception was raised, replace the stack value with it
          m_stack.back() = *imm;
        } catch (std::exception &) {
          // couldn't read immediate, just leave the stack as is
        }
        break;
      }
      case Node::Op::Include:
        Renderer(template_storage, function_storage)
            .render_to(os, template_storage.find(get_imm(bc)->get_ref<const std::string &>())->second, *m_data,
                       m_loop_data);
        break;
      case Node::Op::Callback: {
        auto callback = function_storage.find_callback(bc.str, bc.args);
        if (!callback) {
          throw RenderError("function '" + static_cast<std::string>(bc.str) + "' (" +
                            std::to_string(static_cast<unsigned int>(bc.args)) + ") not found");
        }
        json result = callback(get_args(bc));
        pop_args(bc);
        m_stack.emplace_back(std::move(result));
        break;
      }
      case Node::Op::Jump: {
        i = bc.args - 1; // -1 due to ++i in loop
        break;
      }
      case Node::Op::ConditionalJump: {
        if (!truthy(m_stack.back())) {
          i = bc.args - 1; // -1 due to ++i in loop
        }
        m_stack.pop_back();
        break;
      }
      case Node::Op::StartLoop: {
        // jump past loop body if empty
        if (m_stack.back().empty()) {
          m_stack.pop_back();
          i = bc.args; // ++i in loop will take it past EndLoop
          break;
        }

        m_loop_stack.emplace_back();
        LoopLevel &level = m_loop_stack.back();
        level.value_name = bc.str;
        level.values = std::move(m_stack.back());
        if (m_loop_data) {
          level.data = *m_loop_data;
        }
        level.index = 0;
        m_stack.pop_back();

        if (bc.value.is_string()) {
          // map iterator
          if (!level.values.is_object()) {
            m_loop_stack.pop_back();
            throw RenderError("for key, value requires object");
          }
          level.loop_type = LoopLevel::Type::Map;
          level.key_name = bc.value.get_ref<const std::string &>();

          // sort by key
          for (auto it = level.values.begin(), end = level.values.end(); it != end; ++it) {
            level.map_values.emplace_back(it.key(), &it.value());
          }
          auto sort_lambda = [](const LoopLevel::KeyValue &a, const LoopLevel::KeyValue &b) {
            return a.first < b.first;
          };
          std::sort(level.map_values.begin(), level.map_values.end(), sort_lambda);
          level.map_it = level.map_values.begin();
          level.size = level.map_values.size();
        } else {
          if (!level.values.is_array()) {
            m_loop_stack.pop_back();
            throw RenderError("type must be array");
          }

          // list iterator
          level.loop_type = LoopLevel::Type::Array;
          level.size = level.values.size();
        }

        // provide parent access in nested loop
        auto parent_loop_it = level.data.find("loop");
        if (parent_loop_it != level.data.end()) {
          json loop_copy = *parent_loop_it;
          (*parent_loop_it)["parent"] = std::move(loop_copy);
        }

        // set "current" loop data to this level
        m_loop_data = &level.data;
        update_loop_data();
        break;
      }
      case Node::Op::EndLoop: {
        if (m_loop_stack.empty()) {
          throw RenderError("unexpected state in renderer");
        }
        LoopLevel &level = m_loop_stack.back();

        bool done;
        level.index += 1;
        if (level.loop_type == LoopLevel::Type::Array) {
          done = (level.index == level.values.size());
        } else {
          level.map_it += 1;
          done = (level.map_it == level.map_values.end());
        }

        if (done) {
          m_loop_stack.pop_back();
          // set "current" data to outer loop data or main data as appropriate
          if (!m_loop_stack.empty()) {
            m_loop_data = &m_loop_stack.back().data;
          } else {
            m_loop_data = loop_data;
          }
          break;
        }

        update_loop_data();

        // jump back to start of loop
        i = bc.args - 1; // -1 due to ++i in loop
        break;
      }
      default: {
        throw RenderError("unknown operation in renderer: " + std::to_string(static_cast<unsigned int>(bc.op)));
      }
      }
    }
  }
};

} // namespace inja

#endif // INCLUDE_INJA_RENDERER_HPP_
