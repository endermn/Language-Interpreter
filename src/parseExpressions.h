#include "declarations.h"


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
	if(lx.token == Token{'['}){
		lx.next();
		std::vector<UPAST> args;
		if (lx.token != Token(']')){
		next_element:
			args.push_back(UPAST{parseExpression(lx)});
			if(lx.token == Token{','}){
				lx.next();
				goto next_element;
			}
		}
		lx.expect(']');
		return std::make_unique<ArrayExpr>(line, std::move(args));
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
        lx.expect('(');
        lx.expect(')');
		return std::make_unique<InputExpr>(line);
	}
	if (lx.token == Token{ "exit"sv }) {
		lx.next();
		lx.expect('(');
		lx.expect(')');
		return std::make_unique<exitExpr>(line);
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
	lx.error("expected an expression");
}
UPAST parseIndexExpression(Lexer& lx) {
	UPAST left = parsePrimaryExpression(lx);
	while (true) {
		if (lx.token == Token{ '.' }) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parsePrimaryExpression(lx), BinaryOperator::Index);
		}
		else {
			return left;
		}
	}
}
UPAST parsePostfixExpression(Lexer& lx) {
	UPAST pastExpr = parseIndexExpression(lx);
	
	if(lx.token == Token{'?'}) {
		int line = lx.tokenLine;
		lx.next();
		return std::make_unique<ArraySizeExpr>(line, std::move(pastExpr));
	}else{
		return pastExpr;
	}
}


UPAST parseMultiplyDivideExpression(Lexer& lx) {
	UPAST left = parsePostfixExpression(lx);
	while (true) {
		if (lx.token == Token{ '*' }) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parseIndexExpression(lx), BinaryOperator::Multiply);
		}
		else if (lx.token == Token{ '/' }) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parseIndexExpression(lx), BinaryOperator::Divide);
		}
		else if (lx.token == Token{ '%' }) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parseIndexExpression(lx), BinaryOperator::DivideRemainder);
		}
		else if (lx.token == Token{ExtendedToken::SlashSlash}) {
			int line = lx.tokenLine;
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parseIndexExpression(lx), BinaryOperator::DivideWhole);
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
UPAST parseCompareExpression(Lexer& lx) {
	UPAST left = parseAddSubtractExpression(lx);
	int line = lx.tokenLine;
	if (lx.token == Token{ExtendedToken::EqualsEquals}) {
		lx.next();
		return std::make_unique<BinaryExpr>(line, std::move(left), parseAddSubtractExpression(lx), BinaryOperator::Equal);
	}
	else if(lx.token == Token{ExtendedToken::NotEquals}) {
		lx.next();
		return std::make_unique<BinaryExpr>(line, std::move(left), parseAddSubtractExpression(lx), BinaryOperator::NotEquals);
	}
	else if(lx.token == Token{ExtendedToken::LessEquals}) {
		lx.next();
		return std::make_unique<BinaryExpr>(line, std::move(left), parseAddSubtractExpression(lx), BinaryOperator::LessEquals);
	}
	else if(lx.token == Token{ExtendedToken::GreaterEquals}) {
		lx.next();
		return std::make_unique<BinaryExpr>(line, std::move(left), parseAddSubtractExpression(lx), BinaryOperator::GreaterEquals);
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
UPAST parseExpression(Lexer& lx){
	UPAST left = parseCompareExpression(lx);
	while (true) {
		int line = lx.tokenLine;
		if (lx.token == Token{ExtendedToken::OrOr}) {
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parseCompareExpression(lx), BinaryOperator::OrOr);
		}
		else if (lx.token == Token{ExtendedToken::AndAnd}) {
			lx.next();
			left = std::make_unique<BinaryExpr>(line, std::move(left), parseCompareExpression(lx), BinaryOperator::AndAnd);
		}
		else {
			return left;
		}
	}
}

std::optional<Type> parseType(Lexer& lx) {
	if (lx.token == Token{ "void"sv }) {
		lx.next();
		return Type::Void;
	}
	if (lx.token == Token{ "array"sv }){
		lx.next();
		return Type::Array;
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

UPAST parseStatement(Lexer&);
UPAST parseIf(Lexer& lx){
	int line = lx.tokenLine;
	lx.next();
	auto con = parseExpression(lx);
	lx.expect('{');
	while (lx.token == Token{'\n'})
		lx.next();
	std::vector<UPAST> ifStatements;
	std::vector<UPAST> elseStatements;
	while (lx.token != Token{ '}' }) {
		ifStatements.emplace_back(parseStatement(lx));
	}
	lx.next();
	if (lx.token == Token{ "else"sv }) {
		lx.next();
		if (lx.token == Token{ "if"sv }) {
			elseStatements.emplace_back(parseIf(lx));
		}else{
			lx.expect('{');
			while (lx.token == Token{'\n'})
				lx.next();
			while (lx.token != Token{ '}' }) {
				elseStatements.emplace_back(parseStatement(lx));
			}
			lx.next();
		}
	}
	return std::make_unique<IfStatement>(line, std::move(con), std::move(ifStatements), std::move(elseStatements));
}
UPAST parseStatement(Lexer& lx) {
	int line = lx.tokenLine;
	if (lx.token == Token{ "if"sv }) {
		UPAST ifStatement = parseIf(lx);
		lx.expectSemi();
		return ifStatement;
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
		while (lx.token == Token{'\n'})
			lx.next();
		while (lx.token != Token('}')) {
			statements.emplace_back(parseStatement(lx));
		}
		lx.next();
		lx.expectSemi();

		return std::make_unique<FuncDeclaration>(line, funcName, std::move(params), *return_type, std::move(statements));
	}
	if (lx.token == Token{ "for"sv }) {
		lx.next();
		auto forVar = parseStatement(lx);
		auto con = parseExpression(lx);
		std::vector<UPAST> forStatements;
		lx.expect('{');
		while (lx.token == Token{'\n'})
			lx.next();
		while (lx.token != Token('}')) {
			forStatements.emplace_back(parseStatement(lx));
		}
		lx.next();
		lx.expectSemi();
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
		lx.expectSemi();
		return std::make_unique<PrintExpr>(line, std::move(expression));
	}
	if (lx.token == Token{"throw"sv}) {
		lx.next();
		lx.expect('(');
		UPAST expression = parseExpression(lx);
		lx.expect(')');
		lx.expectSemi();
		return std::make_unique<ErrorExpr>(line, std::move(expression));
	}
	if (lx.token == Token{"return"sv}) {
		UPAST expression = nullptr;
		lx.next();
		if(lx.token != Token{';'} && lx.token != Token{'\n'})
			expression = parseExpression(lx);
		lx.expectSemi();
		return std::make_unique<ReturnStatement>(line, std::move(expression));
	}
	auto type = parseType(lx);

	if (type.has_value()) {
		std::string_view name = parseName(lx);
		lx.expect('=');
		UPAST expr = parseExpression(lx);
		lx.expectSemi();
		return std::make_unique<VariableDeclaration>(line, name, type.value(), std::move(expr));
	}
	
	UPAST expr = parseExpression(lx);
	lx.expectSemi();
	return expr;
}