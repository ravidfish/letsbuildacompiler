#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "asmheader.h"

#define strbufsize 1024
#define errbufsize 1024

int look;
int table[26];

void initTable() {
	int i;
	for (i=0; i<26; i++) {
		table[i] = 0;
	}
}

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

//Recognize and skip over newline
void newLine() {
	while ('\n' == look || '\r' == look) 
		getChar();
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
int getNum() {
	int retVal = 0;
	if (!isDigit(look))
		expected("Integer");
	else {
		while (isDigit(look)) {
			retVal = 10 * retVal + look - '0';
			getChar();
		}
	}
	return retVal;
}

//Accept input to a variable
void input() {
	char name;
	match('?');
	name = getName();
	printf("%c: ", name);
	scanf("%d", &table[name-'A']);
}

//Write a variable to output
void output() {
	char name;
	match('!');
	name = getName();
	printf("%c=%d\n", name, table[name-'A']);
}

//Output a string with a leading tab
void emit(char *s) {
	printf("\t%s", s);
}

//Output a string with tab and newline
void emitln(char *s) {
	printf("\t%s\n", s);
}

int expression();

int factor() {
	int retVal;
	if ('(' == look) {
		match('(');
		retVal = expression();
		match(')');
	}
	else if (isAlpha(look)) {
		retVal = table[getName()-'A'];
	}
	else {
		retVal = getNum();
	}
	return retVal;
}

int term() {
	int retVal;
	retVal = factor();
	while ('*'==look || '/'==look) {
		switch (look) {
			case '*':
				match('*');
				retVal *= factor();
				break;
			case '/':
				match('/');
				retVal /= factor();
				break;
		}
	}
	return retVal;
}

int expression() {
	int retVal;
	if (isAddop(look)) {
		retVal = 0;
	}
	else {
		retVal = term();
	}
	while (isAddop(look)) {
		switch (look) {
			case '+':
				match('+');
				retVal += term();
				break;
			case '-':
				match('-');
				retVal -= term();
				break;
		}
	}
	return retVal;
}

void assignment() {
	char name;
	name = getName();
	match('=');
	table[name - 'A'] = expression();
}

//Initialize
void init() {
	initTable();
	getChar();
}

int main (int argc, const char * argv[]) {
    init();
#ifdef RELEASE
	asmheader();
#endif
	do {
		switch (look) {
			case '?':
				input();
				break;
			case '!':
				output();
				break;
			default:
				assignment();
				break;
		}
		newLine();
	} while ('.' != look);
	
#ifdef RELEASE
	asmfooter();
#endif
    return 0;
}
