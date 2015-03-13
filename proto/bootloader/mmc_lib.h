/*-------------------------------------------------
	 mmc_lib.h C-header file for AVRGCC
	------------------------------------
	Interface for MultiMediaCard via SPI
	
	Author:
		Stefan Seegel
		dahamm@gmx.net
		ICQ UIN 14964408
		
	Thanks to:
		Ulrich Radig for his code
		Simon Lehmayr for good ideas
		ape for timing improovment
		Andreas Schwarz for the great forum
		
	Version:
		2.0 / 10.05.2005
	
	History:
		2.0 / 10.05.2005:
			- splited read & write commands in start..., read/write & stop...
			- Improved protocol handling for more supported cards
			- Improved description in header file
			- Added different SPI speeds for initialization, read and write
			- Supports SD cards
			- Added macros for CID register access
	
		1.2 / 11.08.2004:
			- Changed "mmc_send_byte()" to inline
			- Changed parameter "count" of "mmc_read_sector()" to unsigned short (16 bit)
			
		1.1 / 29.07.2004:
			-Removed typedef struct for CSD
			-Added different init-clock rate
			
		1.0 / 28.07.2004:
			-First release
	
---------------------------------------------------*/

#if defined (__AVR_ATmega644__)
 #define SPCR	SPCR0
 #define SPIE	SPIE0
 #define SPE	SPE0	
 #define DORD	DORD0
 #define MSTR	MSTR0
 #define CPOL	CPOL0
 #define CPHA	CPHA0
 #define SPR1	SPR01
 #define SPR0	SPR00

 #define SPSR	SPSR0
 #define SPIF	SPIF0
 #define WCOL	WCOL0
 #define SPI2X	SPI2X0

 #define SPDR	SPDR0
#endif

//Port & Pin definitions. Be sure to use a pin of the same port as SPI for CS (Chip Select) !
//Settings below are recommended for a MEGA169
#define MMC_PORT PORTB
#define MMC_DDR DDRB

#define SPI_MISO	PB3		//DataOut of MMC 
#define SPI_MOSI	PB2		//DataIn of  MMC
#define SPI_CLK  	PB1		//Clock of MMC
#define MMC_CS		PB5		//ChipSelect of MMC
#define SPI_SS          PB0             //Not used but according to Daniel R. must be defined


//Clockrate while initialisation / reading / writing
#define SPI_INIT_CLOCK 1<<SPR1 | 1<<SPR0
#define SPI_READ_CLOCK 0<<SPR1 | 0<<SPR0
#define SPI_WRITE_CLOCK 1<<SPR1 | 0<<SPR0

#define SPI_DOUBLE_SPEED 0 //0: normal speed, 1: double speed

//MMC Commandos
#define MMC_GO_IDLE_STATE 0
#define MMC_SEND_OP_COND 1
#define MMC_SEND_CSD	9
#define MMC_SEND_CID 10
#define MMC_SET_BLOCKLEN 16
#define MMC_READ_SINGLE_BLOCK 17
#define MMC_WRITE_BLOCK 24

// Result Codes
#define MMC_OK 				0
#define MMC_INIT 1
#define MMC_NOSTARTBYTE	2
#define MMC_CMDERROR	3
#define MMC_TIMEOUT		4

extern unsigned char mmc_init(void);
/*			
*		Call mmc_init one time after a card has been connected to the µC's SPI bus!
*	
*		return values:
*			MMC_OK:				MMC initialized successfully
*			MMC_INIT:			Error while trying to reset MMC
*			MMC_TIMEOUT:	Error/Timeout while trying to initialize MMC
*/

//extern unsigned char mmc_start_read_block(unsigned long adr, unsigned char adrshift);
extern unsigned char mmc_start_read_block(unsigned long adr);
/*
*		mmc_start_read_sector initializes the reading of a sector
*
*		Parameters:
*			adr: specifies address to be read from
*			adrshift: the specified address is shifted left by <adrshift> bits
*								e.g. if you use a blocksize of 512 bytes you should set
*								adrshift to 9 in order to specify the blocknumber in adr.
*
*		Return values:
*			MMC_OK:						Command successful
*			MMC_CMDERROR:			Error while sending read command to mmc
*			MMC_NOSTARTBYTE:	No start byte received
*		
*		Example Code:
*			unsigned char mmc_buf[512];
*			mmc_init();	//Initializes MMC / SD Card
*			set_blocklen(512);	//Sets blocklen to 512 bytes (default)
*			mmc_start_read_block(1000, 9);	//start reading sector 1000
*			mmc_read_buffer(&mmc_buf[0], 100);
*			mmc_read_buffer(&mmc_buf[100], 412);
*			mmc_stop_read_block();
*
*		Notes:
*			After calling mmc_start_read_block data is read with
*			mmc_read_buffer. Make sure to exactly read the amount of bytes
*			of the current blocklen (default 512 bytes).  This has not to be
*			done in a single mmc_read_buffer function.
*/

extern void mmc_read_buffer(void);
/*
*		mmc_read_buffer reads data from mmc after calling mmc_start_read_block
*
*		Notes:
*			See mmc_start_read_buffer
*/

extern void mmc_stop_read_block(void);
/*-----------------------------------------------------------------------------
*		mmc_stop_read_block must be called after the amount of bytes set by
*			mmc_setblocklen	has been read
-----------------------------------------------------------------------------*/
