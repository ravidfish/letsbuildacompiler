#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "asmheader.h"

#define errbufsize 1024
#define labelbufsize 10
#define CR 0x0D
#define LF 0x0A

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

//Skip CRLF
void fin() {
	while (CR == look || LF == look)
		getChar();
}

//Recognize an Addop
int isAddop(char c) {
	return '+'==c || '-'==c;
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

//Recognize and translate an "Other"
void other() {
	char outstring[2];
	outstring[0] = getName();
	outstring[1] = 0x00;
	emitln(outstring);
}

void block(char *exitLabel);

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
	emitln("sete\t%%al");
	emitln("movsx\t%%al,%%eax\t#extend al to eax");
}

//Recognize and translate Relational not equals
void notEquals() {
	match('#');
	expression();
	emitln("pop\t%%ebx");
	emitln("cmp\t%%eax,%%ebx");
	emitln("setne\t%%al");
	emitln("movsx\t%%al,%%eax\t#extend al to eax");
	
}

//Recognize and translate Relational less than
void less() {
	match('<');
	expression();
	emitln("pop\t%%ebx");
	emitln("cmp\t%%eax,%%ebx");
	emitln("setl\t%%al"); //Set al if ebx<eax
	emitln("movsx\t%%al,%%eax\t#extend al to eax");
}

//Recognize and translate Relational greater than
void greater() {
	match('>');
	expression();
	emitln("pop\t%%ebx");
	emitln("cmp\t%%eax,%%ebx");
	emitln("setg\t%%al"); //Set al if ebx>eax
	emitln("movsx\t%%al,%%eax\t#extend al to eax");
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
		emitln("test\t%%eax,%%eax");
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

//Recognize and Translate an assignment statement
void assignment() {
	char name;
	name = getName();
	match('=');
	boolExpression();
	emitln("mov\t%%eax,%c", name);
}

//Recognize and translate a break statement
void doBreak(char *exitLabel) {
	match('b');
	if (NULL == exitLabel || 0 == strlen(exitLabel))
		fail("No loop to break from");
	else
		emitln("jmp\t%s\t#BREAK to %s", exitLabel, exitLabel);
}

//Recognize and translate IF construct
void doIf(char *exitLabel) {
	char label1[labelbufsize];
	char label2[labelbufsize];

	match('i');
	newLabel(label1);
	strlcpy(label2, label1, labelbufsize);
	emitln("#IF");
	boolExpression();
	emitln("je\t%s\t", label1);
	block(exitLabel);
	if ('l' == look) {
		match('l');
		newLabel(label2);
		emitln("jmp\t%s", label2);
		postLabel(label1, "#ELSE");
		block(exitLabel);
	}
	match('e');
	postLabel(label2, "#ENDIF");
}

//Recognize and translate a while statement
void doWhile() {
	char label1[labelbufsize];
	char label2[labelbufsize];
	
	match('w');
	newLabel(label1);
	newLabel(label2);
	postLabel(label1, "#WHILE");
	boolExpression();
	emitln("je\t%s", label2);
	block(label2);
	match('e');
	emitln("jmp\t%s", label1);
	postLabel(label2, "#ENDWHILE");
}
	
//Recognize and translate a loop statement
void doLoop() {
	char label1[labelbufsize];
	char label2[labelbufsize];
	
	match('p');
	newLabel(label1);
	newLabel(label2);
	postLabel(label1, "#loop");
	block(label2);
	match('e');
	emitln("jmp\t%s\t", label1);
	postLabel(label2, "#ENDloop");
}

//Recognize and translate a repeat statement
void doRepeat() {
	char label1[labelbufsize];
	char label2[labelbufsize];
	
	match('r');
	newLabel(label1);
	newLabel(label2);
	postLabel(label1, "#REPEAT");
	block(label2);
	match('u');
	boolExpression();
	emitln("je\t%s", label1);
	postLabel(label2, "#ENDREPEAT");
}

//Parse and translate a for statement
//FOR <ident> = <expr1> TO <expr2> <block> ENDFOR
void doFor() {
	char label1[labelbufsize];
	char label2[labelbufsize];
	char label3[labelbufsize];
	char name;
	
	match('f');
	newLabel(label1);
	newLabel(label2);
	newLabel(label3);
	name = getName(); // <ident>
	match('=');
	emitln("#FOR");
	expression(); //Move <expr1> into eax
	emitln("mov\t%%eax,%c", name); //Move eax into <ident>
	expression(); //Move <expr2> into eax
	emitln("push\t%%eax\t#Put upper limit on stack");
	emitln("jmp\t%s", label2); //Jump to test with <expr2> on stack and <expr1> in eax
	postLabel(label1, "#beginning of FOR block");
	block(label3);
	match('e');
	emitln("leal\t%c,%%eax", name);
	emitln("incl\t(%%eax)");
	emitln("mov\t(%%eax),%%eax");
	postLabel(label2, "#test at ENDFOR");
	emitln("cmp\t(%%esp),%%eax");
	emitln("jle\t%s", label1);
	postLabel(label3, "#break address for FOR loop");
	emitln("addl\t$4,%%esp #Get upper limit off of stack");
	emitln("#ENDFOR");
}

//Recognize and translate a do statement
//DO <expr> <block> ENDDO
//performs <block> <expr> times
void doDo() {
	char label1[labelbufsize];
	char label2[labelbufsize];
	
	match('d');
	newLabel(label1);
	newLabel(label2);
	emitln("#DO");
	expression();
	emitln("mov\t%%eax, %%ecx");
	emitln("jexcz\t%s", label2);
	postLabel(label1, "#beginning of DO block");
	emitln("push\t%%ecx");
	block(label2);
	match('e');
	emitln("pop\t%%ecx");
	emitln("loop\t%s", label1);
	emitln("subl\t$4,%%esp\t#Increase stack in case of Break");
	postLabel(label2, "#ENDDO");
	emitln("addl\t$4,%%esp\t#Get ecx off stack in case of break");
}

//Recognize and translate a statement block
void block(char *exitLabel) {
	while ('e' != look && 'l' != look && 'u' != look) {
		fin(); //Skip newlines
		switch (look) {
			case 'b':
				doBreak(exitLabel);
				break;
			case 'd':
				doDo();
				break;
			case 'f':
				doFor();
				break;
			case 'i':
				doIf(exitLabel);
				break;
			case 'p':
				doLoop();
				break;
			case 'r':
				doRepeat();
				break;
			case 'w':
				doWhile();
				break;
			default:
				assignment();
				break;
		}
		fin(); //Skip newlines
	}
}

void doProgram() {
	block(NULL);
	if ('e' != look)
		expected("End");
#ifndef RELEASE
	emitln("END");
#endif
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
