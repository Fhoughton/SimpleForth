#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <stack>
#include <cctype>
#include <algorithm>

typedef ptrdiff_t cell_t; //we use a global typedef for portability, ptrdiff_t instead of size_t as negatives wanted

struct xt {
	std::string name;
	xt *prev;
	std::vector<xt> data;
	cell_t val;
	void (*code)(void);
};

std::stack<cell_t> ds;

std::vector<xt> code;
std::string src;
xt *current_xt;
const char *src_pos; 

std::string definition_name;

xt *dictionary;
xt *macros;

bool compile_mode = false;


std::vector<std::string> str_split(std::string str) {
	std::string buf;                 // have a buffer string
	std::stringstream ss(str);       // insert the string into a stream

	std::vector<std::string> tokens; // create vector to hold our words

	while (ss >> buf) {
		tokens.push_back(buf);
	}

	return tokens;
}

xt *find(xt *dict, std::string word) {
	for(;dict;dict=dict->prev) if(dict->name == word) return dict;
	
	return nullptr;
}

std::string get_word() {
	std::string w = "";

	// if it's a string read the the whole thing
	if (*src_pos == '"' && *src_pos != '\0')
	{
		w += '"';
		src_pos++;
		while (*src_pos != '"') {
			w += *src_pos;
			src_pos++;
		}
		w += '"';
	}
	else
	{
		// otherwise just read until we find a space
		while (!(isspace(*src_pos)) && *src_pos != '\0') {
			w+=*src_pos;
			src_pos++;
		}
	}

	if (*src_pos != '\0') { // we don't want to skip the eof and run off into random memory
		if (*src_pos == '"') { src_pos++; }
		src_pos++;
	}

	return w;
}

void f_dolit() {
	// push current_xt val to stack (is the address of the string / value of word)
	ds.push(current_xt->val);
}

xt make_lit_xt(cell_t w) {
	xt new_xt;
	new_xt.val = w;
	new_xt.code = f_dolit;
	return new_xt;
}


void compile(std::string w) {
	if (w[0] == '"') {
		// we are reading a string, not a normal word
		w.erase(remove( w.begin(), w.end(), '\"' ),w.end());
		std::string *temp_string = new std::string;
		*temp_string = w;
		code.push_back(make_lit_xt((cell_t)temp_string));
	}
	else if ((current_xt=find(macros, w))) {
		current_xt->code(); // if it's immediate we run it
	}
	else if ((current_xt=find(dictionary, w))) {
		code.push_back(*current_xt); // if it's not immediate we compile it
	}
	else {
		char *end;
		int number = strtol(w.c_str(), &end, 0);
		if (*end) {
			std::cout << "word not found (compiling): '" + w + "'" << std::endl;
		}
		else {
			code.push_back(make_lit_xt(number));
		}
	}
}

void interpret_word(std::string w) {
	if (compile_mode) { return compile(w); }

	if (w[0] == '"') {
		// string
		w.erase(remove( w.begin(), w.end(), '\"' ),w.end());
		std::string *temp_string = new std::string;
		*temp_string = w;
		ds.push((cell_t)temp_string);
	}
	else if ((current_xt=find(dictionary, w))) {
		// xt in dict, run function
		current_xt->code();
	}
	else {
		char *end;
		int number = strtol(w.c_str(), &end, 0);
		if (*end) {
			std::cout << "word not found: '" + w + "'" << std::endl;
		}
		else ds.push(number);
	}

}

void interpret() {
	src_pos = src.c_str();
	std::string w;

	while (*src_pos != '\0')
	{
		w = get_word();
		interpret_word(w);
	}
}

xt *add_word(std::string name, void (*code)(void)) {
	xt *word = new xt;
	word->prev = dictionary;
	dictionary = word;
	word->name = name;
	word->code = code;
	return word;
}

xt *add_macro(std::string name, void (*code)(void)) {
	xt *word = new xt;
	word->prev = macros;
	macros = word;
	word->name = name;
	word->code = code;
	return word;
}

void f_add() {
	cell_t v1 = ds.top();
	ds.pop();
	cell_t v2 = ds.top();
	ds.pop();
	ds.push(v1 + v2);
}

