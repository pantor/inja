#ifndef INCLUDE_INJA_RENDERER_HPP_
#define INCLUDE_INJA_RENDERER_HPP_

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <memory>
#include <numeric>
#include <ostream>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "config.hpp"
#include "exceptions.hpp"
#include "function_storage.hpp"
#include "node.hpp"
#include "template.hpp"
#include "throw.hpp"
#include "utils.hpp"
#include <jsoncons_ext/jsonpointer/jsonpointer.hpp>

namespace inja {

/*!
@brief Escapes HTML
*/
inline std::string htmlescape(const std::string_view& data) {
  std::string buffer;
  buffer.reserve(static_cast<size_t>(1.1 * data.size()));
  for (size_t pos = 0; pos != data.size(); ++pos) {
    switch (data[pos]) {
      case '&':  buffer.append("&amp;");       break;
      case '\"': buffer.append("&quot;");      break;
      case '\'': buffer.append("&apos;");      break;
      case '<':  buffer.append("&lt;");        break;
      case '>':  buffer.append("&gt;");        break;
      default:   buffer.append(&data[pos], 1); break;
    }
  }
  return buffer;
}

/*!
@brief concat std::string_view
*/
inline std::string operator+(const std::string_view& a, const std::string_view& b) {
  std::string buffer;
  buffer.reserve(a.size() + b.size());
  std::copy(a.begin(), a.end(), std::back_inserter(buffer));
  std::copy(b.begin(), b.end(), std::back_inserter(buffer));
  return buffer;
}

/*!
 * \brief Class for rendering a Template with data.
 */
class Renderer : public NodeVisitor {
  using Op = FunctionStorage::Operation;

  const RenderConfig config;
  const TemplateStorage& template_storage;
  const FunctionStorage& function_storage;

  const Template* current_template;
  size_t current_level {0};
  std::vector<const Template*> template_stack;
  std::vector<const BlockStatementNode*> block_statement_stack;

  const json* data_input;
  std::ostream* output_stream;

  json additional_data;
  json* current_loop_data = &additional_data["loop"];

  std::vector<std::shared_ptr<json>> data_tmp_stack;
  std::stack<const json*> data_eval_stack;
  std::stack<const DataNode*> not_found_stack;

  bool break_rendering {false};

  static bool truthy(const json* data) {
    if (json_::is_bool(*data)) {
      return json_::as<bool>(*data);
    } else if (json_::is_number(*data)) {
      return (*data != 0);
    } else if (data->is_null()) {
      return false;
    }
    return !json_::is_empty(*data);
  }

  void print_data(const std::shared_ptr<json>& value) {
    const json& val = *value;
    if (json_::is_string(val)) {
      if (config.html_autoescape) {
        *output_stream << htmlescape(json_::as<std::string_view>(val));
      } else {
        *output_stream << json_::as<std::string_view>(val);
      }
    } else if (json_::is_uint64(val)) {
      *output_stream << json_::as<uint64_t>(val);
    } else if (json_::is_int64(val)) {
      *output_stream << json_::as<int64_t>(val);
    } else if (json_::is_null(val)) {
#ifdef INJA_JSONCONS
    } else if (json_::is_empty(val)) {
#endif // def INJA_JSONCONS
    } else {
      *output_stream << json_::dump(val);
    }
  }

  const std::shared_ptr<json> eval_expression_list(const ExpressionListNode& expression_list) {
    if (!expression_list.root) {
      throw_renderer_error("empty expression", expression_list);
    }

    expression_list.root->accept(*this);

    if (data_eval_stack.empty()) {
      throw_renderer_error("empty expression", expression_list);
    } else if (data_eval_stack.size() != 1) {
      throw_renderer_error("malformed expression", expression_list);
    }

    const auto result = data_eval_stack.top();
    data_eval_stack.pop();

    if (result == nullptr) {
      if (not_found_stack.empty()) {
        throw_renderer_error("expression could not be evaluated", expression_list);
      }

      const auto node = not_found_stack.top();
      not_found_stack.pop();

      throw_renderer_error("variable '" + static_cast<std::string>(node->name) + "' not found", *node);
    }
    return std::make_shared<json>(*result);
  }

