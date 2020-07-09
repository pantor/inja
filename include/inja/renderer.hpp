// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_RENDERER_HPP_
#define INCLUDE_INJA_RENDERER_HPP_

#include <algorithm>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "config.hpp"
#include "exceptions.hpp"
#include "node.hpp"
#include "template.hpp"
#include "utils.hpp"

namespace inja {

/*!
 * \brief Class for rendering a Template with data.
 */
class Renderer : public NodeVisitor  {
  using Op = FunctionStorage::Operation;

  bool truthy(const json* data) const {
    if (data->empty()) {
      return false;
    } else if (data->is_number()) {
      return (*data != 0);
    } else if (data->is_string()) {
      return !data->empty();
    }

    try {
      return data->get<bool>();
    } catch (json::type_error &e) {
      throw JsonError(e.what());
    }
  }

  void print_json(const json* value) {
    if (value->is_string()) {
      *output_stream << value->get_ref<const std::string &>();
    } else {
      *output_stream << value->dump();
    }
  }

  const json* eval_expression_list(const ExpressionListNode& expression_list) {
    for (auto& expression : expression_list.rpn_output) {
      expression->accept(*this);
    }

    auto result = json_eval_stack.top();
    json_eval_stack.pop();

    if (!json_eval_stack.empty()) {
      throw_renderer_error("malformed expression", expression_list);
    } else if (!result) {
      throw_renderer_error("expression could not be evaluated", expression_list);
    }
    return result;
  }

  void throw_renderer_error(const std::string &message, const AstNode& node) {
    SourceLocation loc = get_source_location(current_template->content, node.pos);
    throw RenderError(message, loc);
  }

  template<size_t N>
  std::array<const json*, N> get_arguments(const AstNode& node) {
    if (json_eval_stack.size() < N) {
      throw_renderer_error("function needs" + std::to_string(N) + "variables, but has only found " + std::to_string(json_eval_stack.size()), node);
    }

    std::array<const json*, N> result;
    for (int i = 0; i < N; i += 1) {
      result[i] = json_eval_stack.top();
      json_eval_stack.pop();
    }
    return result;
  }

  Arguments get_argument_vector(unsigned int N, const AstNode& node) {
    Arguments result;
    for (int i = 0; i < N; i += 1) {
      result.push_back(json_eval_stack.top());
      json_eval_stack.pop();
    }
    return result;
  }

  const RenderConfig config;
  const Template *current_template;
  const TemplateStorage &template_storage;
  const FunctionStorage &function_storage;

  const json *json_input;
  json json_loop_data;
  std::stack<json> json_tmp_stack;
  std::stack<const json*> json_eval_stack;

  std::ostream *output_stream;

public:
  Renderer(const RenderConfig& config, const TemplateStorage &template_storage, const FunctionStorage &function_storage)
      : config(config), template_storage(template_storage), function_storage(function_storage) { }

  void visit(const BlockNode& node) {
    for (auto& n : node.nodes) {
      n->accept(*this);
    }
  }

  void visit(const TextNode& node) {
    *output_stream << node.content;
  }

  void visit(const LiteralNode& node) {
    json_eval_stack.push(&node.value);
  }

  void visit(const JsonNode& node) {
    auto ptr = json::json_pointer(node.ptr);

    try {
      // First try to evaluate as a loop variable
      if (json_loop_data.contains(ptr)) {
        json_eval_stack.push(&json_loop_data.at(ptr));
      } else {
        json_eval_stack.push(&json_input->at(ptr));
      }

    } catch (std::exception &) {
      // Try to evaluate as a no-argument callback
      auto function_data = function_storage.find_function(node.ptr, 0);
      if (function_data.operation == FunctionStorage::Operation::Callback) {
        std::vector<const json *> empty_args {};
        auto value = function_data.callback(empty_args);
        json_tmp_stack.push(value);
        json_eval_stack.push(&json_tmp_stack.top());
      }
      // else {
      //   json_eval_stack.push(nullptr);
      // }

      throw_renderer_error("variable '" + static_cast<std::string>(node.name) + "' not found", node);
    }
  }

