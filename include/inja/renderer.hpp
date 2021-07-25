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

  const RenderConfig config;
  const TemplateStorage &template_storage;
  const FunctionStorage &function_storage;

  const Template *current_template;
  size_t current_level {0};
  std::vector<const Template*> template_stack;
  std::vector<const BlockStatementNode*> block_statement_stack;

  const json *json_input;
  std::ostream *output_stream;

  json json_additional_data;
  json* current_loop_data = &json_additional_data["loop"];

  std::vector<std::shared_ptr<json>> json_tmp_stack;
  std::stack<const json*> json_eval_stack;
  std::stack<const JsonNode*> not_found_stack;

  bool break_rendering {false};

  bool truthy(const json* data) const {
    if (data->is_boolean()) {
      return data->get<bool>();
    } else if (data->is_number()) {
      return (*data != 0);
    } else if (data->is_null()) {
      return false;
    }
    return !data->empty();
  }

  void print_json(const std::shared_ptr<json> value) {
    if (value->is_string()) {
      *output_stream << value->get_ref<const json::string_t&>();
    } else if (value->is_number_integer()) {
      *output_stream << value->get<const json::number_integer_t>();
    } else if (value->is_null()) {
    } else {
      *output_stream << value->dump();
    }
  }

  const std::shared_ptr<json> eval_expression_list(const ExpressionListNode& expression_list) {
    if (!expression_list.root) {
      throw_renderer_error("empty expression", expression_list);
    }

    expression_list.root->accept(*this);

    if (json_eval_stack.empty()) {
      throw_renderer_error("empty expression", expression_list);
    } else if (json_eval_stack.size() != 1) {
      throw_renderer_error("malformed expression", expression_list);
    }

    const auto result = json_eval_stack.top();
    json_eval_stack.pop();

    if (!result) {
      if (not_found_stack.empty()) {
        throw_renderer_error("expression could not be evaluated", expression_list);
      }

      auto node = not_found_stack.top();
      not_found_stack.pop();

      throw_renderer_error("variable '" + static_cast<std::string>(node->name) + "' not found", *node);
    }
    return std::make_shared<json>(*result);
  }

  void throw_renderer_error(const std::string &message, const AstNode& node) {
    SourceLocation loc = get_source_location(current_template->content, node.pos);
    INJA_THROW(RenderError(message, loc));
  }

  template<size_t N, size_t N_start = 0, bool throw_not_found=true>
  std::array<const json*, N> get_arguments(const FunctionNode& node) {
    if (node.arguments.size() < N_start + N) {
      throw_renderer_error("function needs " + std::to_string(N_start + N) + " variables, but has only found " + std::to_string(node.arguments.size()), node);
    }

    for (size_t i = N_start; i < N_start + N; i += 1) {
      node.arguments[i]->accept(*this);
    }

    if (json_eval_stack.size() < N) {
      throw_renderer_error("function needs " + std::to_string(N) + " variables, but has only found " + std::to_string(json_eval_stack.size()), node);
    }

    std::array<const json*, N> result;
    for (size_t i = 0; i < N; i += 1) {
      result[N - i - 1] = json_eval_stack.top();
      json_eval_stack.pop();

      if (!result[N - i - 1]) {
        const auto json_node = not_found_stack.top();
        not_found_stack.pop();

        if (throw_not_found) {
          throw_renderer_error("variable '" + static_cast<std::string>(json_node->name) + "' not found", *json_node);
        }
      }
    }
    return result;
  }

  template<bool throw_not_found=true>
  Arguments get_argument_vector(const FunctionNode& node) {
    const size_t N = node.arguments.size();
    for (auto a: node.arguments) {
      a->accept(*this);
    }

    if (json_eval_stack.size() < N) {
      throw_renderer_error("function needs " + std::to_string(N) + " variables, but has only found " + std::to_string(json_eval_stack.size()), node);
    }

    Arguments result {N};
    for (size_t i = 0; i < N; i += 1) {
      result[N - i - 1] = json_eval_stack.top();
      json_eval_stack.pop();

      if (!result[N - i - 1]) {
        const auto json_node = not_found_stack.top();
        not_found_stack.pop();

        if (throw_not_found) {
          throw_renderer_error("variable '" + static_cast<std::string>(json_node->name) + "' not found", *json_node);
        }
      }
    }
    return result;
  }

  void visit(const BlockNode& node) {
    for (auto& n : node.nodes) {
      n->accept(*this);

      if (break_rendering) {
        break;
      }
    }
  }

  void visit(const TextNode& node) {
    output_stream->write(current_template->content.c_str() + node.pos, node.length);
  }

  void visit(const ExpressionNode&) { }

  void visit(const LiteralNode& node) {
    json_eval_stack.push(&node.value);
  }

  void visit(const JsonNode& node) {
    if (json_additional_data.contains(node.ptr)) {
      json_eval_stack.push(&(json_additional_data[node.ptr]));

    } else if (json_input->contains(node.ptr)) {
      json_eval_stack.push(&(*json_input)[node.ptr]);

    } else {
      // Try to evaluate as a no-argument callback
      const auto function_data = function_storage.find_function(node.name, 0);
      if (function_data.operation == FunctionStorage::Operation::Callback) {
        Arguments empty_args {};
        const auto value = std::make_shared<json>(function_data.callback(empty_args));
        json_tmp_stack.push_back(value);
        json_eval_stack.push(value.get());

      } else {
        json_eval_stack.push(nullptr);
        not_found_stack.emplace(&node);
      }
    }
  }

  void visit(const FunctionNode& node) {
    std::shared_ptr<json> result_ptr;

    switch (node.operation) {
    case Op::Not: {
      const auto args = get_arguments<1>(node);
      result_ptr = std::make_shared<json>(!truthy(args[0]));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::And: {
      result_ptr = std::make_shared<json>(truthy(get_arguments<1, 0>(node)[0]) && truthy(get_arguments<1, 1>(node)[0]));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Or: {
      result_ptr = std::make_shared<json>(truthy(get_arguments<1, 0>(node)[0]) || truthy(get_arguments<1, 1>(node)[0]));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::In: {
      const auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(std::find(args[1]->begin(), args[1]->end(), *args[0]) != args[1]->end());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Equal: {
      const auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] == *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::NotEqual: {
      const auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] != *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Greater: {
      const auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] > *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::GreaterEqual: {
      const auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] >= *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Less: {
      const auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] < *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::LessEqual: {
      const auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] <= *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Add: {
      const auto args = get_arguments<2>(node);
      if (args[0]->is_string() && args[1]->is_string()) {
        result_ptr = std::make_shared<json>(args[0]->get_ref<const std::string&>() + args[1]->get_ref<const std::string&>());
        json_tmp_stack.push_back(result_ptr);
      } else if (args[0]->is_number_integer() && args[1]->is_number_integer()) {
        result_ptr = std::make_shared<json>(args[0]->get<int>() + args[1]->get<int>());
        json_tmp_stack.push_back(result_ptr);
      } else {
        result_ptr = std::make_shared<json>(args[0]->get<double>() + args[1]->get<double>());
        json_tmp_stack.push_back(result_ptr);
      }
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Subtract: {
      const auto args = get_arguments<2>(node);
      if (args[0]->is_number_integer() && args[1]->is_number_integer()) {
        result_ptr = std::make_shared<json>(args[0]->get<int>() - args[1]->get<int>());
        json_tmp_stack.push_back(result_ptr);
      } else {
        result_ptr = std::make_shared<json>(args[0]->get<double>() - args[1]->get<double>());
        json_tmp_stack.push_back(result_ptr);
      }
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Multiplication: {
      const auto args = get_arguments<2>(node);
      if (args[0]->is_number_integer() && args[1]->is_number_integer()) {
        result_ptr = std::make_shared<json>(args[0]->get<int>() * args[1]->get<int>());
        json_tmp_stack.push_back(result_ptr);
      } else {
        result_ptr = std::make_shared<json>(args[0]->get<double>() * args[1]->get<double>());
        json_tmp_stack.push_back(result_ptr);
      }
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Division: {
      const auto args = get_arguments<2>(node);
      if (args[1]->get<double>() == 0) {
        throw_renderer_error("division by zero", node);
      }
      result_ptr = std::make_shared<json>(args[0]->get<double>() / args[1]->get<double>());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Power: {
      const auto args = get_arguments<2>(node);
      if (args[0]->is_number_integer() && args[1]->get<int>() >= 0) {
        int result = static_cast<int>(std::pow(args[0]->get<int>(), args[1]->get<int>()));
        result_ptr = std::make_shared<json>(std::move(result));
        json_tmp_stack.push_back(result_ptr);
      } else {
        double result = std::pow(args[0]->get<double>(), args[1]->get<int>());
        result_ptr = std::make_shared<json>(std::move(result));
        json_tmp_stack.push_back(result_ptr);
      }
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Modulo: {
      const auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(args[0]->get<int>() % args[1]->get<int>());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::AtId: {
      const auto container = get_arguments<1, 0, false>(node)[0];
      node.arguments[1]->accept(*this);
      if (not_found_stack.empty()) {
        throw_renderer_error("could not find element with given name", node);
      }
      const auto id_node = not_found_stack.top();
      not_found_stack.pop();
      json_eval_stack.pop();
      json_eval_stack.push(&container->at(id_node->name));
    } break;
    case Op::At: {
      const auto args = get_arguments<2>(node);
      if (args[0]->is_object()) {
        json_eval_stack.push(&args[0]->at(args[1]->get<std::string>()));
      } else {
        json_eval_stack.push(&args[0]->at(args[1]->get<int>()));
      }
    } break;
    case Op::Default: {
      const auto test_arg = get_arguments<1, 0, false>(node)[0];
      json_eval_stack.push(test_arg ? test_arg : get_arguments<1, 1>(node)[0]);
    } break;
    case Op::DivisibleBy: {
      const auto args = get_arguments<2>(node);
      const int divisor = args[1]->get<int>();
      result_ptr = std::make_shared<json>((divisor != 0) && (args[0]->get<int>() % divisor == 0));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Even: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->get<int>() % 2 == 0);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Exists: {
      auto &&name = get_arguments<1>(node)[0]->get_ref<const std::string &>();
      result_ptr = std::make_shared<json>(json_input->contains(json::json_pointer(JsonNode::convert_dot_to_json_ptr(name))));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::ExistsInObject: {
      const auto args = get_arguments<2>(node);
      auto &&name = args[1]->get_ref<const std::string &>();
      result_ptr = std::make_shared<json>(args[0]->find(name) != args[0]->end());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::First: {
      const auto result = &get_arguments<1>(node)[0]->front();
      json_eval_stack.push(result);
    } break;
    case Op::Float: {
      result_ptr = std::make_shared<json>(std::stod(get_arguments<1>(node)[0]->get_ref<const std::string &>()));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Int: {
      result_ptr = std::make_shared<json>(std::stoi(get_arguments<1>(node)[0]->get_ref<const std::string &>()));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Last: {
      const auto result = &get_arguments<1>(node)[0]->back();
      json_eval_stack.push(result);
    } break;
    case Op::Length: {
      const auto val = get_arguments<1>(node)[0];
      if (val->is_string()) {
        result_ptr = std::make_shared<json>(val->get_ref<const std::string &>().length());
      } else {
        result_ptr = std::make_shared<json>(val->size());
      }
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Lower: {
      std::string result = get_arguments<1>(node)[0]->get<std::string>();
      std::transform(result.begin(), result.end(), result.begin(), ::tolower);
      result_ptr = std::make_shared<json>(std::move(result));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Max: {
      const auto args = get_arguments<1>(node);
      const auto result = std::max_element(args[0]->begin(), args[0]->end());
      json_eval_stack.push(&(*result));
    } break;
    case Op::Min: {
      const auto args = get_arguments<1>(node);
      const auto result = std::min_element(args[0]->begin(), args[0]->end());
      json_eval_stack.push(&(*result));
    } break;
    case Op::Odd: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->get<int>() % 2 != 0);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Range: {
      std::vector<int> result(get_arguments<1>(node)[0]->get<int>());
      std::iota(result.begin(), result.end(), 0);
      result_ptr = std::make_shared<json>(std::move(result));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Round: {
      const auto args = get_arguments<2>(node);
      const int precision = args[1]->get<int>();
      const double result = std::round(args[0]->get<double>() * std::pow(10.0, precision)) / std::pow(10.0, precision);
      if(0==precision){
        result_ptr = std::make_shared<json>(int(result));
      }else{
        result_ptr = std::make_shared<json>(std::move(result));
      }
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Sort: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->get<std::vector<json>>());
      std::sort(result_ptr->begin(), result_ptr->end());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Upper: {
      std::string result = get_arguments<1>(node)[0]->get<std::string>();
      std::transform(result.begin(), result.end(), result.begin(), ::toupper);
      result_ptr = std::make_shared<json>(std::move(result));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsBoolean: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_boolean());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsNumber: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_number());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsInteger: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_number_integer());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsFloat: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_number_float());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsObject: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_object());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsArray: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_array());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsString: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_string());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Callback: {
      auto args = get_argument_vector(node);
      result_ptr = std::make_shared<json>(node.callback(args));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Super: {
      const auto args = get_argument_vector(node);
      const size_t old_level = current_level;
      const size_t level_diff = (args.size() == 1) ? args[0]->get<int>() : 1;
      const size_t level = current_level + level_diff;

      if (block_statement_stack.empty()) {
        throw_renderer_error("super() call is not within a block", node);
      }

      if (level < 1 || level > template_stack.size() - 1) {
        throw_renderer_error("level of super() call does not match parent templates (between 1 and " + std::to_string(template_stack.size() - 1) + ")", node);
      }

      const auto current_block_statement = block_statement_stack.back();
      const Template *new_template = template_stack.at(level);
      const Template *old_template = current_template;
      const auto block_it = new_template->block_storage.find(current_block_statement->name);
      if (block_it != new_template->block_storage.end()) {
        current_template = new_template;
        current_level = level;
        block_it->second->block.accept(*this);
        current_level = old_level;
        current_template = old_template;
      } else {
        throw_renderer_error("could not find block with name '" + current_block_statement->name + "'", node);
      }
      result_ptr = std::make_shared<json>(nullptr);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Join: {
      const auto args = get_arguments<2>(node);
      const auto separator = args[1]->get<std::string>();
      std::ostringstream os;
      std::string sep;
      for (const auto& value : *args[0]) {
        os << sep;
        if (value.is_string()) {
          os << value.get<std::string>(); // otherwise the value is surrounded with ""
        } else {
          os << value;
        }
        sep = separator;
      }
      result_ptr = std::make_shared<json>(os.str());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
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

  void visit(const StatementNode&) { }

  void visit(const ForStatementNode&) { }

  void visit(const ForArrayStatementNode& node) {
    const auto result = eval_expression_list(node.condition);
    if (!result->is_array()) {
      throw_renderer_error("object must be an array", node);
    }

    if (!current_loop_data->empty()) {
      auto tmp = *current_loop_data; // Because of clang-3
      (*current_loop_data)["parent"] = std::move(tmp);
    }

    size_t index = 0;
    (*current_loop_data)["is_first"] = true;
    (*current_loop_data)["is_last"] = (result->size() <= 1);
    for (auto it = result->begin(); it != result->end(); ++it) {
      json_additional_data[static_cast<std::string>(node.value)] = *it;

      (*current_loop_data)["index"] = index;
      (*current_loop_data)["index1"] = index + 1;
      if (index == 1) {
        (*current_loop_data)["is_first"] = false;
      }
      if (index == result->size() - 1) {
        (*current_loop_data)["is_last"] = true;
      }

      node.body.accept(*this);
      ++index;
    }

    json_additional_data[static_cast<std::string>(node.value)].clear();
    if (!(*current_loop_data)["parent"].empty()) {
      const auto tmp = (*current_loop_data)["parent"];
      *current_loop_data = std::move(tmp);
    } else {
      current_loop_data = &json_additional_data["loop"];
    }
  }

  void visit(const ForObjectStatementNode& node) {
    const auto result = eval_expression_list(node.condition);
    if (!result->is_object()) {
      throw_renderer_error("object must be an object", node);
    }

    if (!current_loop_data->empty()) {
      (*current_loop_data)["parent"] = std::move(*current_loop_data);
    }

    size_t index = 0;
    (*current_loop_data)["is_first"] = true;
    (*current_loop_data)["is_last"] = (result->size() <= 1);
    for (auto it = result->begin(); it != result->end(); ++it) {
      json_additional_data[static_cast<std::string>(node.key)] = it.key();
      json_additional_data[static_cast<std::string>(node.value)] = it.value();

      (*current_loop_data)["index"] = index;
      (*current_loop_data)["index1"] = index + 1;
      if (index == 1) {
        (*current_loop_data)["is_first"] = false;
      }
      if (index == result->size() - 1) {
        (*current_loop_data)["is_last"] = true;
      }

      node.body.accept(*this);
      ++index;
    }

    json_additional_data[static_cast<std::string>(node.key)].clear();
    json_additional_data[static_cast<std::string>(node.value)].clear();
    if (!(*current_loop_data)["parent"].empty()) {
      *current_loop_data = std::move((*current_loop_data)["parent"]);
    } else {
      current_loop_data = &json_additional_data["loop"];
    }
  }

  void visit(const IfStatementNode& node) {
    const auto result = eval_expression_list(node.condition);
    if (truthy(result.get())) {
      node.true_statement.accept(*this);
    } else if (node.has_false_statement) {
      node.false_statement.accept(*this);
    }
  }

  void visit(const IncludeStatementNode& node) {
    auto sub_renderer = Renderer(config, template_storage, function_storage);
    const auto included_template_it = template_storage.find(node.file);
    if (included_template_it != template_storage.end()) {
      sub_renderer.render_to(*output_stream, included_template_it->second, *json_input, &json_additional_data);
    } else if (config.throw_at_missing_includes) {
      throw_renderer_error("include '" + node.file + "' not found", node);
    }
  }

  void visit(const ExtendsStatementNode& node) {
    const auto included_template_it = template_storage.find(node.file);
    if (included_template_it != template_storage.end()) {
      const Template *parent_template = &included_template_it->second;
      render_to(*output_stream, *parent_template, *json_input, &json_additional_data);
      break_rendering = true;
    } else if (config.throw_at_missing_includes) {
      throw_renderer_error("extends '" + node.file + "' not found", node);
    }
  }

  void visit(const BlockStatementNode& node) {
    const size_t old_level = current_level;
    current_level = 0;
    current_template = template_stack.front();
    const auto block_it = current_template->block_storage.find(node.name);
    if (block_it != current_template->block_storage.end()) {
      block_statement_stack.emplace_back(&node);
      block_it->second->block.accept(*this);
      block_statement_stack.pop_back(); 
    }
    current_level = old_level;
    current_template = template_stack.back();
  }

  void visit(const SetStatementNode& node) {
    std::string ptr = node.key;
    replace_substring(ptr, ".", "/");
    ptr = "/" + ptr;
    json_additional_data[nlohmann::json::json_pointer(ptr)] = *eval_expression_list(node.expression);
  }

public:
  Renderer(const RenderConfig& config, const TemplateStorage &template_storage, const FunctionStorage &function_storage)
      : config(config), template_storage(template_storage), function_storage(function_storage) { }

  void render_to(std::ostream &os, const Template &tmpl, const json &data, json *loop_data = nullptr) {
    output_stream = &os;
    current_template = &tmpl;
    json_input = &data;
    if (loop_data) {
      json_additional_data = *loop_data;
      current_loop_data = &json_additional_data["loop"];
    }

    template_stack.emplace_back(current_template);
    current_template->root.accept(*this);

    json_tmp_stack.clear();
  }
};

} // namespace inja

#endif // INCLUDE_INJA_RENDERER_HPP_
