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
//	20060801 - 1.2	- Optimised andded convertBCD masterr function.	- NAL
//
//***************************************************************************

const unsigned int unit[]={10000,1000,100,10};
#define UNIT_TEN_THOU	0
#define UNIT_THOU		1
#define UNIT_HUNDRED	2
#define UNIT_TEN		3


/*****************************************************************************
*
*   Function name : convertBCD
*
*   Returns :       Binary coded decimal value of the input 
*
*   Parameters :    input: Value between (0-99999) to be encoded into BCD 
*					digit: Highest digit of the conversion (from defines above)
*
*   Purpose :       Convert a number into a BCD encoded character.
*
*****************************************************************************/
unsigned long convertBCD( char digit, unsigned int input)
{
	unsigned long high = 0;
	
	char i;
	
	for (i=digit;i<=UNIT_TEN;i++){
		while (input >= unit[(int)i])            
		{
			high++;
			input -= unit[(int)i];
		}
		
		high <<= 4;
	}
	
    return  high | input;        // Add ones and return answer
}


/*****************************************************************************
*
*   Function name : CHAR2BCD2
*
*   Returns :       Binary coded decimal value of the input (2 digits)
*
*   Parameters :    Value between (0-99) to be encoded into BCD 
*
*   Purpose :       Convert a character into a BCD encoded character.
*                   The input must be in the range 0 to 99.
*                   The result is byte where the high and low nibbles
*                   contain the tens and ones of the input.
*
*****************************************************************************/
char CHAR2BCD2(char input)
{
	return (char) convertBCD(UNIT_TEN,(unsigned int) input);
}


/*****************************************************************************
*
*   Function name : CHAR2BCD3
*
*   Returns :       Binary coded decimal value of the input (3 digits)
*
*   Parameters :    Value between (0-999) to be encoded into BCD 
*
*   Purpose :       Convert a character into a BCD encoded character.
*                   The input must be in the range 0 to 1024.
*                   The result is an integer where the three lowest nibbles
*                   contain the ones, tens and hundreds of the input.
*
*****************************************************************************/
unsigned int CHAR2BCD3(unsigned int input)
{
		return (unsigned int) convertBCD(UNIT_HUNDRED, input);
}


/*****************************************************************************
*
*   Function name : INT2BCD6
*
*   Returns :       Binary coded decimal value of the input (5 digits)
*
*   Parameters :    Value between (0-99999) to be encoded into BCD 
*
*   Purpose :       Convert a character into a BCD encoded character.
*                   The input must be in the range 0 to 65535
*                   The result is an integer where the three lowest nibbles
*                   contain the ones, tens and hundreds of the input.
*
*****************************************************************************/
unsigned long int2BCD5(unsigned int input)
{
	return (unsigned long) convertBCD(UNIT_TEN_THOU,input);
}


