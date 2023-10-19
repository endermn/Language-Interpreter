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

#include "common.h"


class Lexer {
	std::string file;
	size_t i = 0;
	int line = 0;

public:
	Token token;
	int tokenLine;

	Lexer(const char* filePath)
	{
		std::ifstream input_file(filePath);
		input_file.exceptions(std::ifstream::failbit);
		std::stringstream buffer;
		buffer << input_file.rdbuf();
		file = buffer.str();
		next();
	}

	[[noreturn]] void error(char const* message) {
		std::cerr << tokenLine + 1 << ": " << message << '\n';
		std::exit(1);
	}
	void expect(int code) {
		if (auto n = std::get_if<int>(&token)) {
			if (*n == code) {
				next();
				return;
			}
		}
		error("Unexpected token");
	}
	void next() {
		while (file[i] == ' ' || file[i] == '\n')
		{
			if (file[i] == '\n')
				line++;
			i++;
		}
		tokenLine = line;

		if (std::isalpha(file[i])) {
			size_t oldI = i;
			do {
				i++;
			} while (std::isalnum(file[i]) || file[i] == '_');
			token = std::string_view(&file[oldI], i - oldI);
		}
		else if (std::isdigit(file[i])) {
			double n = 0;
			do {
				n = n * 10 + file[i] - '0';
				i++;
			} while (std::isdigit(file[i]));
			token = n;
		}
		else
			switch (file[i]) {
			case '/':
				if(file[++i] == '/') {
					token = ExtendedToken::SlashSlash;
					i++;
				} else {
					token = '/';
				}
				break;
			case '{':
			case '}':
			case '(':
			case ')':
			case ';':
			case '+':
			case '*':
			case ',':
			case '!':
			case '%':
			case '<':
			case '>':
				token = file[i++];
				break;
			case '=':
				if (file[++i] == '=') {
					token = ExtendedToken::EqualsEquals;
					i++;
				} else {
					token = '=';
				}
				break;
			case '-':
				if (file[++i] == '>') {
					token = ExtendedToken::RightArrow;
					i++;
				} else {
					token = '-';
				}
				break;
			case '#':
				while (file[i] != '\n' && file[i] != 0) {
					i++;
				}
				next();
				break;
			case '"': {
				i++;
				size_t startI = i;
				while (file[i] != '"') {
					if (file[i] == 0) error("expecting a closing '\"'");
					if (file[i] == '\n')line++;
					i++;
				}
				token = file.substr(startI, i++ - startI);
			} break;
			case 0:
				token = 0;
				break;
			default:
				error("unexpectd character");
			}
	}
};


struct StringExpr : AST {
	std::string val;

	StringExpr(int line, std::string val) : AST(line), val(val) {}
	
	Value evaluate(Ctx&) {
		return val;
	}
};

struct VariableExpr : AST {
	std::string_view val;
	
	VariableExpr(int line, std::string_view val) : AST(line), val(val) {}
	
	Value evaluate(Ctx& ctx) {
		return ctx.values[val];
	}
};

struct NumberExpr : AST {
	double val;
	
	NumberExpr(int line, double val) : AST(line), val(val) {}
	
	Value evaluate(Ctx&) {
		return val;
	}
};

struct NotExpr : AST {
	UPAST operand;
	
	NotExpr(int line, UPAST operand) : AST(line), operand(std::move(operand)) {}
	
	Value evaluate(Ctx& ctx) {
		Value val = operand->evaluate(ctx);
		if (auto boolean = std::get_if<bool>(&val)) {
			return !*boolean;
		}
		else {
			error("TYPE IS NOT BOOLEAN");
		}
	}
};

enum class BinaryOperator {
	Add,
	Subtract,
	Multiply,
	Divide,
	Equal,
	Greater,
	Less,
	DivideRemainder,
	DivideWhole,
};

struct InputExpr : AST {
	InputExpr(int line) : AST(line) {}
	Value evaluate(Ctx&) {
		std::string val;
		std::getline(std::cin, val);
		return val;
	}
};

