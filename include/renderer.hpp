#ifndef PANTOR_INJA_RENDERER_HPP
#define PANTOR_INJA_RENDERER_HPP

#include <algorithm>
#include <string>
#include <sstream>


namespace inja {

using json = nlohmann::json;


class Renderer {
public:
	std::map<Parsed::CallbackSignature, std::function<json(const Parsed::Arguments&, const json&)>> map_callbacks;

	template<bool>
	bool eval_expression(const Parsed::ElementExpression& element, const json& data) {
		const json var = eval_function(element, data);
		if (var.empty()) { return false; }
		else if (var.is_number()) { return (var != 0); }
		else if (var.is_string()) { return not var.empty(); }
		try {
			return var.get<bool>();
		} catch (json::type_error& e) {
			inja_throw("json_error", e.what());
			throw;
		}
	}

	template<typename T = json>
  T eval_expression(const Parsed::ElementExpression& element, const json& data) {
		const json var = eval_function(element, data);
		if (var.empty()) return T();
		try {
			return var.get<T>();
		} catch (json::type_error& e) {
			inja_throw("json_error", e.what());
			throw;
		}
	}

	json eval_function(const Parsed::ElementExpression& element, const json& data) {
		switch (element.function) {
			case Parsed::Function::Upper: {
				std::string str = eval_expression<std::string>(element.args[0], data);
				std::transform(str.begin(), str.end(), str.begin(), ::toupper);
				return str;
			}
			case Parsed::Function::Lower: {
				std::string str = eval_expression<std::string>(element.args[0], data);
				std::transform(str.begin(), str.end(), str.begin(), ::tolower);
				return str;
			}
			case Parsed::Function::Range: {
				const int number = eval_expression<int>(element.args[0], data);
				std::vector<int> result(number);
				std::iota(std::begin(result), std::end(result), 0);
				return result;
			}
			case Parsed::Function::Length: {
				const std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				return list.size();
			}
			case Parsed::Function::Sort: {
				std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				std::sort(list.begin(), list.end());
				return list;
			}
			case Parsed::Function::First: {
				const std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				return list.front();
			}
			case Parsed::Function::Last: {
				const std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				return list.back();
			}
			case Parsed::Function::Round: {
				const double number = eval_expression<double>(element.args[0], data);
				const int precision = eval_expression<int>(element.args[1], data);
				return std::round(number * std::pow(10.0, precision)) / std::pow(10.0, precision);
			}
			case Parsed::Function::DivisibleBy: {
				const int number = eval_expression<int>(element.args[0], data);
				const int divisor = eval_expression<int>(element.args[1], data);
				return (divisor != 0) && (number % divisor == 0);
			}
			case Parsed::Function::Odd: {
				const int number = eval_expression<int>(element.args[0], data);
				return (number % 2 != 0);
			}
			case Parsed::Function::Even: {
				const int number = eval_expression<int>(element.args[0], data);
				return (number % 2 == 0);
			}
			case Parsed::Function::Max: {
				const std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				return *std::max_element(list.begin(), list.end());
			}
			case Parsed::Function::Min: {
				const std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				return *std::min_element(list.begin(), list.end());
			}
			case Parsed::Function::Not: {
				return not eval_expression<bool>(element.args[0], data);
			}
			case Parsed::Function::And: {
				return (eval_expression<bool>(element.args[0], data) and eval_expression<bool>(element.args[1], data));
			}
			case Parsed::Function::Or: {
				return (eval_expression<bool>(element.args[0], data) or eval_expression<bool>(element.args[1], data));
			}
			case Parsed::Function::In: {
				const json value = eval_expression(element.args[0], data);
				const json list = eval_expression(element.args[1], data);
				return (std::find(list.begin(), list.end(), value) != list.end());
			}
			case Parsed::Function::Equal: {
				return eval_expression(element.args[0], data) == eval_expression(element.args[1], data);
			}
			case Parsed::Function::Greater: {
				return eval_expression(element.args[0], data) > eval_expression(element.args[1], data);
			}
			case Parsed::Function::Less: {
				return eval_expression(element.args[0], data) < eval_expression(element.args[1], data);
			}
			case Parsed::Function::GreaterEqual: {
				return eval_expression(element.args[0], data) >= eval_expression(element.args[1], data);
			}
			case Parsed::Function::LessEqual: {
				return eval_expression(element.args[0], data) <= eval_expression(element.args[1], data);
			}
			case Parsed::Function::Different: {
				return eval_expression(element.args[0], data) != eval_expression(element.args[1], data);
			}
			case Parsed::Function::Float: {
				return std::stod(eval_expression<std::string>(element.args[0], data));
			}
			case Parsed::Function::Int: {
				return std::stoi(eval_expression<std::string>(element.args[0], data));
			}
			case Parsed::Function::ReadJson: {
				try {
					return data.at(json::json_pointer(element.command));
				} catch (std::exception&) {
					inja_throw("render_error", "variable '" + element.command + "' not found");
				}
			}
			case Parsed::Function::Result: {
				return element.result;
			}
			case Parsed::Function::Default: {
				try {
					return eval_expression(element.args[0], data);
				} catch (std::exception&) {
					return eval_expression(element.args[1], data);
				}
			}
			case Parsed::Function::Callback: {
				Parsed::CallbackSignature signature = std::make_pair(element.command, element.args.size());
				return map_callbacks.at(signature)(element.args, data);
			}
			case Parsed::Function::Exists: {
				const std::string name = eval_expression<std::string>(element.args[0], data);
				return data.find(name) != data.end();
			}
			case Parsed::Function::ExistsInObject: {
				const std::string name = eval_expression<std::string>(element.args[1], data);
				const json d = eval_expression(element.args[0], data);
				return d.find(name) != d.end();
			}
			case Parsed::Function::IsBoolean: {
				const json d = eval_expression(element.args[0], data);
				return d.is_boolean();
			}
			case Parsed::Function::IsNumber: {
				const json d = eval_expression(element.args[0], data);
				return d.is_number();
			}
			case Parsed::Function::IsInteger: {
				const json d = eval_expression(element.args[0], data);
				return d.is_number_integer();
			}
			case Parsed::Function::IsFloat: {
				const json d = eval_expression(element.args[0], data);
				return d.is_number_float();
			}
			case Parsed::Function::IsObject: {
				const json d = eval_expression(element.args[0], data);
				return d.is_object();
			}
			case Parsed::Function::IsArray: {
				const json d = eval_expression(element.args[0], data);
				return d.is_array();
			}
			case Parsed::Function::IsString: {
				const json d = eval_expression(element.args[0], data);
				return d.is_string();
			}
		}

		inja_throw("render_error", "unknown function in renderer: " + element.command);
		return json();
	}

