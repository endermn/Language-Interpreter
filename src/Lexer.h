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
		std::cerr << tokenLine + 1 << ": " << makeStringRed(message) << '\n';
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
		while (file[i] == ' ' || file[i] == '\t')
			i++;
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
			case '\n':
				i++;
				line++;
				token = Token{'\n'};
				break;
			case '/':
				if(file[++i] == '/') {
					token = ExtendedToken::SlashSlash;
					i++;
				} else {
					token = '/';
				}
				break;
			case '<':
				if(file[++i] == '='){
					token = ExtendedToken::LessEquals;
					i++;
				}else{
					token = '<';
				}
				break;
			case '>':
				if(file[++i] == '='){
					token = ExtendedToken::GreaterEquals;
					i++;
				}else{
					token = '>';
				}
				break;
			
			case '!':
				if(file[++i] == '='){
					token = ExtendedToken::NotEquals;
					i++;
				}else{
					token = '!';
				}
				break;
			case '|':
				if(file[++i] == '|'){
					token = ExtendedToken::OrOr;
					i++;
				}else{
					token = '|';
				}
				break;
			case '&':
				if(file[++i] == '&'){
					token = ExtendedToken::AndAnd;
					i++;
				} else {
					token = '&';
				}
				break;
			case '.':
			case '?':
			case '{':
			case '}':
			case '(':
			case ')':
			case '[':
			case ']':
			case ';':
			case '+':
			case '*':
			case ',':
			case '%':
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

	void expectSemi() {
		if (token != Token{';'} && token != Token{'\n'})
			error("expected ';' or a nweline");
		do next();
		while (token == Token{'\n'});
	}
};