  void throw_renderer_error(const std::string& message, const AstNode& node) {
    const SourceLocation loc = get_source_location(current_template->content, node.pos);
    INJA_THROW(RenderError(message, loc));
  }

  void make_result(const json&& result) {
    auto result_ptr = std::make_shared<json>(result);
    data_tmp_stack.push_back(result_ptr);
    data_eval_stack.push(result_ptr.get());
  }

  template <size_t N, size_t N_start = 0, bool throw_not_found = true> std::array<const json*, N> get_arguments(const FunctionNode& node) {
    if (node.arguments.size() < N_start + N) {
      throw_renderer_error("function needs " + std::to_string(N_start + N) + " variables, but has only found " + std::to_string(node.arguments.size()), node);
    }

    for (size_t i = N_start; i < N_start + N; i += 1) {
      node.arguments[i]->accept(*this);
    }

    if (data_eval_stack.size() < N) {
      throw_renderer_error("function needs " + std::to_string(N) + " variables, but has only found " + std::to_string(data_eval_stack.size()), node);
    }

    std::array<const json*, N> result;
    for (size_t i = 0; i < N; i += 1) {
      result[N - i - 1] = data_eval_stack.top();
      data_eval_stack.pop();

      if (!result[N - i - 1]) {
        const auto data_node = not_found_stack.top();
        not_found_stack.pop();

        if (throw_not_found) {
          throw_renderer_error("variable '" + static_cast<std::string>(data_node->name) + "' not found", *data_node);
        }
      }
    }
    return result;
  }

  template <bool throw_not_found = true> Arguments get_argument_vector(const FunctionNode& node) {
    const size_t N = node.arguments.size();
    for (const auto& a : node.arguments) {
      a->accept(*this);
    }

    if (data_eval_stack.size() < N) {
      throw_renderer_error("function needs " + std::to_string(N) + " variables, but has only found " + std::to_string(data_eval_stack.size()), node);
    }

    Arguments result {N};
    for (size_t i = 0; i < N; i += 1) {
      result[N - i - 1] = data_eval_stack.top();
      data_eval_stack.pop();

      if (!result[N - i - 1]) {
        const auto data_node = not_found_stack.top();
        not_found_stack.pop();

        if (throw_not_found) {
          throw_renderer_error("variable '" + static_cast<std::string>(data_node->name) + "' not found", *data_node);
        }
      }
    }
    return result;
  }

  void visit(const BlockNode& node) override {
    for (const auto& n : node.nodes) {
      n->accept(*this);

      if (break_rendering) {
        break;
      }
    }
  }

  void visit(const TextNode& node) override {
    output_stream->write(current_template->content.c_str() + node.pos, node.length);
  }

  void visit(const ExpressionNode&) override {}

  void visit(const LiteralNode& node) override {
    data_eval_stack.push(&node.value);
  }

  void visit(const DataNode& node) override {
    if (json_::contains(additional_data, node.ptr)) {
      data_eval_stack.push(&json_::get(additional_data, node.ptr));
    } else if (json_::contains(*data_input, node.ptr)) {
      data_eval_stack.push(&json_::get(*data_input, node.ptr));
    } else {
      // Try to evaluate as a no-argument callback
      const auto function_data = function_storage.find_function(node.name, 0);
      if (function_data.operation == FunctionStorage::Operation::Callback) {
        Arguments empty_args {};
        const auto value = std::make_shared<json>(function_data.callback(empty_args));
        data_tmp_stack.push_back(value);
        data_eval_stack.push(value.get());
      } else {
        data_eval_stack.push(nullptr);
        not_found_stack.emplace(&node);
      }
    }
  }

