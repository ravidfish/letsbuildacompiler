#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "asmheader.h"

#define strbufsize 1024
#define errbufsize 1024

int look;

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

//Report what was expected and halt
void expected(char *s) {
	char errbuf[errbufsize];
	strncpy(errbuf, s, errbufsize);
	strncat(errbuf, " Expected", errbufsize - strlen(" Expected"));
	fail(errbuf);
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

//Recognize an alpha character
#define isAlpha(c) isalpha(c)

//Recognize a decimal digit
#define isDigit(c) isdigit(c)

//Recognize an Addop
int isAddop(char c) {
	return '+'==c || '-'==c;
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

//Output a string with tab and newline
void emitln(char *s) {
	printf("\t%s\n", s);
}

//Initialize
void init() {
	getChar();
}

void expression(); //forward declaration

void factor() {
	char strbuf[strbufsize];
	if ('('==look) {
		match('(');
		expression();
		match(')');
	}
	else {
		snprintf(strbuf, strbufsize, "movl\t$%c,%%eax", getNum());
		emitln(strbuf);
	}
}

void multiply() {
	match('*');
	factor();
	emitln("pop\t%ebx"); //pop top of stack to ebx
	emitln("mul\t%ebx"); //multiply eax by ebx
}

void divide() {
	match('/');
	factor();
	emitln("mov\t%eax,%ebx"); //move second factor to ebx
	emitln("pop\t%eax"); //pop first factor into eax
	emitln("xor\t%edx,%edx"); //clear high word of dividend
	emitln("div\t%ebx"); //divide first factor by second factor
}

void term() {
	factor();
	while ('*'==look || '/'==look) {
		emitln("push\t%eax");
		switch (look) {
			case '*':
				multiply();
				break;
			case '/':
				divide();
				break;
			default:
				expected("Mulop");
				break;
		}
	}
}

void add() {
	match('+');
	term();
	emitln("pop\t%ebx"); //pop top of stack to ebx
	emitln("add\t%ebx,%eax"); //add ebx to eax
}

void subtract() {
	match('-');
	term();
	emitln("pop\t%ebx"); //pop top of stack to ebx
	emitln("sub\t%ebx,%eax"); //subtract ebx from eax
	emitln("neg\t%eax"); //negate eax to fix sign error
}

void expression() {
	if (isAddop(look))
		emitln("xor\t%eax,%eax"); //unary - or + so first operand is zero
	else
		term();
	while (isAddop(look)) {
		emitln("push\t%eax"); //push eax to top of stack
		switch (look) {
			case '+':
				add();
				break;
			case '-':
				subtract();
				break;
			default:
				expected("Addop");
				break;
		}
	}
}

int main (int argc, const char * argv[]) {
    init();
#ifdef RELEASE
	asmheader();
#endif
	expression();
#ifdef RELEASE
	asmfooter();
#endif
    return 0;
}
