//Multi-character version from the end of Part VII

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "asmheader.h"

#define CR 0x0D
#define LF 0x0A

#define errbufsize 1024
#define labelbufsize 64
#define tokenbuflen 64

int look;
int lCount = 0;
char labelbuf[labelbufsize+1]; //Extra character for the NULL

//define keywords
#define kwCount 4
const char *kwList[kwCount] = {"IF", "ELSE", "ENDIF", "END"};
const char kwCode[kwCount + 1] = {'x', 'i', 'l', 'e', 'e'};
//typedef enum sym_t {
//	ifSym,
//	elseSym,
//	endifSym,
//	endSym,
//	ident,
//	number,
//	operator
//} symType;

char token; //Current token type
char value[tokenbuflen+1]; //Current token string


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
		fail("EOF on stdin");
}

//Report what was expected and halt
void expected(char *s) {
	char err[errbufsize];
	strncpy(err, s, errbufsize);
	strncat(err, " Expected", errbufsize - strlen(" Expected"));
	fail(err);
}

//Recognize an Alpha character
#define isAlpha(c) isalpha(c)

//Recognize a decimal digit
#define isDigit(c) isdigit(c)

//Recognize alphanumeric character
#define isAlNum(c) isalnum(c)

//Recognize an Addop
int isAddop(char c) {
	return '+'==c || '-'==c;
}

//Recognize a Mulop
int isMulop(char c) {
	return '*'==c || '/'==c;
}

//Recognize whitespace
int isWhite(char c) {
	return ' '==c || '\t'==c;
}

//Recognize an operator
int isOp(char c) {
	return strchr("+-*/<>:=", c) != NULL;
}

//Skip leading white space
void skipWhite() {
	while (isWhite(look))
		getChar();
}

//Recognize EOL
int isEOL(char c) {
	return CR==c || LF==c;
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

//Match a specific input string
void matchString(char *s) {
	if (!(0 == strcmp(value, s))) {
		expected(s);
	}
}

//Skip EOL
void fin() {
	while (isEOL(look)) {
		getChar();
		skipWhite();
	}
}

//Table lookup
//If the input string matches a table entry, return the
//  entry index. If not, return -1
int lookup(const char *tab[], char *s, int tableLength) {
	int found = 0;
	int i = tableLength;
	while (!found && --i>=0) {
		found = (0 == strcmp(tab[i], s));
	}
	return i;
}

//Get an identifier
char *getName() {
	int bufInx = 0;
	while (isEOL(look))
		fin();
	if (!isAlpha(look))
		expected("Name");
	while (isAlNum(look) && bufInx < tokenbuflen) {
		value[bufInx++] = toupper(look);
		getChar();
	}
	value[bufInx] = 0x00; //Terminate the string
	if (isAlNum(look))
		fail("Name exceeds maximum length");
	skipWhite();	
	return value;
}

//Get a number
char *getNum() {
	int bufInx = 0;
	if (!isDigit(look))
		expected("Integer");
	while (isDigit(look) && bufInx < tokenbuflen) {
		value[bufInx++] = look;
		getChar();
	}
	value[bufInx] = 0x00; //Terminate the string
	if (isDigit(look))
		fail("Number exceeds maximum length");
	token = '#';
	skipWhite();
	return value;
}

void scan() {
	getName();
	token = kwCode[lookup(kwList, value, kwCount) + 1];
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
	getName();
	if ('(' == look) {
		match('(');
		match(')');
		emitln("call\t%s", value); //Call a procedure
	}
	else {
		emitln("mov\t%s,%%eax", value); //Move variable into eax
	}
}

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
		emitln("movl\t$%s,%%eax", getNum());
	}
}

//Recognize and translate the first math factor
void signedFactor() {
	int isNegative = '-' == look;
	
	if (isAddop(look)) {
		getChar();
		skipWhite();
	}
	factor();
	if (isNegative) {
		emitln("neg\t%%eax");
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

//Completion of Term processing (called by term() and firstTerm())
void term1() {
	while (isMulop(look)) {
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

//Recognize and translate <term>
void term() {
	factor();
	term1();
}

//Recognize and translate a math term with possible leading sign
void firstTerm() {
	signedFactor();
	term1();
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


//Recognize and translate an expression
void expression() {
	firstTerm();
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

//Recognize and translate a Boolean condition
//Dummy version
void condition() {
	emitln("Condition");
}

void block();

//Recognize and translate IF construct
void doIf() {
	char label1[labelbufsize];
	char label2[labelbufsize];
	
	emitln("#IF");
	condition();
	newLabel(label1);
	strlcpy(label2, label1, labelbufsize);
	emitln("je\t%s\t", label1);
	block();
	if ('l' == token) {
		newLabel(label2);
		emitln("jmp\t%s", label2);
		postLabel(label1, "#ELSE");
		block();
	}
	postLabel(label2, "#ENDIF");
	matchString("ENDIF");
}

//Recognize and Translate an assignment statement
void assignment() {
	char name[tokenbuflen];
	strlcpy(name, value, tokenbuflen);
	match('=');
	expression();
	emitln("mov\t%%eax,%s", name);
}

//Recognize and translate a statement block
void block() {
	scan();
	while ('e' != token && 'l' != token) {
		switch (token) {
			case 'i':
				doIf();
				break;
			default:
				assignment();
		}
		scan();
	}
}

//Parse and translate a program
void doProgram() {
	block();
	matchString("END");
	emitln("END");
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
	doProgram();
	
#ifdef RELEASE
	asmfooter();
#endif
    return 0;
}
