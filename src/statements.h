#include "expressions.h"


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



struct ReturnStatement : AST {
	UPAST returnee;
	
	ReturnStatement(int line, UPAST returnee) : AST(line), returnee(std::move(returnee)) {}
	
	Value evaluate(Ctx& ctx) {
		throw returnee == nullptr ? std::monostate{} : returnee->evaluate(ctx);
	}
};