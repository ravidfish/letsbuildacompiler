/*
 *  machoasm.h
 *  Lets's Build a Compiler
 *  This support code adds header and footer to create an executable binary on OS X
 *  It exits with the final eax value in the program's exit code
 *  Use 'gcc file.s -o file' to assemble/link
 *  Use 'echo $?' after running the program to view the exit code from the command line
 *
 *  Created by Steve Nicholson on 12/4/09.
 *  Copyright 2009 Harmony Ridge Software. All rights reserved.
 *
 */

void asmheader();
void asmfooter();