  void visit(const FunctionNode& node) override {
    switch (node.operation) {
    case Op::Not: {
      const auto args = get_arguments<1>(node);
      make_result(!truthy(args[0]));
    } break;
    case Op::And: {
      make_result(truthy(get_arguments<1, 0>(node)[0]) && truthy(get_arguments<1, 1>(node)[0]));
    } break;
    case Op::Or: {
      make_result(truthy(get_arguments<1, 0>(node)[0]) || truthy(get_arguments<1, 1>(node)[0]));
    } break;
    case Op::In: {
      const auto args = get_arguments<2>(node);
      decltype(auto) range = json_::array_range(*args[1]);
      make_result(std::find(range.begin(), range.end(), *args[0]) != range.end());
    } break;
    case Op::Equal: {
      const auto args = get_arguments<2>(node);
      make_result(*args[0] == *args[1]);
    } break;
    case Op::NotEqual: {
      const auto args = get_arguments<2>(node);
      make_result(*args[0] != *args[1]);
    } break;
    case Op::Greater: {
      const auto args = get_arguments<2>(node);
      make_result(*args[0] > *args[1]);
    } break;
    case Op::GreaterEqual: {
      const auto args = get_arguments<2>(node);
      make_result(*args[0] >= *args[1]);
    } break;
    case Op::Less: {
      const auto args = get_arguments<2>(node);
      make_result(*args[0] < *args[1]);
    } break;
    case Op::LessEqual: {
      const auto args = get_arguments<2>(node);
      make_result(*args[0] <= *args[1]);
    } break;
    case Op::Add: {
      const auto args = get_arguments<2>(node);
      if (json_::is_string(*args[0]) && json_::is_string(*args[1])) {
        make_result(json_::as<std::string_view>(*args[0]) + json_::as<std::string_view>(*args[1]));
      } else if (json_::is_int64(*args[0]) && json_::is_int64(*args[1])) {
        make_result(json_::as<int64_t>(*args[0]) + json_::as<int64_t>(*args[1]));
      } else {
        make_result(json_::as<double>(*args[0]) + json_::as<double>(*args[1]));
      }
    } break;
    case Op::Subtract: {
      const auto args = get_arguments<2>(node);
      if (json_::is_int64(*args[0]) && json_::is_int64(*args[1])) {
        make_result(json_::as<int64_t>(*args[0]) - json_::as<int64_t>(*args[1]));
      } else {
        make_result(json_::as<double>(*args[0]) - json_::as<double>(*args[1]));
      }
    } break;
    case Op::Multiplication: {
      const auto args = get_arguments<2>(node);
      if (json_::is_int64(*args[0]) && json_::is_int64(*args[1])) {
        make_result(json_::as<int64_t>(*args[0]) * json_::as<int64_t>(*args[1]));
      } else {
        make_result(json_::as<double>(*args[0]) * json_::as<double>(*args[1]));
      }
    } break;
    case Op::Division: {
      const auto args = get_arguments<2>(node);
      if (json_::as<double>(*args[1]) == 0) {
        throw_renderer_error("division by zero", node);
      }
      make_result(json_::as<double>(*args[0]) / json_::as<double>(*args[1]));
    } break;
    case Op::Power: {
      const auto args = get_arguments<2>(node);
      if (json_::is_int64(*args[0]) && json_::as<int64_t>(*args[1]) >= 0) {
        const auto result = static_cast<int64_t>(std::pow(json_::as<int64_t>(*args[0]), json_::as<int64_t>(*args[1])));
        make_result(result);
      } else {
        const auto result = std::pow(json_::as<int64_t>(*args[0]), json_::as<int64_t>(*args[1]));
        make_result(result);
      }
    } break;
    case Op::Modulo: {
      const auto args = get_arguments<2>(node);
      make_result(json_::as<int64_t>(*args[0]) % json_::as<int64_t>(*args[1]));
    } break;
    case Op::AtId: {
      const auto container = get_arguments<1, 0, false>(node)[0];
      node.arguments[1]->accept(*this);
      if (not_found_stack.empty()) {
        throw_renderer_error("could not find element with given name", node);
      }
      const auto id_node = not_found_stack.top();
      not_found_stack.pop();
      data_eval_stack.pop();
      data_eval_stack.push(&container->at(id_node->name));
    } break;
    case Op::At: {
      const auto args = get_arguments<2>(node);
      if (args[0]->is_object()) {
        data_eval_stack.push(&args[0]->at(json_::as<std::string_view>(*args[1])));
      } else {
        data_eval_stack.push(&args[0]->at(json_::as<int>(*args[1])));
      }
    } break;
    case Op::Capitalize: {
      auto result = json_::as<std::string>(*get_arguments<1>(node)[0]);
      result[0] = static_cast<char>(::toupper(result[0]));
      std::transform(result.begin() + 1, result.end(), result.begin() + 1, [](char c) { return static_cast<char>(::tolower(c)); });
      make_result(std::move(result));
    } break;
    case Op::Default: {
      const auto test_arg = get_arguments<1, 0, false>(node)[0];
      data_eval_stack.push((test_arg != nullptr) ? test_arg : get_arguments<1, 1>(node)[0]);
    } break;
    case Op::DivisibleBy: {
      const auto args = get_arguments<2>(node);
      const auto divisor = json_::as<int64_t>(*args[1]);
      make_result((divisor != 0) && (json_::as<int64_t>(*args[0]) % divisor == 0));
    } break;
    case Op::Even: {
      make_result(json_::as<int64_t>(*get_arguments<1>(node)[0]) % 2 == 0);
    } break;
    case Op::Exists: {
      auto&& name = json_::as<std::string>(*get_arguments<1>(node)[0]);
      make_result(json_::contains(*data_input, DataNode::convert_dot_to_ptr(name)));
    } break;
    case Op::ExistsInObject: {
      const auto args = get_arguments<2>(node);
      auto name = json_::as<std::string_view>(*args[1]);
      make_result(json_::has(*args[0], name));
    } break;
    case Op::First: {
      const auto result = &get_arguments<1>(node)[0]->at(0);
      data_eval_stack.push(result);
    } break;
    case Op::Float: {
      make_result(std::stod(json_::as<std::string>(*get_arguments<1>(node)[0])));
    } break;
    case Op::Int: {
      make_result(std::stoi(json_::as<std::string>(*get_arguments<1>(node)[0])));
    } break;
    case Op::Last: {
      const auto& a0 = get_arguments<1>(node)[0];
      const auto result = &a0->at(a0->size() - 1);
      data_eval_stack.push(result);
    } break;
    case Op::Length: {
      const auto val = get_arguments<1>(node)[0];
      if (val->is_string()) {
        make_result(json_::as<std::string_view>(*val).size());
      } else {
        make_result(val->size());
      }
    } break;
    case Op::Lower: {
      auto result = json_::as<std::string>(*get_arguments<1>(node)[0]);
      std::transform(result.begin(), result.end(), result.begin(), [](char c) { return static_cast<char>(::tolower(c)); });
      make_result(std::move(result));
    } break;
    case Op::Max: {
      decltype(auto) args = json_::array_range(*get_arguments<1>(node)[0]);
      const auto result = std::max_element(args.begin(), args.end());
      data_eval_stack.push(&(*result));
    } break;
    case Op::Min: {
      decltype(auto) args = json_::array_range(*get_arguments<1>(node)[0]);
      const auto result = std::min_element(args.begin(), args.end());
      data_eval_stack.push(&(*result));
    } break;
    case Op::Odd: {
      make_result(json_::as<int64_t>(*get_arguments<1>(node)[0]) % 2 != 0);
    } break;
    case Op::Range: {
      std::vector<int> result(json_::as<int64_t>(*get_arguments<1>(node)[0]));
      std::iota(result.begin(), result.end(), 0);
      make_result(std::move(result));
    } break;
    case Op::Replace: {
      const auto args = get_arguments<3>(node);
      auto result = json_::as<std::string>(*args[0]);
      replace_substring(result, json_::as<std::string>(*args[1]), json_::as<std::string>(*args[2]));
      make_result(std::move(result));
    } break;
    case Op::Round: {
      const auto args = get_arguments<2>(node);
      const auto precision = json_::as<int64_t>(*args[1]);
      const double result = std::round(json_::as<double>(*args[0]) * std::pow(10.0, precision)) / std::pow(10.0, precision);
      if (precision == 0) {
        make_result(static_cast<int>(result));
      } else {
        make_result(result);
      }
    } break;
    case Op::Sort: {
      auto result_ptr = std::make_shared<json>(json_::as<std::vector<json>>(*get_arguments<1>(node)[0]));
      decltype(auto) range = json_::array_range(*result_ptr);
      std::sort(range.begin(), range.end());      
      data_tmp_stack.push_back(result_ptr);
      data_eval_stack.push(result_ptr.get());
    } break;
    case Op::Upper: {
      auto result = json_::as<std::string>(*get_arguments<1>(node)[0]);
      std::transform(result.begin(), result.end(), result.begin(), [](char c) { return static_cast<char>(::toupper(c)); });
      make_result(std::move(result));
    } break;
    case Op::IsBoolean: {
      make_result(json_::is_bool(*get_arguments<1>(node)[0]));
    } break;
    case Op::IsNumber: {
      make_result(json_::is_number(*get_arguments<1>(node)[0]));
    } break;
    case Op::IsInteger: {
      make_result(json_::is_int64(*get_arguments<1>(node)[0]));
    } break;
    case Op::IsFloat: {
      make_result(json_::is_float(*get_arguments<1>(node)[0]));
    } break;
    case Op::IsObject: {
      make_result(json_::is_object(*get_arguments<1>(node)[0]));
    } break;
    case Op::IsArray: {
      make_result(json_::is_array(*get_arguments<1>(node)[0]));
    } break;
    case Op::IsString: {
      make_result(json_::is_string(*get_arguments<1>(node)[0]));
    } break;
    case Op::Callback: {
      auto args = get_argument_vector(node);
      make_result(node.callback(args));
    } break;
    case Op::Super: {
      const auto args = get_argument_vector(node);
      const size_t old_level = current_level;
      const size_t level_diff = (args.size() == 1) ? json_::as<int64_t>(*args[0]) : 1;
      const size_t level = current_level + level_diff;

      if (block_statement_stack.empty()) {
        throw_renderer_error("super() call is not within a block", node);
      }

      if (level < 1 || level > template_stack.size() - 1) {
        throw_renderer_error("level of super() call does not match parent templates (between 1 and " + std::to_string(template_stack.size() - 1) + ")", node);
      }

      const auto current_block_statement = block_statement_stack.back();
      const Template* new_template = template_stack.at(level);
      const Template* old_template = current_template;
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
      make_result(nullptr);
    } break;
    case Op::Join: {
      const auto args = get_arguments<2>(node);
      const auto separator = json_::as<std::string>(*args[1]);
      std::ostringstream os;
      std::string sep;
      for (const auto& value : json_::array_range(*args[0])) {
        os << sep;
        if (value.is_string()) {
          os << json_::as<std::string_view>(value); // otherwise the value is surrounded with ""
        } else {
          os << json_::dump(value);
        }
        sep = separator;
      }
      make_result(os.str());
    } break;
    case Op::None:
      break;
    }
  }

