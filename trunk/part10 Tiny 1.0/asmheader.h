/*
 *  asmheader.h
 *  Lets's Build a Compiler
 *  This support code creates an executable binary on OS X
 *  It exits with the final ax value in the program's exit code
 *  Use 'echo $?' after running the program to view the exit code from the command line
 *
 *  Created by Steve Nicholson on 12/4/09.
 *  Copyright 2009 Harmony Ridge Software. All rights reserved.
 *
 */

#define IOBUFSIZE 256
#define SYS_read 3
#define SYS_write 4

#define stdin_num 0
#define stdout_num 1
#define stderr_num 2

void asmheader();
void asmprolog();
void asmepilog();