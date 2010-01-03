/*
 *  asmheader.c
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
	printf("#assemble/link with 'gcc file.s -o file'\n");
	printf("	.text\n");
	printf(".globl _main\n");
	printf("_main:\n");
	printf("	pushl	%%ebp\n");
	printf("	movl	%%esp, %%ebp\n");
}

void asmfooter() {
	printf("# contents of %%eax will be the exit code\n");
	printf("	leave\n");
	printf("	ret\n");
	printf("#Reserve space for all the one-character variable names\n");
	printf(".data\n");
	printf("A:\t.space 4\n");
	printf("B:\t.space 4\n");
	printf("C:\t.space 4\n");
	printf("D:\t.space 4\n");
	printf("E:\t.space 4\n");
	printf("F:\t.space 4\n");
	printf("G:\t.space 4\n");
	printf("H:\t.space 4\n");
	printf("I:\t.space 4\n");
	printf("J:\t.space 4\n");
	printf("K:\t.space 4\n");
	printf("L:\t.space 4\n");
	printf("M:\t.space 4\n");
	printf("N:\t.space 4\n");
	printf("O:\t.space 4\n");
	printf("P:\t.space 4\n");
	printf("Q:\t.space 4\n");
	printf("R:\t.space 4\n");
	printf("S:\t.space 4\n");
	printf("T:\t.space 4\n");
	printf("U:\t.space 4\n");
	printf("V:\t.space 4\n");
	printf("W:\t.space 4\n");
	printf("X:\t.space 4\n");
	printf("Y:\t.space 4\n");
	printf("Z:\t.space 4\n");
	printf("	.subsections_via_symbols\n");
}