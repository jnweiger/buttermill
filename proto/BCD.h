//***************************************************************************
//
//  File........: BCD.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: AVR-GCC 3.3.1; avr-libc 1.0
//
//  Description.: AVR Butterfly BCD conversion algorithms
//
//  Revisions...: 1.1
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20030116 - 1.0  - Created                                       - KS
//  20050610 - 1.1  - Added int2BCD5 and extended CHAR2BCD3			- NAL
//
//***************************************************************************

// Function declarations
char CHAR2BCD2(char input);
unsigned int CHAR2BCD3(char input);
unsigned long int2BCD5(unsigned int input);
