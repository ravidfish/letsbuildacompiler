//This version is the complete implementation of TINY 1.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "asmheader.h"

#define LF 0x0A
#define CR 0x0D

#define errbufsize 1024
#define labelbufsize 10
#define tokenbuflen 64
#define maxSymbols 100

int look;
int labelCount = 0;
int symbolCount = 0;

char *symbolTable[maxSymbols];
char symbolType[maxSymbols];

//define keywords and token types
#pragma mark Keyaords and Token Types
#define kwCount 11
char *kwList[kwCount] = {"IF", "ELSE", "ENDIF", "WHILE", "ENDWHILE",
"READ", "WRITE", "VAR", "BEGIN", "END", "PROGRAM"};
const char kwCode[kwCount + 1] = {"xileweRWvbep"};

char token; //Current token type
char value[tokenbuflen]; //Current token string

//Report an error
void error(char *err) {
	fprintf(stderr, "Error: %s.\n", err);
}

//Report error and halt
void fail(char *err, ...) {
	char errstr[errbufsize];
	va_list args;
	va_start(args, err);
	vsnprintf(errstr, errbufsize, err, args);
	error(errstr);
	va_end(args);
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
	snprintf(theLabel, labelbufsize, "L%05d", labelCount++);
	return theLabel;
}

//Post a label and comment to output
void postLabel(char *theLabel, char *comment) {
	printf("%s:\t%s\n", theLabel, comment);
}

//Report what was expected and halt
void expected(char *s) {
	fail("%s Expected", s);
}