struct BinaryExpr : AST {
	UPAST left, right;
	BinaryOperator op;
	BinaryExpr(int line, UPAST left, UPAST right, BinaryOperator op) :
		AST(line), left(std::move(left)), right(std::move(right)), op(op) {}
	Value evaluate(Ctx& ctx) {
		Value leftVal = left->evaluate(ctx);
		Value rightVal = right->evaluate(ctx);
		if (auto leftNumber = std::get_if<double>(&leftVal)) {
			if (auto rightNumber = std::get_if<double>(&rightVal)) {
				switch (op) {
				case BinaryOperator::Add:
					return *leftNumber + *rightNumber;
				case BinaryOperator::Subtract:
					return *leftNumber - *rightNumber;
				case BinaryOperator::Multiply:
					return *leftNumber * *rightNumber;
				case BinaryOperator::Divide:
					return *leftNumber / *rightNumber;
				case BinaryOperator::Equal:
					return *leftNumber == *rightNumber;
				case BinaryOperator::Less:
					return *leftNumber < *rightNumber;
				case BinaryOperator::Greater:
					return *leftNumber > *rightNumber;
				case BinaryOperator::DivideRemainder:
					return double(int(*leftNumber) % int(*rightNumber));
				case BinaryOperator::DivideWhole:
					return double(int(*leftNumber / *rightNumber));
				}
			}
		}else if (auto leftString = std::get_if<std::string>(&leftVal)) {

			if (auto rightString = std::get_if<std::string>(&rightVal))
				switch (op) {
				case BinaryOperator::Subtract:
					return std::stod(*leftString) - std::stod(*rightString);
				case BinaryOperator::Equal:
					return *leftString == *rightString;
				case BinaryOperator::Add:
					return *leftString + *rightString;
				case BinaryOperator::Less:
					return *leftString < *rightString;
				case BinaryOperator::Greater:
					return *leftString > *rightString;
				}
			else if (auto rightNumber = std::get_if<double>(&rightVal)) {
				switch (op) {
				case BinaryOperator::Add:
					return *leftString + std::to_string(*rightNumber);
				case BinaryOperator::Multiply:
					for (int i = 0; i < *rightNumber; i++) {
						*leftString += *leftString;
					}
					return *leftString;
				}
			}
		} else if (auto leftBool = std::get_if<bool>(&leftVal)) {
			if (auto rightBool = std::get_if<bool>(&rightVal))
				if (op == BinaryOperator::Equal)
					return *leftBool == *rightBool;
		}
		error("Both values need to be numbers");
	}
};

struct ForStatement : AST {
	UPAST variable, condition;
	std::vector<UPAST> forStatements;
	
	ForStatement(int line, UPAST variable, UPAST condition, std::vector<UPAST>&& forStatements) :
		AST(line), variable(std::move(variable)), condition(std::move(condition)), forStatements(std::move(forStatements)) {}
	
	Value evaluate(Ctx& ctx) {
		Value valVar = variable->evaluate(ctx);
		while (true) {
			Value valCon = condition->evaluate(ctx);
			if (auto con = std::get_if<bool>(&valCon)) {
				if (!*con) {
					break;
				}
			}
			else {
				error("the condition must be a boolean");
			}
			for (auto &el : forStatements) {
				el->evaluate(ctx);
			}
		}
		return std::monostate();
	}
};

struct IfStatement : AST {
	UPAST condition;
	std::vector<UPAST> ifStatements, elseStatements;
	IfStatement(int line, UPAST condition, std::vector<UPAST>&& ifStatements, std::vector<UPAST>&& elseStatements) :
		AST(line), condition(std::move(condition)), ifStatements(std::move(ifStatements)),
		elseStatements(std::move(elseStatements)){}
	Value evaluate(Ctx& ctx) {
		Value val = condition->evaluate(ctx);
		if (auto boolean = std::get_if<bool>(&val)) {
			evalStatements(ctx, *boolean ? ifStatements : elseStatements);
			return std::monostate{};
		}
		else {
			error("THE GIVEN CONDITION ISN'T A BOOLEAN");
		}

	}
};
struct PrintExpr : AST {
	UPAST printee;
	
	PrintExpr(int line, UPAST printee) : AST(line), printee(std::move(printee)) {}
	
	Value evaluate(Ctx& ctx) {
		if (printee == nullptr) 
		{
			std::cout << '\n';
			return std::monostate{};
		}
		Value val = printee->evaluate(ctx);
		if (auto str = std::get_if<std::string>(&val)) {
			std::cout << *str;
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
		return std::monostate{};
	}
};

struct VariableDeclaration : AST {
	std::string_view name;
	Type type;
	UPAST expr;

	VariableDeclaration(int line, std::string_view name, Type type, UPAST expr) :
		AST(line), expr(std::move(expr)), name(name), type(type){}
	
	Value evaluate(Ctx& ctx) {
		Value val = expr->evaluate(ctx);
		if (type_of_value(val) != type)
			error("wrong type of variable initializer");
		ctx.values[name] = val;
		return std::monostate{};
	}
};

struct FuncDeclaration : AST {
	std::string_view name;
	std::vector<ParamDeclaration> params;
	Type return_type;
	std::vector<UPAST> body;
	
	FuncDeclaration(int line, std::string_view name, std::vector<ParamDeclaration>&& params, Type return_type, std::vector<UPAST>&& body) :
		AST(line), name(name), params(std::move(params)),return_type(return_type), body(std::move(body)) {}
	