  void visit(const FunctionNode& node) {
    switch (node.operation) {
    case Op::Not: {
      auto args = get_arguments<1>(node);
      bool result = !truthy(args[0]);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::And: {
      auto args = get_arguments<2>(node);
      bool result = truthy(args[1]) && truthy(args[0]);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Or: {
      auto args = get_arguments<2>(node);
      bool result = truthy(args[1]) || truthy(args[0]);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::In: {
      auto args = get_arguments<2>(node);
      bool result = std::find(args[0]->begin(), args[0]->end(), *args[1]) != args[0]->end();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Equal: {
      auto args = get_arguments<2>(node);
      bool result = (*args[1] == *args[0]);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::NotEqual: {
      auto args = get_arguments<2>(node);
      bool result = (*args[1] != *args[0]);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Greater: {
      auto args = get_arguments<2>(node);
      bool result = (*args[1] > *args[0]);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::GreaterEqual: {
      auto args = get_arguments<2>(node);
      bool result = (*args[1] >= *args[0]);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Less: {
      auto args = get_arguments<2>(node);
      bool result = (*args[1] < *args[0]);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::LessEqual: {
      auto args = get_arguments<2>(node);
      bool result = (*args[1] <= *args[0]);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Add: {
      auto args = get_arguments<2>(node);
      if (args[1]->is_number_integer() && args[0]->is_number_integer()) {
        int result = args[1]->get<int>() + args[0]->get<int>();
        json_tmp_stack.push(result);
      } else {
        double result = args[1]->get<double>() + args[0]->get<double>();
        json_tmp_stack.push(result);
      }
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Subtract: {
      auto args = get_arguments<2>(node);
      if (args[1]->is_number_integer() && args[0]->is_number_integer()) {
        int result = args[1]->get<int>() - args[0]->get<int>();
        json_tmp_stack.push(result);
      } else {
        double result = args[1]->get<double>() - args[0]->get<double>();
        json_tmp_stack.push(result);
      }
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Multiplication: {
      auto args = get_arguments<2>(node);
      if (args[1]->is_number_integer() && args[0]->is_number_integer()) {
        int result = args[1]->get<int>() * args[0]->get<int>();
        json_tmp_stack.push(result);
      } else {
        double result = args[1]->get<double>() * args[0]->get<double>();
        json_tmp_stack.push(result);
      }
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Division: {
      auto args = get_arguments<2>(node);
      if (args[0]->get<double>() == 0) {
        throw_renderer_error("division by zero", node);
      }
      double result = args[1]->get<double>() / args[0]->get<double>();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Power: {
      auto args = get_arguments<2>(node);
      double result = std::pow(args[1]->get<double>(), args[0]->get<int>());
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Modulo: {
      auto args = get_arguments<2>(node);
      double result = args[1]->get<int>() % args[0]->get<int>();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::At: {
      auto args = get_arguments<2>(node);
      auto result = args[1]->at(args[0]->get<int>());
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Default: {
      auto normal_arg = get_arguments<1>(node)[0];
      if (normal_arg) {
        json_eval_stack.push(normal_arg);
        get_arguments<1>(node)[0];

      } else {
        auto default_arg = get_arguments<1>(node)[0];
        json_eval_stack.push(default_arg);
      }
    } break;
    case Op::DivisibleBy: {
      auto args = get_arguments<2>(node);
      int divisor = args[0]->get<int>();
      bool result = (divisor != 0) && (args[1]->get<int>() % divisor == 0);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Even: {
      bool result = (get_arguments<1>(node)[0]->get<int>() % 2 == 0);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Exists: {
      auto &&name = get_arguments<1>(node)[0]->get_ref<const std::string &>();
      bool result = (json_input->find(name) != json_input->end());
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::ExistsInObject: {
      auto args = get_arguments<2>(node);
      auto &&name = args[0]->get_ref<const std::string &>();
      bool result = (args[1]->find(name) != args[1]->end());
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::First: {
      auto result = get_arguments<1>(node)[0]->front();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Float: {
      double result = std::stod(get_arguments<1>(node)[0]->get_ref<const std::string &>());
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Int: {
      int result = std::stoi(get_arguments<1>(node)[0]->get_ref<const std::string &>());
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Last: {
      auto result = get_arguments<1>(node)[0]->back();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Length: {
      auto val = get_arguments<1>(node)[0];
      size_t result;
      if (val->is_string()) {
        result = val->get_ref<const std::string &>().length();
      } else {
        result = val->size();
      }
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Lower: {
      auto result = get_arguments<1>(node)[0]->get<std::string>();
      std::transform(result.begin(), result.end(), result.begin(), ::tolower);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Max: {
      auto args = get_arguments<1>(node);
      auto result = *std::max_element(args[0]->begin(), args[0]->end());
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Min: {
      auto args = get_arguments<1>(node);
      auto result = *std::min_element(args[0]->begin(), args[0]->end());
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Odd: {
      bool result = (get_arguments<1>(node)[0]->get<int>() % 2 != 0);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Range: {
      std::vector<int> result(get_arguments<1>(node)[0]->get<int>());
      std::iota(std::begin(result), std::end(result), 0);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Round: {
      auto args = get_arguments<2>(node);
      int precision = args[0]->get<int>();
      auto result = std::round(args[1]->get<double>() * std::pow(10.0, precision)) / std::pow(10.0, precision);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Sort: {
      auto result = get_arguments<1>(node)[0]->get<std::vector<json>>();
      std::sort(result.begin(), result.end());
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Upper: {
      auto result = get_arguments<1>(node)[0]->get<std::string>();
      std::transform(result.begin(), result.end(), result.begin(), ::toupper);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::IsBoolean: {
      bool result = get_arguments<1>(node)[0]->is_boolean();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::IsNumber: {
      bool result = get_arguments<1>(node)[0]->is_number();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::IsInteger: {
      bool result = get_arguments<1>(node)[0]->is_number_integer();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::IsFloat: {
      bool result = get_arguments<1>(node)[0]->is_number_float();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::IsObject: {
      bool result = get_arguments<1>(node)[0]->is_object();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::IsArray: {
      bool result = get_arguments<1>(node)[0]->is_array();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::IsString: {
      bool result = get_arguments<1>(node)[0]->is_string();
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::Callback: {
      auto args = get_argument_vector(node.number_args, node);
      json result = node.callback(args);
      json_tmp_stack.push(result);
      json_eval_stack.push(&json_tmp_stack.top());
    } break;
    case Op::ParenLeft:
    case Op::ParenRight:
    case Op::None:
      break;
    }
  }

  void visit(const ExpressionListNode& node) {
    print_json(eval_expression_list(node));
  }

  void visit(const ForArrayStatementNode& node) {
    auto result = eval_expression_list(node.condition);
    if (!result->is_array()) {
      throw_renderer_error("object must be an array", node);
    }

    json* current_loop_data = &json_loop_data["loop"];
    if (!json_loop_data.empty()) {
      (*current_loop_data)["parent"] = std::move(*current_loop_data);
    }

    for (auto it = result->begin(); it != result->end(); ++it) {
      json_loop_data[static_cast<std::string>(node.value)] = *it;

      int index = std::distance(result->begin(), it);
      (*current_loop_data)["index"] = index;
      (*current_loop_data)["index1"] = index + 1;
      (*current_loop_data)["is_first"] = (index == 0);
      (*current_loop_data)["is_last"] = (index == result->size() - 1);

      node.body.accept(*this);
    }

    json_loop_data[static_cast<std::string>(node.value)].clear();
    if (!json_loop_data["parent"].empty()) {
      *current_loop_data = std::move((*current_loop_data)["parent"]);
    } else {
      current_loop_data->clear();
    }
  }

  void visit(const ForObjectStatementNode& node) {
    auto result = eval_expression_list(node.condition);
    if (!result->is_object()) {
      throw_renderer_error("object must be an object", node);
    }

    json* current_loop_data = &json_loop_data["loop"];
    if (!json_loop_data.empty()) {
      (*current_loop_data)["parent"] = std::move(*current_loop_data);
    }

    for (auto it = result->begin(); it != result->end(); ++it) {
      json_loop_data[static_cast<std::string>(node.key)] = it.key();
      json_loop_data[static_cast<std::string>(node.value)] = it.value();

      int index = std::distance(result->begin(), it);
      (*current_loop_data)["index"] = index;
      (*current_loop_data)["index1"] = index + 1;
      (*current_loop_data)["is_first"] = (index == 0);
      (*current_loop_data)["is_last"] = (index == result->size() - 1);

      node.body.accept(*this);
    }

    json_loop_data[static_cast<std::string>(node.key)].clear();
    json_loop_data[static_cast<std::string>(node.value)].clear();
    if (!json_loop_data["parent"].empty()) {
      *current_loop_data = std::move((*current_loop_data)["parent"]);
    } else {
      current_loop_data->clear();
    }
  }

  void visit(const IfStatementNode& node) {
    auto result = eval_expression_list(node.condition);
    if (truthy(result)) {
      node.true_statement.accept(*this);
    } else if (node.has_false_statement) {
      node.false_statement.accept(*this);
    }
  }

  void visit(const IncludeStatementNode& node) {
    auto sub_renderer = Renderer(config, template_storage, function_storage);
    auto included_template_it = template_storage.find(node.file);

    if (included_template_it != template_storage.end()) {
      sub_renderer.render_to(*output_stream, included_template_it->second, *json_input, &json_loop_data);
    } else if (config.throw_at_missing_includes) {
      throw_renderer_error("include '" + node.file + "' not found", node);
    }
  }

  void render_to(std::ostream &os, const Template &tmpl, const json &data, json *loop_data = nullptr) {
    output_stream = &os;
    current_template = &tmpl;
    json_input = &data;
    if (loop_data) {
      json_loop_data = *loop_data;
    }

    current_template->root.accept(*this);
  }
};

} // namespace inja

#endif // INCLUDE_INJA_RENDERER_HPP_