  void visit(const ExpressionListNode& node) override {
    print_data(eval_expression_list(node));
  }

  void visit(const StatementNode&) override {}

  void visit(const ForStatementNode&) override {}

  void visit(const ForArrayStatementNode& node) override {
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
    for (auto& it: json_::array_range(*result)) {
      additional_data[static_cast<std::string>(node.value)] = it;
#ifdef INJA_JSONCONS
      // modifing additional_data invalidate current_loop_data
      current_loop_data = &additional_data["loop"];
#endif // def INJA_JSONCONS

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

    additional_data[static_cast<std::string>(node.value)].clear();
#ifdef INJA_JSONCONS
    current_loop_data = &additional_data["loop"];
#endif // def INJA_JSONCONS
    if (!(*current_loop_data)["parent"].empty()) {
      const auto tmp = (*current_loop_data)["parent"];
      *current_loop_data = tmp;
    } else {
      current_loop_data = &additional_data["loop"];
    }
  }

  void visit(const ForObjectStatementNode& node) override {
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
    for (const auto& it : json_::object_range(*result)) {
      additional_data[static_cast<std::string>(node.key)] = it.key();
      additional_data[static_cast<std::string>(node.value)] = it.value();
#ifdef INJA_JSONCONS
      current_loop_data = &additional_data["loop"];
#endif // def INJA_JSONCONS

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

    additional_data[static_cast<std::string>(node.key)].clear();
    additional_data[static_cast<std::string>(node.value)].clear();
#ifdef INJA_JSONCONS
    current_loop_data = &additional_data["loop"];
#endif // def INJA_JSONCONS
    if (!(*current_loop_data)["parent"].empty()) {
      *current_loop_data = std::move((*current_loop_data)["parent"]);
    } else {
      current_loop_data = &additional_data["loop"];
    }
  }

  void visit(const IfStatementNode& node) override {
    const auto result = eval_expression_list(node.condition);
    if (truthy(result.get())) {
      node.true_statement.accept(*this);
    } else if (node.has_false_statement) {
      node.false_statement.accept(*this);
    }
  }

  void visit(const IncludeStatementNode& node) override {
    auto sub_renderer = Renderer(config, template_storage, function_storage);
    const auto included_template_it = template_storage.find(node.file);
    if (included_template_it != template_storage.end()) {
      sub_renderer.render_to(*output_stream, included_template_it->second, *data_input, &additional_data);
    } else if (config.throw_at_missing_includes) {
      throw_renderer_error("include '" + node.file + "' not found", node);
    }
  }

  void visit(const ExtendsStatementNode& node) override {
    const auto included_template_it = template_storage.find(node.file);
    if (included_template_it != template_storage.end()) {
      const Template* parent_template = &included_template_it->second;
      render_to(*output_stream, *parent_template, *data_input, &additional_data);
      break_rendering = true;
    } else if (config.throw_at_missing_includes) {
      throw_renderer_error("extends '" + node.file + "' not found", node);
    }
  }

  void visit(const BlockStatementNode& node) override {
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

  void visit(const SetStatementNode& node) override {
    std::string ptr = node.key;
    replace_substring(ptr, ".", "/");
    ptr = "/" + ptr;
    json_::set_value(additional_data, ptr, *eval_expression_list(node.expression));
  }

public:
  explicit Renderer(const RenderConfig& config, const TemplateStorage& template_storage, const FunctionStorage& function_storage)
      : config(config), template_storage(template_storage), function_storage(function_storage) {}

  void render_to(std::ostream& os, const Template& tmpl, const json& data, json* loop_data = nullptr) {
    output_stream = &os;
    current_template = &tmpl;
    data_input = &data;
    if (loop_data != nullptr) {
      additional_data = *loop_data;
      current_loop_data = &additional_data["loop"];
    }

    template_stack.emplace_back(current_template);
    current_template->root.accept(*this);

    data_tmp_stack.clear();
  }
};

} // namespace inja

#endif // INCLUDE_INJA_RENDERER_HPP_