	Value evaluate(Ctx& ctx) {
		ctx.funcs[name] = Func{params, return_type, body};
		return std::monostate{};
	}
};



struct FuncCallExpression : AST {
	std::string_view name;
	std::vector<UPAST> args;

	FuncCallExpression(int line, std::string_view name, std::vector<UPAST>&& args) :
		AST(line), name(name), args(std::move(args)) {}
	
	Value evaluate(Ctx& ctx) {
		auto func = ctx.funcs[name];
		
		if (func.params.size() != args.size())
			error("Invalid number of arguments ?!");
		
		for (int i = 0; i < args.size(); i++){
			auto arg_value = args[i]->evaluate(ctx);
			if (type_of_value(arg_value) != func.params[i].type)
				args[i]->error("wrong type of argument");
			ctx.values[func.params[i].name] = arg_value;
		}
		
		evalStatements(ctx, func.body);
		
		if(func.return_type == Type::Void)
			return std::monostate{};
		error("Reached end of non-void function");
	}
};

/*
file:
	{function}
function:
	type ID '(' paramlist ')' '{' statements '}'
type:
	'void' | 'bool' | 'double' | 'string'
paramlist:
	[param {',' param}]
param:
	type ID
statements:
	{statement}
statement:
	'print' '(' expression ')' ';'
	expression ';'
	type ID '=' expression ';'
expression:
	expression-mul-div { ('+' | '-') expression-mul-div }
expression-mul-div:
	expression-primary { ('*' | '/') expression-primary } 
expression-primary:
	NUMBER
*/

std::string_view parseName(Lexer& lx) {
	if (auto psv = std::get_if<std::string_view>(&lx.token)) {
		auto sv = *psv;
		lx.next();
		return sv;
	}
	else {
		lx.error("Expected a name");
	}
}
UPAST parseExpression(Lexer& lx);
UPAST parsePrimaryExpression(Lexer& lx) {
		int line = lx.tokenLine;
	if (lx.token == Token{ '!' }) {
		lx.next();
		return std::make_unique<NotExpr>(line, parsePrimaryExpression(lx));
	}
	if (auto pn = std::get_if<double>(&lx.token)) {
		auto n = *pn;
		lx.next();
		return std::make_unique<NumberExpr>(line, n);
	}
	if (auto pstr = std::get_if<std::string>(&lx.token)) {
		auto str = *pstr;
		lx.next();
		return std::make_unique<StringExpr>(line, str);
	}
	if (lx.token == Token{ "input"sv }) {
		lx.next();
		return std::make_unique<InputExpr>(line);
	}
	if (auto pstrv = std::get_if<std::string_view>(&lx.token)) {
		auto str = *pstrv;
		lx.next();
		if(lx.token == Token{'('}) {
			lx.next();
			std::vector<UPAST> args;
			if (lx.token != Token(')')){
			next_arg:
				args.push_back(UPAST{parseExpression(lx)});
				if(lx.token == Token{','}){
					lx.next();
					goto next_arg;
				}
			}
			lx.expect(')');
			return std::make_unique<FuncCallExpression>(line, str, std::move(args));
		}
		return std::make_unique<VariableExpr>(line, str);
	}
	if (lx.token == Token {'('}){
		lx.next();
		UPAST expr = parseExpression(lx);
		lx.expect(')');
		return expr;
	}
	lx.error("ciktor isn't working");
}

UPAST parseMultiplyDivideExpression(Lexer& lx) {
	UPAST left = parsePrimaryExpression(lx);
	while (true) {
		if (lx.token == Token{ '*' }) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parsePrimaryExpression(lx), BinaryOperator::Multiply);
		}
		else if (lx.token == Token{ '/' }) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parsePrimaryExpression(lx), BinaryOperator::Divide);
		}
		else if (lx.token == Token{ '%' }) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parsePrimaryExpression(lx), BinaryOperator::DivideRemainder);
		}
		else if (lx.token == Token{ExtendedToken::SlashSlash}) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parsePrimaryExpression(lx), BinaryOperator::DivideWhole);
		}
		else {
			return left;
		}
	}
}
UPAST parseAddSubtractExpression(Lexer& lx) {
	UPAST left = parseMultiplyDivideExpression(lx);
	while (true) {
		if (lx.token == Token{ '+' }) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parseMultiplyDivideExpression(lx), BinaryOperator::Add);
		}
		else if (lx.token == Token{ '-' }) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parseMultiplyDivideExpression(lx), BinaryOperator::Subtract);
		}
		else {
			return left;
		}
	}
}
UPAST parseExpression(Lexer& lx) {
	UPAST left = parseAddSubtractExpression(lx);
	int line = lx.tokenLine;
	if (lx.token == Token{ExtendedToken::EqualsEquals}) {
		lx.next();
		return std::make_unique<BinaryExpr>(line, std::move(left), parseAddSubtractExpression(lx), BinaryOperator::Equal);
	}
	else if (lx.token == Token{ '<' }) {
		lx.next();
		return std::make_unique<BinaryExpr>(line, std::move(left), parseAddSubtractExpression(lx), BinaryOperator::Less);
	}
	else if (lx.token == Token{ '>' }) {
		lx.next();
		return std::make_unique<BinaryExpr>(line, std::move(left), parseAddSubtractExpression(lx), BinaryOperator::Greater);
	}
	else {
		return left;
	}
}
std::optional<Type> parseType(Lexer& lx) {
	if (lx.token == Token{ "void"sv }) {
		lx.next();
		return Type::Void;
	}
	if (lx.token == Token{ "bool"sv }) {
		lx.next();
		return Type::Bool;
	}
	if (lx.token == Token{ "int"sv } || lx.token == Token{ "double"sv }) {
		lx.next();
		return Type::Double;
	}
	if (lx.token == Token{ "string"sv }) {
		lx.next();
		return Type::String;
	}
	return std::nullopt;
}
UPAST parseStatement(Lexer& lx) {
	int line = lx.tokenLine;
	if (lx.token == Token{ "if"sv }) {
		lx.next();
		auto con = parseExpression(lx);
		lx.expect('{');
		std::vector<UPAST> ifStatements;
		std::vector<UPAST> elseStatements;
		while (lx.token != Token{ '}' }) {
			ifStatements.emplace_back(parseStatement(lx));
		}
		lx.next();
		if (lx.token == Token{ "else"sv }) {
			lx.next();
			if (lx.token == Token{ "if"sv }) {
				elseStatements.emplace_back(parseStatement(lx));
			}else{
				lx.expect('{');
				while (lx.token != Token{ '}' }) {
					elseStatements.emplace_back(parseStatement(lx));
				}
				lx.next();
			}
		}

		return std::make_unique<IfStatement>(line, std::move(con), std::move(ifStatements), std::move(elseStatements));
	}
	if (lx.token == Token{"func"sv}) {
		lx.next();
		std::string_view funcName = parseName(lx);
		lx.expect('<');
		std::vector<ParamDeclaration> params;
		if (lx.token != Token{'>'}) {
		next_param:
			auto type = parseType(lx);
			if (!type)
				lx.error("Expected a parameter type");
			std::string_view paramName = parseName(lx);
			params.push_back(ParamDeclaration{*type, paramName});
			if (lx.token == Token{','}) {
				lx.next();
				goto next_param;
			}
		}
		lx.expect('>');
		
		auto return_type = parseType(lx);
		if (!return_type)
			lx.error("Expected a return type");
		
		std::vector<UPAST> statements;
		lx.expect('{');
		while (lx.token != Token('}')) {
			statements.emplace_back(parseStatement(lx));
		}
		lx.next();

		return std::make_unique<FuncDeclaration>(line, funcName, std::move(params), *return_type, std::move(statements));
	}
	if (lx.token == Token{ "for"sv }) {
		lx.next();
		auto forVar = parseStatement(lx);
		auto con = parseExpression(lx);
		std::vector<UPAST> forStatements;
		lx.expect('{');
		while (lx.token != Token('}')) {
			forStatements.emplace_back(parseStatement(lx));
		}
		lx.next();
		return std::make_unique<ForStatement>(line, std::move(forVar), std::move(con), std::move(forStatements));
	}
	if (lx.token == Token{"print"sv}) {
			UPAST expression = nullptr;
			lx.next();
			lx.expect('(');
			if (lx.token != Token{ ')' }) {
				expression = parseExpression(lx);
			}
			lx.expect(')');
			lx.expect(';');
			return std::make_unique<PrintExpr>(line, std::move(expression));
	}
	auto type = parseType(lx);

	if (type.has_value()) {
		std::string_view name = parseName(lx);
		lx.expect('=');
		UPAST expr = parseExpression(lx);
		lx.expect(';');
		return std::make_unique<VariableDeclaration>(line, name, type.value(), std::move(expr));
	}
	
	UPAST expr = parseExpression(lx);
	lx.expect(';');
	return expr;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cerr << "usage: ciktor file" << '\n';
		std::exit(1);
	}
	Lexer lx(argv[1]);
	
	std::vector<UPAST> statements;
	while(lx.token != Token{0})
		statements.push_back(parseStatement(lx));
	
	Ctx ctx;
	ctx.values["true"] = true;
	ctx.values["false"] = false;
	for (auto& i : statements)
		i->evaluate(ctx);
	
	return 0;
}

/*
	void main(){

	print("hello world");
}
*/

// 'void' 'main' '(' ')' '{' 'print'
