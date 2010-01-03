#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "asmheader.h"

#define errbufsize 1024
#define labelbufsize 10

int look;
int lCount = 0;

//Report an error
void error(char *err) {
	fprintf(stderr, "Error: %s.\n", err);
}

//Report error and halt
void fail(char *err) {
	error(err);
	abort();
}

//Read new character from input stream
void getChar() {
	look = getchar();
	if (ferror(stdin))
		fail("Error reading stdin");
	if (feof(stdin))
		error("EOF on stdin");
}

//Generate a Unique lable
//Copy the label into theLabel
//Returns pointer to new label
char *newLabel(char *theLabel) {
	snprintf(theLabel, labelbufsize, "L%02d", lCount++);
	return theLabel;
}

//Post a label and comment to output
void postLabel(char *theLabel, char *comment) {
	printf("%s:\t%s\n", theLabel, comment);
}

//Report what was expected and halt
void expected(char *s) {
	char err[errbufsize];
	strncpy(err, s, errbufsize);
	strncat(err, " Expected", errbufsize - strlen(" Expected"));
	fail(err);
}

//Match a specific input character
void match(char c) {
	char err[4] = {"' '"};
	if (look == c)
		getChar();
	else {
		err[1] = c;
		expected(err);
	}
}

//Recognize an Alpha character
#define isAlpha(c) isalpha(c)

//Recognize a decimal digit
#define isDigit(c) isdigit(c)

//Recognize alphanumeric character
#define isAlNum(c) isalnum(c)

//Recognize whitespace
int isWhite(char c) {
	return ' '==c || '\t'==c;
}

//Skip leading white space
void skipWhite() {
	while (isWhite(look))
		getChar();
}

//Recognize an Addop
int isAddop(char c) {
	return '+' == c || '-' == c;
}

//Recognize a Boolean or operation
int isOrOp(char c) {
	return '|' == c || '~' == c;
}

//Recognize a relop
int isRelop(char c) {
	return '=' == c || '#' == c || '<' == c || '>' == c;
}

//Recognize a Boolean literal
int isBoolean(char c) {
	c = toupper(c);
	return ('T' == c || 'F' == c);
}

//Get a Boolean literal
int getBoolean() {
	int retVal;
	if (!isBoolean(look))
		expected("Boolean literal");
	retVal = 'T' == toupper(look);
	getChar();
	return retVal;
}

//Get an identifier
char getName() {
	char retval = 0;
	if (!isAlpha(look))
		expected("Name");
	else {
		retval = toupper(look);
		getChar();
	}
	return retval;
}

//Get a number
char getNum() {
	char retval = 0;
	if (!isDigit(look))
		expected("Integer");
	else {
		retval = look;
		getChar();
	}
	return retval;
}

//Output a string with a leading tab
void emit(char *s) {
	printf("\t%s", s);
}

//Output a printf-style formatted string and arguments with tab and newline
void emitln(char *s, ...) {
	va_list args;
	va_start(args, s);
	printf("\t");
	vprintf(s, args);
	printf("\n");
	va_end(args);
}

//Recognize and translate an identifier
void ident() {
	char name;
	name = getName();
	if ('(' == look) {
		match('(');
		match(')');
		emitln("call\t%c", name); //Call a procedure
	}
	else {
		emitln("mov\t%c,%%eax", name); //Move variable into eax
	}
}

/*
Grammar:
    <b-expression> ::= <b-term> [<orop> <b-term>]* 
    <b-term>       ::= <not-factor> [and <not-factor>]* 
    <not-factor>   ::= [NOT] <b-factor> 
    <b-factor>     ::= <b-literal> | <b-variable> | <relation> 
    <relation>     ::= <expression> [<relop> <expression>] 
    <expression>   ::= <term> [<addop> <term>]* 
    <term>         ::= <signed factor> [<mulop> factor]* 
    <signed factor>::= [<addop>] <factor> 
    <factor>       ::= <integer> | <variable> | (<b-expression>)
*/

void expression(); //forward declaration

//Recognize and translate a math factor
void factor() {
	if ('('==look) { //<b-expression>
		match('(');
		expression();
		match(')');
	}
	else if (isAlpha(look)) { //variable>
		ident();
	}
	else { //<integer>
		emitln("movl\t$%c,%%eax", getNum());
	}
}

//Recognize and translate the first math factor
void signedFactor() {
	if ('+' == look) {
		getChar();
	}
	if ('-' == look) {
		getChar();
		if (isDigit(look)) {
			emitln("mov\t$-%c,%%eax", getNum());
		}
		else {
			factor();
			emitln("neg\t%%eax");
		}
	}
	else {
		factor();
	}
}

