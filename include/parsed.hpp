#ifndef PANTOR_INJA_PARSED_HPP
#define PANTOR_INJA_PARSED_HPP

#include <string>
#include <vector>


namespace inja {

using json = nlohmann::json;


enum class ElementNotation {
	Dot,
	Pointer
};

struct Parsed {
	enum class Type {
		Comment,
		Condition,
		ConditionBranch,
		Expression,
		Loop,
		Main,
		String
	};

	enum class Delimiter {
		Comment,
		Expression,
		LineStatement,
		Statement
	};

	enum class Statement {
		Condition,
		Include,
		Loop
	};

	enum class Function {
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
		Callback,
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
		ReadJson,
		Exists,
		ExistsInObject,
		IsBoolean,
		IsNumber,
		IsInteger,
		IsFloat,
		IsObject,
		IsArray,
		IsString,
		Default
	};

	enum class Condition {
		If,
		ElseIf,
		Else
	};

	enum class Loop {
		ForListIn,
		ForMapIn
	};

	struct Element {
		Type type;
		std::string inner;
		std::vector<std::shared_ptr<Element>> children;

		explicit Element(): Element(Type::Main, "") { }
		explicit Element(const Type type): Element(type, "") { }
		explicit Element(const Type type, const std::string& inner): type(type), inner(inner), children({}) { }
	};

	struct ElementString: public Element {
		const std::string text;

		explicit ElementString(const std::string& text): Element(Type::String), text(text) { }
	};

	struct ElementComment: public Element {
		const std::string text;

		explicit ElementComment(const std::string& text): Element(Type::Comment), text(text) { }
	};

	struct ElementExpression: public Element {
		Function function;
		std::vector<ElementExpression> args;
		std::string command;
		json result;

		explicit ElementExpression(): ElementExpression(Function::ReadJson) { }
		explicit ElementExpression(const Function function_): Element(Type::Expression), function(function_), args({}), command("") { }
	};

	struct ElementLoop: public Element {
		Loop loop;
		const std::string key;
		const std::string value;
		const ElementExpression list;

		explicit ElementLoop(const Loop loop_, const std::string& value, const ElementExpression& list, const std::string& inner): Element(Type::Loop, inner), loop(loop_), value(value), list(list) { }
		explicit ElementLoop(const Loop loop_, const std::string& key, const std::string& value, const ElementExpression& list, const std::string& inner): Element(Type::Loop, inner), loop(loop_), key(key), value(value), list(list) { }
	};

	struct ElementConditionContainer: public Element {
		explicit ElementConditionContainer(): Element(Type::Condition) { }
	};

	struct ElementConditionBranch: public Element {
		const Condition condition_type;
		const ElementExpression condition;

		explicit ElementConditionBranch(const std::string& inner, const Condition condition_type): Element(Type::ConditionBranch, inner), condition_type(condition_type) { }
		explicit ElementConditionBranch(const std::string& inner, const Condition condition_type, const ElementExpression& condition): Element(Type::ConditionBranch, inner), condition_type(condition_type), condition(condition) { }
	};

	using Arguments = std::vector<ElementExpression>;
	using CallbackSignature = std::pair<std::string, size_t>;
};

}

#endif // PANTOR_INJA_PARSED_HPP
