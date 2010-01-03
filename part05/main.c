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

//Recognize and translate an "Other"
void other() {
	char outstring[2];
	outstring[0] = getName();
	outstring[1] = 0x00;
	emitln(outstring);
}

void block(char *exitLabel);

//Parse and translate Boolean condition
//Dummy version
void condition() {
	emitln("<condition>");
}

//Parse and translate an expression
//Dummy version
void expression() {
	emitln("<expression> #result is in %%eax");
}

//Recognize and translate a break statement
void doBreak(char *exitLabel) {
	match('b');
	if (NULL == exitLabel || 0 == strlen(exitLabel))
		fail("No loop to break from");
	else
		emitln("JMP\t%s\t#BREAK to %s", exitLabel, exitLabel);
}

//Recognize and translate IF construct
void doIf(char *exitLabel) {
	char label1[labelbufsize];
	char label2[labelbufsize];

	match('i');
	newLabel(label1);
	strlcpy(label2, label1, labelbufsize);
	emitln("#IF");
	condition();
	emitln("JEQ\t%s\t", label1);
	block(exitLabel);
	if ('l' == look) {
		match('l');
		newLabel(label2);
		emitln("JMP\t%s", label2);
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
	condition();
	emitln("JEQ\t%s", label2);
	block(label2);
	match('e');
	emitln("JMP\t%s", label1);
	postLabel(label2, "#ENDWHILE");
}
	
//Recognize and translate a loop statement
void doLoop() {
	char label1[labelbufsize];
	char label2[labelbufsize];
	
	match('p');
	newLabel(label1);
	newLabel(label2);
	postLabel(label1, "#LOOP");
	block(label2);
	match('e');
	emitln("JMP\t%s\t", label1);
	postLabel(label2, "#ENDLOOP");
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
	condition();
	emitln("JEQ\t%s", label1);
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
	emitln("MOV\t%%eax,%c", name); //Move eax into <ident>
	expression(); //Move <expr2> into eax
	emitln("PUSH\t%%eax\t#Put upper limit on stack");
	emitln("JMP\t%s", label2); //Jump to test with <expr2> on stack and <expr1> in eax
	postLabel(label1, "#beginning of FOR block");
	block(label3);
	match('e');
	emitln("LEAL\t%c,%%eax", name);
	emitln("INCL\t(%%eax)");
	emitln("MOV\t(%%eax),%%eax");
	postLabel(label2, "#test at ENDFOR");
	emitln("CMP\t(%%esp),%%eax");
	emitln("JLE\t%s", label1);
	postLabel(label3, "#break address for FOR loop");
	emitln("ADDL\t$4,%%esp #Get upper limit off of stack");
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
	emitln("MOV\t%%eax, %%ecx");
	emitln("JECXZ\t%s", label2);
	postLabel(label1, "#beginning of DO block");
	emitln("PUSH\t%%ecx");
	block(label2);
	match('e');
	emitln("POP\t%%ecx");
	emitln("LOOP\t%s", label1);
	emitln("SUBL\t$4,%%esp\t#Increase stack in case of Break");
	postLabel(label2, "#ENDDO");
	emitln("ADDL\t$4,%%esp\t#Get ecx off stack in case of break");
}

//Recognize and translate a statement block
void block(char *exitLabel) {
	while ('e' != look && 'l' != look && 'u' != look) {
		switch (look) {
			case 'b':
				doBreak(exitLabel);
				break;
			case 'd':
				doDo();
				break;
			case 'i':
				doIf(exitLabel);
				break;
			case 'f':
				doFor();
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
				other();
				break;
		}
	}
}

void doProgram() {
	block(NULL);
	if ('e' != look)
		expected("End");
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
