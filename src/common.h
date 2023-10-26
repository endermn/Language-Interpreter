#include <iostream>
#include <fstream>
#include <sstream>
#include <variant>
#include <string_view>
#include <cctype>
#include <optional>
#include <unordered_map>
#include <span>
#include <vector>
#include <memory>


using namespace std::literals;


std::string makeStringRed(std::string str){
	return "\033[1;31m" + str + "\033[0m\n";
}

enum class Type {
	Void,
	Bool, 
	Double,
	String,
	Array,
};

struct ArrayElement;
using Value = std::variant<std::monostate, bool, double, std::string, std::vector<ArrayElement>>;
struct ArrayElement {
	Value value;
};

struct Ctx;
static Type type_of_value(Value value) {
	return (Type)value.index();
}

struct AST {
	AST(int line) : line(line) {}
	virtual ~AST() = default;
	[[noreturn]] void error(char const* message) {
		std::cerr << line + 1 << ": " << makeStringRed(message) << '\n';
		std::exit(1);
	}
	virtual Value evaluate(Ctx&) = 0;
	int line;
};

using UPAST = std::unique_ptr<AST>;

struct ParamDeclaration {
	Type type;
	std::string_view name;
};

struct Func {
	std::span<ParamDeclaration const> params;
	Type return_type;
	std::span<UPAST const> body;
};

struct Ctx {
	std::unordered_map<std::string_view, Value> values;
	std::unordered_map<std::string_view, Func> funcs;
};

static void evalStatements(Ctx& ctx, std::span<UPAST const> statements) {
	for (auto& statement : statements) {
		if (type_of_value(statement->evaluate(ctx)) != Type::Void)
			statement->error("Statement is not void");
	}
};

void printValue(const Value& val){

	if (auto str = std::get_if<std::string>(&val)) {
		std::cout << *str;
	}
	else if(auto arr = std::get_if<std::vector<ArrayElement>>(&val)){
		std::cout << "[";
		for(int i = 0; i < arr->size();i++){
			if(i > 0)
				std::cout << ", ";
			printValue((*arr)[i].value);
		}
		std::cout << "]";

	}
	else if (auto number = std::get_if<double>(&val)) {
		std::cout << *number;
	}
	else if (auto boolean = std::get_if<bool>(&val)) {
		std::cout << (*boolean ? "true" : "false");
	}
	else {
		std::cout << "void";
	}
}
void throwError(const Value& val){

	if (auto str = std::get_if<std::string>(&val)) {
		std::cout << makeStringRed(*str) << '\n';
	}
	else if(auto arr = std::get_if<std::vector<ArrayElement>>(&val)){
		std::cout << makeStringRed("[");
		for(int i = 0; i < arr->size();i++){
			if(i > 0)
				std::cout << makeStringRed(", ");
			throwError((*arr)[i].value);
		}
		std::cout << makeStringRed("]");

	}
	else if (auto number = std::get_if<double>(&val)) {
		std::cout << makeStringRed(std::to_string(*number));
	}
	else if (auto boolean = std::get_if<bool>(&val)) {
		std::cout << makeStringRed(*boolean ? "true" : "false");
	}
	else {
		std::cout << "void";
	}
	exit(1);
	
}

enum class BinaryOperator {
	Add,
	Subtract,
	Multiply,
	Divide,
	Equal,
	Greater,
	GreaterEquals,
	Less,
	LessEquals,
	NotEquals,
	DivideRemainder,
	DivideWhole,
	Index,
	AndAnd,
	OrOr,
};

enum class ExtendedToken { RightArrow, SlashSlash, EqualsEquals, LessEquals, GreaterEquals, NotEquals, AndAnd, OrOr };

using Token = std::variant<int, ExtendedToken, double, std::string_view, std::string>;
