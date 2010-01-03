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

//Output a string with tab and newline
void emitln(char *s) {
	printf("\t%s\n", s);
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

//Code goes here

#ifdef RELEASE
	asmfooter();
#endif
    return 0;
}
