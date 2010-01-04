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

//Reading http://zathras.de/angelweb/blog-intel-assembler-on-mac-os-x.htm helped me
//get started in generating the skeleton code

void asmheader() {
	printf("#assemble/link with 'gcc file.s -o file\n");
	printf("	.text\n");
	printf(".globl _main\n");
	printf("	.data\n");
	printf("IOBUF: .space %d\n", IOBUFSIZE);
}

void asmprolog() {
	printf("	.text\n");

	printf("\n#convert eax to ascii in IOBUF and append newline\n");
	printf("# RETURN: eax contains length of string (including newline)\n");
	printf("_convertToAscii:\n");
	printf("	push	%%ebp\n");
	printf("	mov	%%esp, %%ebp\n");
	printf("	\n");
	printf("	#set ebx if eax is negative\n");
	printf("	mov	$10, %%ecx	#Move 10 into ecx to use for dividing later\n");
	printf("	mov $0, %%ebx\n");
	printf("	cmp	$0, %%eax\n");
	printf("	jge	__cta0\n");
	printf("	mov $1, %%ebx\n");
	printf("	neg	%%eax\n");
	printf("__cta0:\n");
	printf("	mov	$0,%%edx\n");
	printf("	div	%%ecx	#divide edx:eax by 10. edx<-remainder, eax<-quotient\n");
	printf("	add	$0x30,%%edx	#convert remainder to ASCII\n");
	printf("	push	%%edx #push digit onto stack\n");
	printf("	cmp	$0, %%eax\n");
	printf("	jg	__cta0\n");
	printf("	\n");
	printf("	#value is converted and placed on stack with most significant digit on top\n");
	printf("	lea	IOBUF, %%edi\n");
	printf("	mov	$0, %%ecx\n");
	printf("	test	%%ebx,%%ebx\n");
	printf("	jz	__cta1\n");
	printf("	movl	$0x2D, (%%edi,%%ecx) #'-' character\n");
	printf("	inc	%%ecx\n");
	printf("	\n");
	printf("__cta1: #move encoded value from stack to buffer\n");
	printf("	pop	%%eax\n");
	printf("	mov	%%eax,(%%edi,%%ecx)\n");
	printf("	inc	%%ecx\n");
	printf("	cmp	%%ebp,%%esp\n");
	printf("	jne	__cta1\n");
	printf("	\n");
	printf("	movl	$0x0A, (%%edi,%%ecx) #newline\n");
	printf("	inc	%%ecx\n");
	printf("	mov	%%ecx,%%eax\n");
	printf("	leave\n");
	printf("	ret\n\n");
	
	printf("# Convert ASCII value in IOBUF to decimal value\n");
	printf("#  INPUT: eax = number of characters in IOBUF\n");
	printf("#  RETURN: eax = converted value\n");
	printf("#  locals:\n");
	printf("#     -4(%%ebp): number of characters in IOBUF\n");
	printf("#     -8(%%ebp): negative flag (non-zero if value is negative)\n");
	printf("#    -12(%%ebp): decimal value 10 for multiplying eax\n");
	printf("_convertFromAscii:\n");
	printf("    push    %%ebp\n");
	printf("    mov %%esp, %%ebp\n");
	printf("    sub $24, %%esp\n");
	printf("    mov %%eax, -4(%%ebp)   #move number of characters to -4(%%ebp)\n");
	printf("    mov $0, %%eax    #initialize return value to zero\n");
	printf("    mov %%eax, -8(%%ebp)   #clear negative flag\n");
	printf("    cmpl    $0, -4(%%ebp)\n");
	printf("    jle __cfa_exit #If zero characters read, stop now\n");
	printf("    \n");
	printf("    movl    $10, -12(%%ebp)\n");
	printf("    lea IOBUF, %%esi\n");
	printf("    mov $0, %%ecx    #initialize index register\n");
	printf("    mov (%%esi, %%ecx), %%bl\n");
	printf("    cmp $0x2D, %%bl	#test for minus sign\n");
	printf("    jne __cfa_readLoop\n");
	printf("    movl    $1, -8(%%ebp)	#set negative flag\n");
	printf("    inc %%ecx\n");
	printf("__cfa_readLoop:\n");
	printf("    cmp -4(%%ebp), %%ecx\n");
	printf("    jge __cfa_checkForNegative  #reached end of buffer\n");
	printf("    mov (%%esi, %%ecx), %%bl  #move next byte to bl\n");
	printf("    inc %%ecx    #advance index\n");
	printf("    cmp $0x2C, %%bl  #compare to ascii comma\n");
	printf("    je  __cfa_readLoop  #ignore commas\n");
	printf("    sub $0x30, %%bl\n");
	printf("    jl  __cfa_checkForNegative  #if ascii value less than 0x30 (ascii 0), we are finished\n");
	printf("    cmp $9, %%bl\n");
	printf("    jg  __cfa_checkForNegative  #if value>9, we are finished\n");
	printf("    mull -12(%%ebp)  #multiply eax by 10\n");
	printf("    movsx %%bl, %%ebx\n");
	printf("    add %%ebx, %%eax  #add new digit to eax\n");
	printf("    jmp __cfa_readLoop\n");
	printf("    \n");
	printf("__cfa_checkForNegative:\n");
	printf("    cmpl    $0, -8(%%ebp)\n");
	printf("    je  __cfa_exit\n");
	printf("    neg %%eax\n");
	printf("    \n");
	printf("__cfa_exit:\n");
	printf("    leave\n");
	printf("    ret\n\n");	
	
	printf("# Write IOBUF to stdout\n");
	printf("#  INPUT: eax = number of characters to write\n");
	printf("#  RETURN: eax = number of characters written\n");
	printf("_writeIobuf:\n");
	printf("	push	%%ebp\n");
	printf("	mov	%%esp, %%ebp\n");
	printf("	push	%%eax	#length of string to write\n");
	printf("	lea	IOBUF,%%eax\n");
	printf("	push	%%eax	#buffer address\n");
	printf("	pushl	$%d	#stdout\n", stdout_num);
	printf("	mov	$%d, %%eax	#SYS_write\n", SYS_write);
	printf("	push	%%eax\n");
	printf("	int	$0x80\n");
	printf("	leave\n");
	printf("	ret\n\n");

	printf("# Read stdin to IOBUF\n");
	printf("#  RETURN: eax = number of characters received\n");
	printf("_readIobuf:\n");
	printf("	push	%%ebp\n");
	printf("	mov	%%esp, %%ebp\n");
	printf("	pushl	$%d	#buffer size\n", IOBUFSIZE);
	printf("	lea	IOBUF,%%eax\n");
	printf("	push	%%eax	#buffer address\n");
	printf("	pushl	$%d	#stdin\n", stdin_num);
	printf("	mov	$%d, %%eax	#SYS_write\n", SYS_read);
	printf("	push	%%eax\n");
	printf("	int	$0x80\n");
	printf("	leave\n");
	printf("	ret\n\n");
	
	printf("_main:\n");
	printf("	push	%%ebp\n");
	printf("	mov	%%esp, %%ebp\n");
	printf("	mov	$0, %%eax\n");

	printf("# program starts here\n");
}

void asmepilog() {
	printf("# contents of %%eax will be the exit code\n");
	printf("	leave\n");
	printf("	ret\n");
	printf("	.subsections_via_symbols\n");
}