//Report an undefined identifier
void undefined(char *name) {
	fail("Undefined identifier: %s", name);
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

//Recognize EOL
int isEOL(char c) {
	return CR==c || LF==c;
}

//Recognize an Addop
int isAddop(char c) {
	return '+'==c || '-'==c;
}

//Recognize a Mulop
int isMulop(char c) {
	return '*'==c || '/'==c;
}

//Recognize a Boolean Orop
int isOrop(char c) {
	return '|'==c || '~'==c;
}

//Recognize a Relop (relational operation)
int isRelop(char c) {
	return strchr("=#<>", c) != NULL;
}

//Table lookup
//If the input string matches a table entry, return the
//  entry index. If not, return -1
//  A NULL pointer in the table represents the end of the table
int lookup(char *tab[], char *s, int tableLength) {
	int found = 0;
	int i = -1;
	while (!found && ++i<tableLength) {
		if (NULL == tab[i]) {
			//End of table. Search is unsuccessful.
			i = -1;
			break;
		}
		else {
			found = (0 == strcmp(tab[i], s));
		}
	}
	if (i >= tableLength) i = -1;
	return i;
}

//Look for Symbol in Table
int inTable(char *symbol) {
	return -1 != lookup(symbolTable, symbol, maxSymbols);
}

//Add a new entry to symbol table
void addEntry(char *symbol, char symType) {
	if (inTable(symbol)) {
		fail("Duplicate Identifier: %s", symbol);
	}
	if (symbolCount >= maxSymbols) {
		fail("Symbol Table Full");
	}
	symbolTable[symbolCount] = malloc(strlen(symbol)+1); //Allocate memory for the symbol
	strcpy(symbolTable[symbolCount], symbol); //Copy the symbol into the array
	symbolType[symbolCount] = symType;
	symbolCount++;
}

//Skip leading white space
void skipWhite() {
	while (isWhite(look))
		getChar();
}

//Skip over multiple end-of-lines
void newLine() {
	while (isEOL(look)) {
		getChar();
		skipWhite();
	}
}

//Match a specific input character
void match(char c) {
	char err[4] = {"' '"};
	newLine();
	if (look == c)
		getChar();
	else {
		err[1] = c;
		expected(err);
	}
	skipWhite();
}

//Match a specific input string
void matchString(char *s) {
	if (!(0 == strcmp(value, s))) {
		expected(s);
	}
}

//Get an identifier
char *getName() {
	int bufInx = 0; //index into "value" string
	newLine();
	if (!isAlpha(look))
		expected("Name");
	while (isAlNum(look) && bufInx < tokenbuflen - 1) {
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
int getNum() {
	int retval = 0;
	newLine();
	if (!isDigit(look))
		expected("Integer");
	while (isDigit(look)) {
		retval = 10 * retval + look - '0';
		getChar();
	}
	skipWhite();
	return retval;
}

//Get an identifier and scan it for keywords
void scan() {
	getName();
	token = kwCode[lookup(kwList, value, kwCount) + 1];
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

//
#pragma mark Code Generation Routines
//

//Complement the primary register
void notIt() {
	emitln("not\t%%eax");
}

//Clear the primary register
void clear() {
	emitln("mov\t$0,%%eax");
}

//Negate the primary register
void negate() {
	emitln("neg\t%%eax");
}

//Store primary register to variable
void store(char *name) {
	if (!inTable(name)) {
		undefined(name);
	}
	emitln("mov\t%%eax,%s", name);
}

//Load a constant value to the primary register
void loadConst(int n) {
	emitln("mov\t$%d,%%eax", n);
}

//Load a variable to the primary register
void loadVar(char *name) {
	if (!inTable(name)) {
		undefined(name);
	}
	emitln("mov\t%s,%%eax", name);
}

//Read a variable (whose name is in value variable) into the primary register
void readVar() {
	emitln("call\t_readIobuf");
	emitln("call\t_convertFromAscii");
	store(value);
}

//Write value in primary register
void writeVar() {
	emitln("call\t_convertToAscii");
	emitln("call\t_writeIobuf");
}

//Push primary register onto stack
void push() {
	emitln("push\t%%eax");
}

//AND top of stack with primary register
void popAnd() {
	emitln("pop\t%%ebx"); //pop top of stack to ebx
	emitln("and\t%%ebx,%%eax"); //and ebx to eax
}

//OR top of stack with primary register
void popOr() {
	emitln("pop\t%%ebx"); //pop top of stack to ebx
	emitln("or\t%%ebx,%%eax"); //or ebx to eax
}

//XOR top of stack with primary register
void popXor() {
	emitln("pop\t%%ebx"); //pop top of stack to ebx
	emitln("xor\t%%ebx,%%eax"); //xor ebx to eax
}

//Compare top of stack with primary
void popCompare() {
	emitln("pop\t%%ebx"); //pop top of stack to ebx
	emitln("cmp\t%%eax,%%ebx"); //compare ebx with eax
	//Keep in mind that in AT&T syntax, cmp looks backward
	//jg will jump if ebx>eax
}

//Set eax if compare was =
void setEqual() {
	emitln("sete\t%%al");
	emitln("neg\t%%al\t#change 1 to -1"); //TRUE is -1 in this language
	emitln("movsx\t%%al,%%eax\t#extend al to eax");
}

//Set eax if compare was !=
void setNEqual() {
	emitln("setne\t%%al");
	emitln("neg\t%%al\t#change 1 to -1"); //TRUE is -1 in this language
	emitln("movsx\t%%al,%%eax\t#extend al to eax");
}

//Set eax if compare was >
void setGreater() {
	emitln("setg\t%%al");
	emitln("neg\t%%al\t#change 1 to -1"); //TRUE is -1 in this language
	emitln("movsx\t%%al,%%eax\t#extend al to eax");
}

//Set eax if compare was >=
void setGreaterOrEqual() {
	emitln("setge\t%%al");
	emitln("neg\t%%al\t#change 1 to -1"); //TRUE is -1 in this language
	emitln("movsx\t%%al,%%eax\t#extend al to eax");
}

//Set eax if compare was <
void setLess() {
	emitln("setl\t%%al");
	emitln("neg\t%%al\t#change 1 to -1"); //TRUE is -1 in this language
	emitln("movsx\t%%al,%%eax\t#extend al to eax");
}

//Set eax if compare was <=
void setLessOrEqual() {
	emitln("setle\t%%al");
	emitln("neg\t%%al\t#change 1 to -1"); //TRUE is -1 in this language
	emitln("movsx\t%%al,%%eax\t#extend al to eax");
}

//Add top of stack to primary register
void popAdd() {
	emitln("pop\t%%ebx"); //pop top of stack to ebx
	emitln("add\t%%ebx,%%eax"); //add ebx to eax
}

//Subtract primary register from top of stack
void popSub() {
	emitln("pop\t%%ebx"); //pop first operand to ebx
	emitln("sub\t%%ebx,%%eax"); //subtract ebx from eax
	emitln("neg\t%%eax"); //negate eax to fix sign error
}

//Multiply top of stack by primary register
void popMul() {
	emitln("pop\t%%ebx"); //pop top of stack to ebx
	emitln("mul\t%%ebx"); //multiply eax by ebx
}

//Divide top of stack by primary register
void popDiv() {
	emitln("mov\t%%eax,%%ebx"); //move second factor to ebx
	emitln("pop\t%%eax"); //pop first factor into eax
	emitln("xor\t%%edx,%%edx"); //clear high word of dividend
	emitln("div\t%%ebx"); //divide first factor by second factor
}

//Branch unconditional
void branch(char *label) {
	emitln("jmp\t%s", label);
}

//Branch false
void branchFalse(char *label) {
	emitln("test\t%%eax,%%eax");
	emitln("je\t%s", label);
}

void header() {
#ifdef RELEASE
	asmheader();
#else
	emitln("HEADER");
#endif
}

void prolog() {
#ifdef RELEASE
	asmprolog();
#else
	emitln("PROLOG");
#endif
}

void epilog() {
#ifdef RELEASE
	asmepilog();
#else
	emitln("EPILOG");
#endif
}

//
#pragma mark End of Code Generation Routines
//

//Allocate storage for a variable
void alloc(char *name) {
	if (inTable(name)) {
		fail("Duplicate variable name: %s", name);
	}
	addEntry(name, 'v');
	printf("%s:\t", name);
	if ('=' == look) {
		match('=');
		printf(".long ");
		//Allocate a 4-byte variable with the specified value
		if ('-' == look) {
			printf("-");
			match('-');
		}
		printf("%d\n", getNum());
	}
	else {
		//Allocate uninitialized space
		printf(".space 4\n");
	}
}

void expression();

//Recognize and Translate a Relation "Equals"
void equals() {
	match('=');
	expression();
	popCompare();
	setEqual();
}

//Recognize and Translate a Relation "Not Equals"
void notEquals() {
	match('>');
	expression();
	popCompare();
	setNEqual();
}

//Recognize and Translate a Relation "Less Than or Equal"
void lessOrEqual() {
	match('=');
	expression();
	popCompare();
	setLessOrEqual();
}

//Recognize and Translate a Relation "Less Than"
void less() {
	match('<');
	switch (look) {
		case '=':
			lessOrEqual();
			break;
		case '>':
			notEquals();
			break;
		default:
			expression();
			popCompare();
			setLess();
			break;
	}
}

//Recognize and Translate a Relation "Greater Than"
void greater() {
	match('>');
	if ('=' == look) {
		match('=');
		expression();
		popCompare();
		setGreaterOrEqual();
	}
	else {
		expression();
		popCompare();
		setGreater();
	}
}

//Recognize and Translate a Relation "Greater Than or Equal"
void greaterOrEqual() {
	match('=');
	expression();
	popCompare();
	setGreaterOrEqual();
}

//Parse and translate a Relation
void relation() {
	expression();
	if (isRelop(look)) {
		push();
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
	}
}

//Parse and translate a Boolean factor with leading NOT
void notFactor() {
	if ('!'==look) {
		match('!');
		relation();
		notIt();
	}
	else {
		relation();
	}
}

//Parse and translate a Boolean term
void boolTerm() {
	notFactor();
	while ('&'==look) {
		push();
		match('&');
		notFactor();
		popAnd();
	}
}

//Recognize and translate a Boolean OR
void boolOr() {
	match('|');
	boolTerm();
	popOr();
}

//Recognize and translate a Boolean XOR
void boolXor() {
	match('~');
	boolTerm();
	popXor();
}

//Parse and translate a Boolean expression
void boolExpression() {
	boolTerm();
	while (isOrop(look)) {
		push();
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

//Parse and translate a math factor
void factor() {
	if ('(' == look) {
		match('(');
		boolExpression();
		match(')');
	}
	else if (isAlpha(look)) {
		loadVar(getName());
	}
	else {
		loadConst(getNum());
	}
}

//Parse and translate a negative factor
void negFactor() {
	match('-');
	if (isDigit(look)) {
		loadConst(-getNum());
	}
	else {
		factor();
		negate();
	}
}

//Parse and translate a leading factor
void firstFactor() {
	switch (look) {
		case '+':
			match('+');
			factor();
			break;
		case '-':
			negFactor();
			break;
		default:
			factor();
			break;
	}
}

//Recognize and translate a multiply
void multiply() {
	match('*');
	factor();
	popMul();
}

//Recognize and translate a divide
void divide() {
	match('/');
	factor();
	popDiv();
}

//Process a Data Declaration
void decl() {
	alloc(getName());
	while (',' == look) {
		match(',');
		alloc(getName());
	}
}

//Parse and translage Global Declarations
void topDecls() {
	scan();
	while ('b' != token) {
		switch (token) {
			case 'v':
				decl();
				break;
			default:
				fail("Unrecognized keyword: %s", value);
				break;
		}
		scan();
	}
}

//Common code used by term() and firstTerm()
void term1() {
	while (isMulop(look)) {
		push();
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

//Parse and translate a math term
void term() {
	factor();
	term1();
}

//Parse and translate a leading term
void firstTerm() {
	firstFactor();
	term1();
}

//Recognize and translate an add
void add() {
	match('+');
	term();
	popAdd();
}

//Recognize and translate a subtract
void subtract() {
	match('-');
	term();
	popSub();
}

//Parse and translate an expression
void expression() {
	newLine();
	firstTerm();
	while (isAddop(look)) {
		push();
		switch (look) {
			case '+':
				add();
				break;
			case '-':
				subtract();
				break;
		}
		newLine();
	}
}

//Parse and translate an Assignment statement
void assignment() {
	char name[tokenbuflen];
	strlcpy(name, value, tokenbuflen);
	match('=');
	boolExpression();
	store(name);
}

void block();

//Process a Read statement
void doRead() {
	match('(');
	getName();
	readVar();
	while (',' == look) {
		match(',');
		getName();
		readVar();
	}
	match(')');
}

void doWrite() {
	match('(');
	expression();
	writeVar();
	while (',' == look) {
		match(',');
		expression();
		writeVar();
	}
	match(')');
}
	

//Recognize and translate an IF construct
void doIf() {
	char label1[labelbufsize];
	char label2[labelbufsize];
	
	newLabel(label1);
	strlcpy(label2, label1, labelbufsize);
	emitln("#IF");
	boolExpression();
	branchFalse(label1);
	block();
	if ('l' == token) {
		newLabel(label2);
		branch(label2);
		postLabel(label1, "#ELSE");
		block();
	}
	postLabel(label2, "#ENDIF");
	matchString("ENDIF");
}

//Recognize and translate a while statement
void doWhile() {
	char label1[labelbufsize];
	char label2[labelbufsize];
	
	newLabel(label1);
	newLabel(label2);
	postLabel(label1, "#WHILE");
	boolExpression();
	branchFalse(label2);
	block();
	matchString("ENDWHILE");
	branch(label1);
	postLabel(label2, "#ENDWHILE");
}

//Parse and translate a Block of statements
void block() {
	scan();
	while ('e' != token && 'l' != token) {
		switch (token) {
			case 'i':
				doIf();
				break;
			case 'w':
				doWhile();
				break;
			case 'R':
				doRead();
				break;
			case 'W':
				doWrite();
				break;
			default:
				assignment();
				break;
		}
		scan();
	}
}

//Parse and translate a Main Program
void doMain() {
	matchString("BEGIN");
	prolog();
	block();
	matchString("END");
	epilog();
}

//Parse and translate a Program
void prog() {
	matchString("PROGRAM");
	header();
	topDecls();
	doMain();
	match('.');
}

//Initialize
void init() {
	int i;
	for (i=0; i<maxSymbols; i++) {
		symbolTable[i] = NULL;
		symbolType[i] = ' ';
	}
	getChar();
	scan();
}

int main (int argc, const char * argv[]) {
    init();
	
	prog();
	if (!isEOL(look)) {
		fail("Unexpected data after '.'");
	}
	
    return 0;
}
