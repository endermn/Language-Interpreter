#include "statements.h"


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
