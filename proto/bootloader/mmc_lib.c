#include <avr/io.h>
#include "mmc_lib.h"
#include "fat16.h"

#define MMC_CMD0_RETRY	(unsigned char)16

static unsigned char cmd[6];

inline static void spi_send_byte(unsigned char data)
{
	SPDR=data;
	loop_until_bit_is_set(SPSR, SPIF); // wait for byte transmitted...
}

static unsigned char send_cmd(void)
{
	unsigned char i;
        unsigned char *buf;
	
	spi_send_byte(0xFF); //Dummy delay 8 clocks
	MMC_PORT &= ~(1<<MMC_CS);	//MMC Chip Select -> Low (activate)

// Manchmal mag AVR-GCC while() lieber als for() und erzeugt kürzeren Code ;)
// Scheinbar gerade bei sehr kurzen Schleifen.

/*
	for (i=0; i<6; i++)
	{//Send 6 Bytes
		spi_send_byte(cmd[i]);
	}
*/
 
        i=6;
        buf = cmd;
	while(i)
	{//Send 6 Bytes
 	 spi_send_byte(*buf++);
	 i--;
	}

	unsigned char result;
	
	for(i=0; i<128; i++)
	{//waiting for response (!0xff)
 	 spi_send_byte(0xFF);
		
		result = SPDR;
		
		if ((result & 0x80) == 0)
			break;
	}

	return(result); // TimeOut !?
}

unsigned char mmc_init(void)
{
	MMC_DDR &= ~(1<<SPI_MISO);	//MMC Data Out -> Input
	MMC_DDR |= 1<<SPI_CLK | 1<<SPI_MOSI | 1<<MMC_CS;	//MMC Chip Select -> Output
		
	SPCR = 1<<SPE | 1<<MSTR | SPI_INIT_CLOCK; //SPI Enable, SPI Master Mode
	SPSR = SPI_DOUBLE_SPEED << SPI2X;
	
	unsigned char i;
	

// Manchmal mag AVR-GCC while() lieber als for() und erzeugt kürzeren Code ;)
// Scheinbar gerade bei sehr kurzen Schleifen.
/*
	for (i=0; i<10; i++) { //Pulse 80+ clocks to reset MMC
 	 spi_send_byte(0xFF);
	}
*/

        i=10;
	while(i) { //Pulse 80+ clocks to reset MMC
 	 spi_send_byte(0xFF);
         i--;
	}

	unsigned char res;

	cmd[0] = 0x40 + MMC_GO_IDLE_STATE;
	cmd[1] = 0x00; cmd[2] = 0x00; cmd[3] = 0x00; cmd[4] = 0x00; cmd[5] = 0x95;
	
	for (i=0; i<MMC_CMD0_RETRY; i++)
	{
		res=send_cmd(); //store result of reset command, should be 0x01

		MMC_PORT |= 1<<MMC_CS; //MMC Chip Select -> High (deactivate);
      	 	spi_send_byte(0xFF);
		if (res == 0x01)
			break;
	}

	if(i==MMC_CMD0_RETRY) return(MMC_TIMEOUT);

	if (res != 0x01) //Response R1 from MMC (0x01: IDLE, The card is in idle state and running the initializing process.)
		return(MMC_INIT);
	
	cmd[0]=0x40 + MMC_SEND_OP_COND;
		
//May be this becomes an endless loop ?
//Counting i from 0 to 255 and then timeout
//was to SHORT for some of my cards !
	while(send_cmd() != 0) {
		MMC_PORT |= 1<<MMC_CS; //MMC Chip Select -> High (deactivate);
		spi_send_byte(0xFF);
	}
	
	return(MMC_OK);
}

static unsigned char wait_start_byte(void)
{
	unsigned char i;
	
	i=255;
	do
	{
 	 spi_send_byte(0xFF);
         if(SPDR == 0xFE) return MMC_OK;
	}while(--i);
	
	return MMC_NOSTARTBYTE;
}


unsigned char mmc_start_read_block(unsigned long adr)
{
/*
	adr <<= 9;
	
	cmd[0] = 0x40 + MMC_READ_SINGLE_BLOCK;
	cmd[1] = (adr & 0xFF000000) >> 0x18;
	cmd[2] = (adr & 0x00FF0000) >> 0x10;
	cmd[3] = (adr & 0x0000FF00) >> 0x08;
	cmd[4] = (adr & 0x000000FF);
*/	

	adr <<= 1;
	
	cmd[0] = 0x40 + MMC_READ_SINGLE_BLOCK;
	cmd[1] = (adr & 0x00FF0000) >> 0x10;
	cmd[2] = (adr & 0x0000FF00) >> 0x08;
	cmd[3] = (adr & 0x000000FF);
	cmd[4] = 0;

	SPCR = 1<<SPE | 1<<MSTR | SPI_READ_CLOCK; //SPI Enable, SPI Master Mode
	
	if (send_cmd() != 0x00) {
		MMC_PORT |= 1<<MMC_CS; //MMC Chip Select -> High (deactivate);
		return(MMC_CMDERROR); //wrong response!
	}
	

	if (wait_start_byte())
	{
		MMC_PORT |= 1<<MMC_CS; //MMC Chip Select -> High (deactivate);
		return MMC_NOSTARTBYTE;
	}
	
	return(MMC_OK);
}

void mmc_read_buffer(void)
{
 unsigned char *buf;
 unsigned short len;
 
 buf = fat_buf;
 len= 512;

	while (len)
	{
 	 spi_send_byte(0xFF);
 	 *buf++ = SPDR;
	 len--;
	}
}

void mmc_stop_read_block(void)
{
	//read 2 bytes CRC (not used);
	spi_send_byte(0xFF);
 	spi_send_byte(0xFF);
	MMC_PORT |= 1<<MMC_CS; //MMC Chip Select -> High (deactivate);
}