void f_sub() {
	cell_t v1 = ds.top();
	ds.pop();
	cell_t v2 = ds.top();
	ds.pop();
	ds.push(v2 - v1);
}

void f_mult() {
	cell_t v1 = ds.top();
	ds.pop();
	cell_t v2 = ds.top();
	ds.pop();
	ds.push(v1 * v2);
}

void f_div() {
	cell_t v1 = ds.top();
	ds.pop();
	cell_t v2 = ds.top();
	ds.pop();
	ds.push(v2 / v1);
}

void f_dup() {
	ds.push(ds.top());
}

void f_swap() {
	cell_t top_val = ds.top();
	ds.pop();
	cell_t second_val = ds.top();
	ds.pop();

	ds.push(top_val);
	ds.push(second_val);
}

void f_dot() {
	std::cout << ds.top() << std::endl;
	ds.pop();
}

void f_print() {
	std::cout << *(std::string*)(ds.top()) << std::endl;
	ds.pop();
}

void f_colon() {
	compile_mode = true;
	definition_name = get_word();
}

void f_showstack() {
	std::stack<cell_t> s = ds;
	
	while(!s.empty())
	{
	    cell_t w = s.top();
	    std::cout << w << std::endl;
	    s.pop();
	}
}

// function assigned to colon definitions, runs the xts in the data section
void f_docolon() {
	for (xt foo: current_xt->data) {
		foo.code();
	}
}

void f_semicolon() {
	xt *new_word = add_word(definition_name, f_docolon);

	// need to set data section here
	new_word->data = code;
	code.clear();

	// exit compiling
	compile_mode = false;
}

void f_exit() {
	exit(EXIT_SUCCESS);
}

// shows the words of a function, allows easier understanding of code at runtime
void f_dis() {
	std::string func_name = *(std::string*)(ds.top());
	xt *func = find(dictionary, func_name);

	if (func) {
		if (func->code == f_docolon) {
			std::cout << ": " << func_name << " ";

			for (xt foo: func->data) {
				if (foo.code != f_dolit) {
					std::cout << foo.name << " ";
				}
				else {
					std::cout << foo.val << " ";
				}	
			}
			std::cout << ";" << std::endl;
		}
		else {
			std::cout << "<builtin>" << std::endl;
		}
	}
	else {
		std::cout << "<unknown word>" << std::endl;
	}
	ds.pop();
}

void load_primitives() {
	// Arithmetic
	add_word("+", f_add);
	add_word("-", f_sub);
	add_word("*", f_mult);
	add_word("/", f_div);

	// Stack Manipulation
	add_word("dup", f_dup);
	add_word("swap", f_swap);
	
	// IO
	add_word(".", f_dot);
	add_word("ss", f_showstack);
	add_word("print", f_print);

	// Core
	add_word(":", f_colon);
	add_macro(";", f_semicolon);
	
	// System
	add_word("dis", f_dis);
	add_word("exit", f_exit);
	add_word("quit", f_exit);
}

int main() {
	std::cout << "  _____ _____ __  __ _____  _      ______ ______ ____  _____ _______ _    _ " << std::endl;
	std::cout << " / ____|_   _|  \\/  |  __ \\| |    |  ____|  ____/ __ \\|  __ \\__   __| |  | |" << std::endl;
 	std::cout << "| (___   | | | \\  / | |__) | |    | |__  | |__ | |  | | |__) | | |  | |__| |" << std::endl;
 	std::cout << " \\___ \\  | | | |\\/| |  ___/| |    |  __| |  __|| |  | |  _  /  | |  |  __  |" << std::endl;
 	std::cout << " ____) |_| |_| |  | | |    | |____| |____| |   | |__| | | \\ \\  | |  | |  | |" << std::endl;
 	std::cout << "|_____/|_____|_|  |_|_|    |______|______|_|    \\____/|_|  \\_\\ |_|  |_|  |_|" << std::endl << std::endl;
                                                                             
	load_primitives();

	while (true) {
		src = "";
		std::cout << "> ";
		std::getline(std::cin, src);
		interpret();
	}

	return 0;
}
