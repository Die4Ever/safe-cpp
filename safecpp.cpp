#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
using namespace std;

/*
convert * to a multiply function to prevent use of pointers
convert & to a function to prevent use of pointers, but I need to allow &&, shouldn't be hard
default to const
block type names?
block auto? or detect literals to convert them to the class before assigning to the auto?
block new, delete, malloc, free, using
replace string literals with a string object
replace arrays with std::array
how do I handle this? if I replace self with *this then that can be exploited like...
	int this;
default include approved standard headers?
should I flatten the token vectors?
should I check blocked tokens in stage 2 instead?
need to block functions like printf, sprintf, scanf or maybe wrap them as the safe versions with bounds checking, using an std::array or string as the argument instead of a char*
block typedef?

I can either trust myself to keep * when proceeding a number, or I can use MakeNumber to deduce the type and return a Number<T> with .mult

module system? should I try to automatically split out declarations from definitions? or require the programmer to separate them?

how to do references? if I just overload the & operator to a private throwing function then I should be fine to allow it?

I should really make the parser build a tree... and use a class object for the tokens instead of just a string...
*/

struct Int
{
	int i;
	Int mult(Int b)
	{
		Int r = i*b.i;
		return r;
	}

	Int()
	{
	}

	Int(int a)
	{
		i = a;
	}

	string ToString()
	{
		char buff[64];
#ifdef WIN32
		sprintf_s(buff, 64, "%d", i);
#else
		snprintf(buff, 64, "%d", i);
#endif
		return string(buff);
	}

	Int operator++() { ++i; return *this; }
	Int operator++(int) { auto temp = *this; i++; return temp; }
	template<class T>
	bool operator<(T b) { return i < b; }
	operator int() { return i; }

private:
	Int* operator&() const { throw runtime_error("Int::operator&()"); return NULL; }
	Int operator*() const { throw runtime_error("Int::operator*()"); return *this; }
	Int operator*(const Int b) const { throw runtime_error("Int::operator*()"); return *this; }
};

template<class T>
struct Number
{
	T i;
	
	Number<T>() {}
	
	Number<T>(T a)
	{
		i = a;
	}
	
	T mult(Number<T> b)
	{
		T r = b.i*i;
		return r;
	}
	
	operator T() { return i; }
	Number<T> operator++() { ++i; return *this; }
	Number<T> operator++(int) { auto temp = *this; i++; return temp; }
	Number<T> operator+(Number<T> b) { return Number<T>(i+b.i); }
};

template<class T>
Number<T> MakeNumber(T i)
{
	return Number<T>(i);
}

//states...
const int NORMAL = 1;
const int MULT_START = 2;
const int MULT_END = 3;
const int IDENTIFIER = 4;
const int WHITESPACE = 5;
const int NUMBER = 6;
const int STRING = 7;
const int CHAR = 8;
const int OTHER = 111;


string sub_process(vector<string> &tokens, Number<unsigned int> &line, Number<unsigned int> &i, string &errors, string &prev)
{
	int state=NORMAL;
	int mults=0;
	string out;
	//if I find a ( or [ or { then I recurse, if find my matching closing then return
	for(;i<tokens.size();i++){
		auto &c = tokens[i];
		if(c.length()==1) {
			char t = c[0];
			if(t=='\n') {
				out += c;
				line++;
			}
			else if(t=='*' /*&& prev[0]!='n'*/) {
				state=MULT_START;
				mults++;
				//out.resize(out.length()-1);
				out += ".mult(";
			}
			else if(t==';' && state==MULT_START) {
				//out.resize(out.length()-1);
				//out+=");";
				while(mults--) out+=")";
				mults=0;
				out+=c;
				prev=c;
				state=NORMAL;
			}
			else if( strchr("[{(", t)) {
				out += c;
				i++;
				prev=c;
				out += sub_process(tokens, line, i, errors, prev);
			} else if( strchr("]})", t)) {
				out += c;
				if(state==MULT_START) {
					while(mults--) out+=")";
					mults=0;
					state=NORMAL;
				}
				prev=c;
				return out;
			}
			else {
				out += c;
				prev=c;
			}
		}
		else if(c.length()>1) {
			char t = c[0];
			if(t!='w') {
				prev=c;
			}
			
			if(t=='n') out += "MakeNumber("+c.substr(1)+")";
			else if (t == 's') out += "string(" + c.substr(1) + ")";
			else out += c.substr(1);
			
			if(t!='w') if(state==MULT_START) { state=NORMAL; while(mults--) out+=")"; mults=0; }
		} else {
			errors += "warning: empty token on line "+line.ToString()+"\n";
		}
	}
	//out += "\n";
	//i=0;
	return out;
}

