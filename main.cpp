#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <stack>
#include <cctype>
#include <algorithm>

struct xt {
	std::string name;
	xt *prev;
	std::vector<xt> data;
	size_t val;
	void (*code)(void);
};

std::stack<size_t> ds;

std::vector<xt> code;
std::string src;
xt *current_xt;
const char *src_pos; 

std::string definition_name;

xt *dictionary;
xt *macros;

bool compile_mode = false;

//MAYBE HAVE CODE STREAM AND WHEN AT END READ USER INPUT AND
//APPEND NEWLY COMPILED CODE TO STREAM THEN EXECUTE

std::vector<std::string> str_split(std::string str) {
	std::string buf;                 // Have a buffer string
	std::stringstream ss(str);       // Insert the string into a stream

	std::vector<std::string> tokens; // Create vector to hold our words

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

	//if it's a string read the the whole thing
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
		//otherwise just read until we find a space
		while (!(isspace(*src_pos)) && *src_pos != '\0') {
			w+=*src_pos;
			src_pos++;
		}
	}

	if (*src_pos != '\0') { //we don't want to skip the eof and run off into random memory
		src_pos++;
	}

	return w;
}

void f_dolit() {
	//push current_xt val to stack (is the address of the string / value of word)
	ds.push(current_xt->val);
}

xt make_lit_xt(size_t w) {
	xt new_xt;
	new_xt.val = w;
	new_xt.code = f_dolit;
	return new_xt;
}


void compile(std::string w) {
	if (w[0] == '"') {
		//string, currently this fucks up with compiling multi word strings!
		w.erase(remove( w.begin(), w.end(), '\"' ),w.end());
		std::string *temp_string = new std::string;
		*temp_string = w;
		code.push_back(make_lit_xt((size_t)temp_string));
	}
	else if ((current_xt=find(macros, w))) {
		current_xt->code();
	}
	else if ((current_xt=find(dictionary, w))) {
		code.push_back(*current_xt);
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
		//string
		w.erase(remove( w.begin(), w.end(), '\"' ),w.end());
		std::string *temp_string = new std::string;
		*temp_string = w;
		ds.push((size_t)temp_string);
		//std::cout << "string:" + w << std::endl;
	}
	else if ((current_xt=find(dictionary, w))) {
		//xt in dict, run function
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

		//std::cout << w << " : " << compile_mode << std::endl;
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
	size_t v1 = ds.top();
	ds.pop();
	size_t v2 = ds.top();
	ds.pop();
	ds.push(v1 + v2);
}

void f_mult() {
	size_t v1 = ds.top();
	ds.pop();
	size_t v2 = ds.top();
	ds.pop();
	ds.push(v1 * v2);
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

//function assigned to colon definitions, runs the xts in the data section
void f_docolon() {

	for (xt foo: current_xt->data) {
		foo.code();
	}
	//interpret current_xt data section here...
}

void f_semicolon() {
	xt *new_word = add_word(definition_name, f_docolon);

	//need to set data section here
	new_word->data = code;
	code.clear();

	//exit compiling
	compile_mode = false;
}

void load_primitives() {
	add_word("+", f_add);
	add_word("*", f_mult);
	add_word(".", f_dot);
	add_word("print", f_print);
	add_word(":", f_colon);
	add_macro(";", f_semicolon); //need to make this a macro!!!
}

int main() {
	//src = "219 2 + . \"foo\" print";
	//src = "2 2 + . \"foo\" print";
	//src = ": +2 2 + ; 10 +2 .";
	//src = ": DOUBLE 2 * ; 10 DOUBLE .";

	load_primitives();

	while (true) {
		src = "";
		std::getline(std::cin, src);
		//std::cout << src << std::endl;
		interpret();
	}
	/*
	std::vector<std::string> words = str_split(code);

	for (std::string s : words) {
		interpret(s);
		//std::cout << s << std::endl;
	}
	*/

	return 0;
}