//Recognize and translate multiply
void multiply() {
	match('*');
	factor();
	emitln("pop\t%%ebx"); //pop top of stack to ebx
	emitln("mul\t%%ebx"); //multiply eax by ebx
}

//Recognize and translate divide
void divide() {
	match('/');
	factor();
	emitln("mov\t%%eax,%%ebx"); //move second factor to ebx
	emitln("pop\t%%eax"); //pop first factor into eax
	emitln("xor\t%%edx,%%edx"); //clear high word of dividend
	emitln("div\t%%ebx"); //divide first factor by second factor
}

//Recognize and translate <term>
void term() {
	signedFactor();
	while ('*'==look || '/'==look) {
		emitln("push\t%%eax");
		switch (look) {
			case '*':
				multiply();
				break;
			case '/':
				divide();
				break;
		}
	}
}

//Recognize and translate add
void add() {
	match('+');
	term();
	emitln("pop\t%%ebx"); //pop top of stack to ebx
	emitln("add\t%%ebx,%%eax"); //add ebx to eax
}

//Recognize and translate subtract
void subtract() {
	match('-');
	term();
	emitln("pop\t%%ebx"); //pop first operand to ebx
	emitln("sub\t%%ebx,%%eax"); //subtract ebx from eax
	emitln("neg\t%%eax"); //negate eax to fix sign error
}

void expression() {
	term();
	while (isAddop(look)) {
		emitln("push\t%%eax"); //push eax to top of stack
		switch (look) {
			case '+':
				add();
				break;
			case '-':
				subtract();
				break;
		}
	}
}

//Recognize and translate Relational equals
void equals() {
	match('=');
	expression();
	emitln("pop\t%%ebx");
	emitln("cmp\t%%eax,%%ebx");
	emitln("sete\t%%eax");
}

//Recognize and translate Relational not equals
void notEquals() {
	match('#');
	expression();
	emitln("pop\t%%ebx");
	emitln("cmp\t%%eax,%%ebx");
	emitln("setne\t%%eax");
}

//Recognize and translate Relational less than
void less() {
	match('<');
	expression();
	emitln("pop\t%%ebx");
	emitln("cmp\t%%eax,%%ebx");
	emitln("setl\t%%eax"); //Set eax if ebx<eax
}

//Recognize and translate Relational greater than
void greater() {
	match('>');
	expression();
	emitln("pop\t%%ebx");
	emitln("cmp\t%%eax,%%ebx");
	emitln("setg\t%%eax"); //Set eax if ebx>eax
}

//Recognize and translate <relation>
void relation() {
	expression();
	if (isRelop(look)) {
		emitln("push\t%%eax");
		switch (look) {
			case '=':
				equals();
				break;
			case '#':
				notEquals();
				break;
			case '<':
				less();
				break;
			case '>':
				greater();
				break;
		}
		emitln("test\t%%eax");
	}
}

//Recognize and translate <b-factor>
void boolFactor() {
	if (isBoolean(look)) {
		if (getBoolean())
			emitln("mov\t$-1,%%eax");
		else
			emitln("mov\t$0,%%eax");
	}
	else {
		relation();
	}
}

//Recognize and translate <not-factor>
void notFactor() {
	if ('!' == look) {
		match('!');
		boolFactor();
		emitln("not\t%%eax");
	}
	else
		boolFactor();
}

//Recognize and translate <b-term>
void boolTerm() {
	notFactor();
	while ('&' == look) {
		emitln("push\t%%eax");
		match('&');
		notFactor();
		emitln("pop\t%%ebx");
		emitln("and\t%%ebx,%%eax");
	}
}

//Recognize and transalte a Boolean or statement
void boolOr() {
	match('|');
	boolTerm();
	emitln("pop\t%%ebx");
	emitln("or\t%%ebx,%%eax");
}

//Recognize and transalte a Boolean or statement
void boolXor() {
	match('~');
	boolTerm();
	emitln("pop\t%%ebx");
	emitln("xor\t%%ebx,%%eax");
}

void boolExpression() {
	boolTerm();
	while (isOrOp(look)) {
		emitln("push\t%%eax");
		switch (look) {
			case '|':
				boolOr();
				break;
			case '~':
				boolXor();
				break;
		}
	}
}

//Initialize
void init() {
	getChar();
}

int main (int argc, const char * argv[]) {
    init();
#ifdef RELEASE
	asmheader();
#endif
	
	boolExpression();
	
#ifdef RELEASE
	asmfooter();
#endif
    return 0;
}
