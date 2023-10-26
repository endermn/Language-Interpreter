#include "parseExpressions.h"


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