string process(const string in)
{
	string errors = "";
	//Int line = 1;
	vector<string> tokens;

	Int line = 1;
	int state = NORMAL;

	for (size_t i = 0; i < in.length(); i++) {
		if (state != STRING && in[i] == '\"') {
			state = STRING;
			tokens.push_back("s");
			tokens.back() += in[i];
		}
		else if (state == STRING) {
			tokens.back() += in[i];
			if (in[i] == '\\') {
				i++;
				tokens.back() += in[i];
			}
			else if (in[i] == '\"' || in[i] == '\n') {
				state = NORMAL;
			}
		}
		else if (state != CHAR && in[i] == '\'') {
			state = CHAR;
			tokens.push_back("c");
			tokens.back() += in[i];
		}
		else if (state == CHAR) {
			tokens.back() += in[i];
			if (in[i] == '\\') {
				i++;
				tokens.back() += in[i];
			}
			else if (in[i] == '\'' || in[i] == '\n') {
				state = NORMAL;
			}
		}
		else if (
			state != IDENTIFIER &&
			(
			in[i] == '_' ||
			(in[i] >= 'a' && in[i] <= 'z') ||
			(in[i] >= 'A' && in[i] <= 'Z')
			)
			) {
			state = IDENTIFIER;
			tokens.push_back("i");
			tokens.back() += in[i];
		}
		else if (state == IDENTIFIER) {
			if (in[i] == '_' ||
				(in[i] >= 'a' && in[i] <= 'z') ||
				(in[i] >= 'A' && in[i] <= 'Z') ||
				(in[i] >= '0' && in[i] <= '9')
				) {
				tokens.back() += in[i];
			}
			else {
				auto &s = tokens.back();
				if (s == "inew" || s == "idelete" || s == "imalloc" || s == "ifree" || s == "using" || s == "iauto" || s == "itypedef") {
					errors += "error: " + s.substr(1) + " found on line " + line.ToString() + "\n";
					tokens.pop_back();
				}
				state = NORMAL;
				i--;//go back and add this character as whatever type it is
				//lines.back().push_back("o");
				//lines.back().back() += in[i];
			}
		}
		else if (in[i] >= '0' && in[i] <= '9') {
			if(state != NUMBER) {
				tokens.push_back("n");
				state = NUMBER;
			}
			tokens.back() += in[i];
		}
		else if(in[i]=='.' && state==NUMBER) {
			tokens.back() += in[i];
		}
		else if (strchr(";[]{}():\'\"<>,.?|", in[i])) {//less than and greater than signs could be extremely tricky, ignore them for now?
			char b[2];
			b[0] = in[i];
			b[1] = 0;
			tokens.push_back(b);
			state = NORMAL;
		}
		else if (in[i] == '*') {
			tokens.push_back("*");
			state = NORMAL;
		}
		else if (in[i] == '&') {
			if (in.length() > i + 1 && in[i + 1] == '&') {
				tokens.push_back("&&&");//if cell length is 1 then the prefix is the character, else ignore the prefix??
				state = NORMAL;
				i++;
				continue;
			}
			tokens.push_back("w ");//maybe check the state and append if already in whitespace
			state = WHITESPACE;
			errors += "error: & found on line " + line.ToString() + "\n";
			continue;
			tokens.push_back("&");
			state = NORMAL;
		}
		else if (in[i] == '#') {
			errors += "error: # found on line " + line.ToString() + "\n";
			continue;
			tokens.push_back("#");
			state = NORMAL;
		}
		else if (in[i] == '\n') {
			//cout << "found newline at end of line "+line.ToString()+"\n";
			tokens.push_back("\n");
			state = NORMAL;
			line++;
		}
		else if (in[i] == ' ' || in[i] == '\t' || in[i] == '\r') {
			//continue;//ignore whitespace
			if (state != WHITESPACE) {
				tokens.push_back("w");
				state = WHITESPACE;
			}
			tokens.back() += in[i];
		}
		else if (state == OTHER) tokens.back() += in[i];
		else {
			state = OTHER;
			tokens.push_back("o");
			tokens.back() += in[i];
		}
	}

	for(auto s : tokens) {if(s=="\n") s="\\n\n"; if(s=="\r") s="\\r"; cout << s;}
	cout << "\n======================\n";
	state = NORMAL;
	line = 1;
	Int i = 0;
	//string out = "#define int Int\n#define self ___getself(this)\ntemplate<class T>\nT& ___getself(T* t) {return *t;}";
	string out = "\ntypedef Number<int> int;\n#define self ___getself(this)\ntemplate<class T>\nT& ___getself(T* t) {return *t;}";
	string prev="w ";
	out += sub_process(tokens, line, i, errors, prev);

	/*state = 1;
	for (size_t i = 0; i < in.length(); i++) {
		if (in[i] == '*') {
			out += ".mult(";
			state = MULT_START;
		}
		else if (in[i] == '&') {
			errors += "& found on line "+line.ToString()+"\n";
			out += ' ';
		}
		else if (in[i] == '#') {
			errors += "& found on line "+line.ToString()+"\n";
			out += ' ';
		}
		else out += in[i];

		if (state == MULT_START) {
			if (in[i]=='_' ||
				(in[i]>='a' && in[i]<='z') ||
				(in[i] >= 'A' && in[i] <= 'Z')
				) {
				state = MULT_END;
			}
			else if (in[i] == ';') {
				out[out.length() - 1] = ')';
				out += in[i];
				state = NORMAL;
				errors += "; found when looking for operand to multiplication at line " + line.ToString() + "\n";
			}
		}
		else if (state == MULT_END) {
			if (in[i] == '_' ||
				(in[i] >= 'a' && in[i] <= 'z') ||
				(in[i] >= 'A' && in[i] <= 'Z') ||
				(in[i] >= '0' && in[i] <= '9')
				) {
			}
			else {
				out[out.length()-1] = ')';
				out += in[i];
				state = NORMAL;
			}
		}

		if (in[i] == '\n') line++;
	}*/
	//out += "\n#undef int\n#undef self\n";
	out += "\n#undef self\n";

	if (errors.length()) cerr << "\n" << errors << "\n";
	return out;
}

//Number<int> test();

int main()
{
	std::array<char, 11> arr;
	arr.operator[](2) = 5;

	string s = "\n\
int test() {\n\
	int inta=7;\n\
	int intb=9;\n\
	inta = intb*3*4*(5+6);\n\
	int * intp = &inta;\n\
	string s = \"blablabla test\";\n\
	return inta*intb;\n\
}";

	cout << "\n\n" << process(s) << "\n\n\n";
	//test();
	return 0;
}

/*Number<int> operator"" _i (int i)
{
	return Number<int>(i);
}*/

/*#define int Number<int>
int test() {
	int inta=MakeNumber(7);
	int intb=MakeNumber(9);
	int intc=11_i .mult(5);
	cout << intc.ToString()<<"\n";
	
	inta = intb.mult(MakeNumber(3).mult(MakeNumber(4).mult((MakeNumber(5)+MakeNumber(6)))));
	//int .mult( intp =  inta);
	return inta.mult(intb);
}
*/
