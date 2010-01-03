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

//Output a printf-style formatted string and arguments with tab and newline
void emitln(char *s, ...) {
	va_list args;
	va_start(args, s);
	printf("\t");
	vprintf(s, args);
	printf("\n");
	va_end(args);
}

//Process Label Statement
void labels() {
	match('l');
}

//Process Const Statement
void constants() {
	match('c');
}

//Process Type Statement
void types() {
	match('t');
}

//Process Var Statement
void variables() {
	match('v');
}

//Process Procedure Definition
void doProcedure() {
	match('p');
}

//Process Function Definition
void doFunction() {
	match('f');
}

void declarations() {
	while (strchr("lctvpf", look)) {
		switch (look) {
			case 'l':
				labels();
				break;
			case 'c':
				constants();
				break;
			case 't':
				types();
				break;
			case 'v':
				variables();
				break;
			case 'p':
				doProcedure();
				break;
			case 'f':
				doFunction();
				break;
		}
	}
}

//Parse and translate the Statement Part
void statements() {
	match('b');
	while ('e' != look) {
		getChar();
	}
	match('e');
}

//Parse and translate a Pascal block
void doBlock(char name) {
	char progName[2] = {name, 0x00};
	declarations();
	postLabel(progName, "# Program entry point");
	statements();
}

void prolog(char name) {
#ifdef RELEASE
	asmheader();
#else
	emitln("PROLOG %c", name);
#endif
}

void epilog(char name) {
#ifdef RELEASE
	asmfooter();
#else
	emitln("EPILOG %c", name);
#endif
}

void prog() {
	char name;
	match('p');
	name = getName();
	prolog(name);
	doBlock(name);
	match('.');
	epilog(name);
}

//Initialize
void init() {
	getChar();
}

int main (int argc, const char * argv[]) {
    init();
	
	prog();
	
    return 0;
}
