#include "Lexer.h"


struct ArrayExpr : AST {
	std::vector<UPAST> elements;
	ArrayExpr(int line, std::vector<UPAST> elements) : AST(line), elements(std::move(elements)){}

	Value evaluate(Ctx& ctx) {
		std::vector<ArrayElement> arrayElems;
		for(auto& i : elements){
			arrayElems.push_back(ArrayElement{i->evaluate(ctx)});
		}
		return arrayElems;
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
		if(auto it = ctx.values.find(val); it != ctx.values.end())
			return it->second;
		error("no such variable");
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

struct InputExpr : AST {
	InputExpr(int line) : AST(line) {}
	Value evaluate(Ctx&) {
		std::string val;
		std::getline(std::cin, val);
		return val;
	}
};

struct exitExpr : AST {
	exitExpr(int line) : AST(line) {}
	Value evaluate(Ctx&) {
		exit(1);
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
		if(op == BinaryOperator::Index){
			if(auto leftArr = std::get_if<std::string>(&leftVal)) {
				if(auto rightNumber = std::get_if<double>(&rightVal)){
					if(
					*rightNumber >= 0 &&
					*rightNumber < leftArr->size() && 
					int(*rightNumber) == *rightNumber
					)
						return std::string{(*leftArr)[*rightNumber]};
				}
			}
			if(auto leftArr = std::get_if<std::vector<ArrayElement>>(&leftVal)){
				if(auto rightNumber = std::get_if<double>(&rightVal)){
					if(
					*rightNumber >= 0 &&
					*rightNumber < leftArr->size() && 
					int(*rightNumber) == *rightNumber
					)
						return (*leftArr)[*rightNumber].value;
					else
						error("index must an integer in range 0..<arraySize");
				}else{
					error("index must be a number");
				}
			}else{
				error("NOT AN ARRAY");
			}
		}
		if (auto leftNumber = std::get_if<double>(&leftVal)) {
			if (auto rightNumber = std::get_if<double>(&rightVal)) {
				switch (op) {
				case BinaryOperator::NotEquals:
					return *leftNumber != *rightNumber;
				case BinaryOperator::LessEquals:
					return *leftNumber <= *rightNumber;
				case BinaryOperator::GreaterEquals:
					return *leftNumber >= *rightNumber;
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
				default:
					error("Unknown binary operator");
				}
			}
		}else if (auto leftString = std::get_if<std::string>(&leftVal)) {
			if (auto rightString = std::get_if<std::string>(&rightVal))

				switch (op) {
				case BinaryOperator::NotEquals:
					return *leftString != *rightString;
				case BinaryOperator::Equal:
					return *leftString == *rightString;
				case BinaryOperator::Add:
					return *leftString + *rightString;
				case BinaryOperator::Subtract:
					return std::stod(*leftString) - std::stod(*rightString);
				case BinaryOperator::GreaterEquals:
					return *leftString >= *rightString;
				case BinaryOperator::LessEquals:
					return *leftString <= *rightString;
				case BinaryOperator::Less:
					return *leftString < *rightString;
				case BinaryOperator::Greater:
					return *leftString > *rightString;
				default:
					error("no such binary operator");
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
				default:
					error("no such binary operator for this kinds of values");
				}
			}

		} else if (auto leftBool = std::get_if<bool>(&leftVal)) {
			if (auto rightBool = std::get_if<bool>(&rightVal)) {
				if (op == BinaryOperator::AndAnd){
					return *leftBool && *rightBool;
				}else if(op == BinaryOperator::OrOr) {
					return *leftBool || *rightBool;
				}else if (op == BinaryOperator::Equal){
					return *leftBool == *rightBool;
				}else if(op == BinaryOperator::NotEquals){
					return *leftBool != *rightBool;
				}
			}
		} else if (auto leftArr = std::get_if<std::vector<ArrayElement>>(&leftVal)) {
			if (auto rightArr = std::get_if<std::vector<ArrayElement>>(&rightVal)){
				if (op == BinaryOperator::Add){
					leftArr->insert(leftArr->end(), rightArr->begin(), rightArr->end());
					return *leftArr;
				}
			}else if(auto rightNum = std::get_if<double>(&rightVal)){

				switch(op){
				case BinaryOperator::Multiply:{
					if(*rightNum < 0 || int(*rightNum) != *rightNum)
						error("Multiplier does not match the expectations given");
					std::vector<ArrayElement> multipliedArray;
					for(int i = 0; i < *rightNum; i++){
						multipliedArray.insert(multipliedArray.end(), leftArr->begin(), leftArr->end());
					}
					return multipliedArray;
				}
				case BinaryOperator::Subtract:{
					if(*rightNum < 0 || int(*rightNum) != *rightNum || leftArr->size() - *rightNum < 0)
						error("Subtracter does not match the expectations given");
					
					leftArr->resize(leftArr->size() - *rightNum);
					return *leftArr;
				}
				default:
					error("no such binary operator for these kinds of values");
				}
			}
		}
		
		error("Both values need to be numbers");
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
		printValue(val);
		
		return std::monostate{};
	}
};

struct ErrorExpr : AST {
	UPAST error;
	
	ErrorExpr(int line, UPAST error) : AST(line), error(std::move(error)) {}
	
	Value evaluate(Ctx& ctx) {
		Value val = error->evaluate(ctx);
		throwError(val);
		
		return std::monostate{};
	}
};
struct ArraySizeExpr : AST {
	UPAST arr;
	ArraySizeExpr(int line, UPAST arr) : AST(line), arr(std::move(arr)){}
	Value evaluate(Ctx& ctx) {
		
		auto val = arr->evaluate(ctx);
		
		if(auto vector_ptr = std::get_if<std::vector<ArrayElement>>(&val)){
			return double(vector_ptr->size());
		
		}else if(auto string_ptr = std::get_if<std::string>(&val)){
			return double(string_ptr->size());
		}
		else{
			error("operand of array size expression must be an array");
		}
	}
};


struct FuncCallExpression : AST {
	std::string_view name;
	std::vector<UPAST> args;

	FuncCallExpression(int line, std::string_view name, std::vector<UPAST>&& args) :
		AST(line), name(name), args(std::move(args)) {}
	
	Value evaluate(Ctx& ctx) {
		auto func = ctx.funcs[name];
		
		Ctx old_ctx = ctx;

		if (func.params.size() != args.size())
			error("Invalid number of arguments ?!");
		
		for (int i = 0; i < args.size(); i++){
			auto arg_value = args[i]->evaluate(ctx);
			if (type_of_value(arg_value) != func.params[i].type)
				args[i]->error("wrong type of argument");
			ctx.values[func.params[i].name] = arg_value;
		}
		try{
			evalStatements(ctx, func.body);
			ctx = old_ctx;
		}catch(Value returnValue){
			if(type_of_value(returnValue) != func.return_type)
				error("Type missmatch. Return type must match function type");
			ctx = old_ctx;
			return returnValue;
		}
		
		if(func.return_type == Type::Void)
			return std::monostate{};
		error("Reached end of non-void function");
	}
};
