/*
 *  machoasm.c
 *  Lets's Build a Compiler
 *  This support code creates an executable binary on OS X
 *  It exits with the final ax value in the program's exit code
 *  Use 'echo $?' after running the program to view the exit code from the command line
 *
 *  Created by Steve Nicholson on 12/4/09.
 *  Copyright 2009 Harmony Ridge Software. All rights reserved.
 *
 */

#include <stdio.h>
#include "asmheader.h"

//This code is from http://zathras.de/angelweb/blog-intel-assembler-on-mac-os-x.htm

void asmheader() {
	printf("#assemble/link with 'gcc file.s -o file\n");
	printf("	.text\n");
	printf(".globl _main\n");
	printf("_main:\n");
	printf("	pushl	%%ebp\n");
}

void asmfooter() {
	printf("# contents of %%eax will be the exit code\n");
	printf("	movl	%%esp, %%ebp\n");
	printf("	subl	$8, %%esp\n");
	printf("	leave\n");
	printf("	ret\n");
	printf("	.subsections_via_symbols\n");
}