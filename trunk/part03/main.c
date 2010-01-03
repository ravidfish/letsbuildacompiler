#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "asmheader.h"

#define strbufsize 1024
#define errbufsize 1024

#define namebufsize 32
#define numbufsize 10

int look;
char namebuf[namebufsize+1];
char numbuf[numbufsize+1];

//Report an error
void error(char *err) {
	fprintf(stderr, "Error: %s.\n", err);
}

//Report error and halt (called abort in original)
void fail(char *err) {
	error(err);
	abort();
}

//Report what was expected and halt
void expected(char *s) {
	char errbuf[errbufsize];
	strncpy(errbuf, s, errbufsize);
	strncat(errbuf, " Expected", errbufsize - strlen(" Expected"));
	fail(errbuf);
}

//Read new character from input stream
void getChar() {
	look = getchar();
	if (ferror(stdin))
		fail("Error reading stdin");
	if (feof(stdin))
		error("EOF on stdin");
}

//Recognize an alpha character
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
	return '+'==c || '-'==c;
}

//Match a specific input character
void match(char c) {
	char err[4] = {"' '"};
	if (look == c) {
		getChar();
		skipWhite();
	}
	else {
		err[1] = c;
		expected(err);
	}
}

//Get an identifier
char *getName() {
	int i=0;
	if (!isAlpha(look))
		expected("Name");
	else {
		while (isAlNum(look)) {
			if (i < namebufsize) //Stop filling buffer when full
				namebuf[i++] = toupper(look);
			getChar(); //Continue reading identifier even if buffer full
		}
		namebuf[i] = 0x00; //Add terminating null
	}
	skipWhite();
	return namebuf;
}

//Get a number
char *getNum() {
	int i = 0;
	if (!isDigit(look))
		expected("Integer");
	else {
		while (isDigit(look)) {
			if (i < numbufsize) //Stop filling buffer when full
				numbuf[i++] = toupper(look);
			getChar(); //Continue reading number even if buffer full
		}
		numbuf[i] = 0x00; //Add terminating null
	}
	skipWhite();
	return numbuf;
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
	skipWhite();
}

void ident() {
	char strbuf[strbufsize];
	char *name;
	name = getName();
	if ('('==look) {
		//Call function
		match('(');
		match(')');
		snprintf(strbuf, strbufsize, "call\t%s # not sure if this works", name);
		emitln(strbuf);
	}
	else {
		//move variable contents into eax
		snprintf(strbuf, strbufsize, "movl\t$%s,%%edx", name);
		emitln(strbuf);
		emitln("mov\t(%edx),%eax");
	}
}

void expression(); //forward declaration

void factor() {
	char strbuf[strbufsize];
	if ('('==look) {
		match('(');
		expression();
		match(')');
	}
	else if (isAlpha(look)) {
		ident();
	}
	else {
		snprintf(strbuf, strbufsize, "movl\t$%s,%%eax", getNum());
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

void assignment() {
	char strbuf[strbufsize];
	char assignName[namebufsize+1];
	
	strlcpy(assignName, getName(), namebufsize+1);
	match('=');
	expression();
	snprintf(strbuf, strbufsize, "movl\t$%s,%%edx", assignName);
	emitln(strbuf);
	emitln("mov\t%eax,(%edx)"); //eax is moved to where edx points
}

int main (int argc, const char * argv[]) {
    init();
#ifdef RELEASE
	asmheader();
#endif
	assignment();
	if ('\n' != look)
		expected("Newline");
#ifdef RELEASE
	asmfooter();
#endif
    return 0;
}