	std::string render(Template temp, const json& data) {
		std::string result {""};
		for (const auto& element: temp.parsed_template().children) {
			switch (element->type) {
				case Parsed::Type::String: {
					auto element_string = std::static_pointer_cast<Parsed::ElementString>(element);
					result.append(element_string->text);
					break;
				}
				case Parsed::Type::Expression: {
					auto element_expression = std::static_pointer_cast<Parsed::ElementExpression>(element);
					const json variable = eval_expression(*element_expression, data);

					if (variable.is_string()) {
						result.append( variable.get<std::string>() );
					} else {
						std::stringstream ss;
						ss << variable;
						result.append( ss.str() );
					}
					break;
				}
				case Parsed::Type::Loop: {
					auto element_loop = std::static_pointer_cast<Parsed::ElementLoop>(element);
					switch (element_loop->loop) {
						case Parsed::Loop::ForListIn: {
							const std::vector<json> list = eval_expression<std::vector<json>>(element_loop->list, data);
							for (unsigned int i = 0; i < list.size(); i++) {
								json data_loop = data;
								/* For nested loops, use parent/index */
								if (data_loop.count("loop") == 1) {
									data_loop["loop"]["parent"] = data_loop["loop"];
								}
								data_loop[element_loop->value] = list[i];
								data_loop["loop"]["index"] = i;
								data_loop["loop"]["index1"] = i + 1;
								data_loop["loop"]["is_first"] = (i == 0);
								data_loop["loop"]["is_last"] = (i == list.size() - 1);
								result.append( render(Template(*element_loop), data_loop) );
							}
							break;
						}
						case Parsed::Loop::ForMapIn: {
							const std::map<std::string, json> map = eval_expression<std::map<std::string, json>>(element_loop->list, data);
							for (const auto& item: map) {
								json data_loop = data;
								data_loop[element_loop->key] = item.first;
								data_loop[element_loop->value] = item.second;
								result.append( render(Template(*element_loop), data_loop) );
							}
							break;
						}
					}

					break;
				}
				case Parsed::Type::Condition: {
					auto element_condition = std::static_pointer_cast<Parsed::ElementConditionContainer>(element);
					for (const auto& branch: element_condition->children) {
						auto element_branch = std::static_pointer_cast<Parsed::ElementConditionBranch>(branch);
						if (element_branch->condition_type == Parsed::Condition::Else || eval_expression<bool>(element_branch->condition, data)) {
							result.append( render(Template(*element_branch), data) );
							break;
						}
					}
					break;
				}
				default: {
					break;
				}
			}
		}
		return result;
	}
};

}

#endif // PANTOR_INJA_RENDERER_HPP
