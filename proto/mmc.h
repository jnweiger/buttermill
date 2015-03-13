/*
 * 	Doku, siehe http://www.mikrocontroller.net/articles/AVR_FAT32
 *  Neuste Version: http://www.mikrocontroller.net/svnbrowser/avr-fat32/
 *	Autor: Daniel R.
 */


#ifndef _MMC_H
  #define _MMC_H  

  #define MMC_Write 	        PORTB	//Port an der die MMC/SD-Karte angeschlossen ist also des SPI
  #define MMC_Read 		PINB
  #define MMC_Direction_REG     DDRB

  // da der SPI port ein master gesteuerter port ist, beziehen sich die bezeichnungen auf den master, also den controller.
  // MISO -> Master In Slave Out, also der eingang am MC und der ausgang der karte.
  // MOSI -> Master Out Slave In, also der ausgang am MC und der eingang der karte.
  // der SS pin des SPI ports wird nicht benutzt! also da nich die karte anschließen, sondern an MMC_Chip_Select !

  #if defined (__AVR_ATmega169__)
        #define SPI_MISO                        3 //cf  //Port Pin an dem Data Output der MMC/SD-Karte angeschlossen ist
        #define SPI_MOSI                        2 // cf  //Port Pin an dem Data Input der MMC/SD-Karte angeschlossen ist
        #define SPI_Clock                       1  //Port Pin an dem die Clock der MMC/SD-Karte angeschlossen ist (clk)
        #define MMC_Chip_Select                 5  //Port Pin an dem Chip Select der MMC/SD-Karte angeschlossen ist
        #define SPI_SS                          0  //Nicht Benutz muß aber definiert werden
  #endif


  //Prototypes
  extern  unsigned char 		mmc_init(void);		// initialisiert die pins und geschwindigkeit
  extern unsigned char 		mmc_read_sector (unsigned long addr,unsigned char *Buffer);	// liest einen sektor mit adresse addr auf Buffer
  extern unsigned char 		mmc_write_sector (unsigned long addr,unsigned char *Buffer);	// schreibt einen sektor mit der adresse addr aus dem puffer Buffer, auf die karte
  extern unsigned char		mmc_multi_block_stop_read (void);						// stop kommando fuer multiblock read operation
  extern unsigned char		mmc_multi_block_stop_write (void);						// stop kommando fuer multiblock write operation
  extern unsigned char 		mmc_multi_block_start_read (unsigned long int addr);	// hier wird das kommando um mehrer bloecke am stueck zu lesen gesendet. ab addr
  extern void 					mmc_multi_block_read_sector (unsigned char *Buffer);	// hiermit werden nacheinander die bloecke gelesen
  extern unsigned char 		mmc_multi_block_start_write (unsigned long int addr);	// hier wird das kommando um mehrere bloecke am stueck zu schreiben gesendet, ab addr
  extern unsigned char		mmc_multi_block_write_sector (unsigned char *Buffer);	// hiermit werden nacheinander die bloecke geschrieben

  //set MMC_Chip_Select to high (MMC/SD-Karte Inaktiv)
  #define MMC_Disable() MMC_Write|= (1<<MMC_Chip_Select);

  //set MMC_Chip_Select to low (MMC/SD-Karte Aktiv)
  #define MMC_Enable() MMC_Write&=~(1<<MMC_Chip_Select);

  // einen takt nichts machen
  #define nop()  __asm__ __volatile__ ("nop" ::)

	#if(MMC_STATUS_INFO==TRUE)
		#define MMC_OK			0
		#define MMC_ERROR_1		1
		#define MMC_ERROR_2		2
		#define MMC_NO_CARD		5
		#define MMC_WP			6

		#define MMC_NOT_PRESENT		7
		#define MMC_PROTECTED		8

		// > defines are based on work by Roland Riegel
		#define configure_pin_protected() 		DDRC &= ~(1 << DDC2)
		#define configure_pin_present() 		DDRA &= ~(1 << DDA1)
		#define get_pin_protected() 			((PINC >> PC2) & 0x01)
		#define get_pin_present() 				((PINA >> PA1) & 0x01)

		#if(MMC_STATUS_INFO == TRUE)
		  extern unsigned char mmc_present(void);
		#endif

		#if(MMC_STATUS_INFO == TRUE && MMC_WRITE == TRUE)
		  extern unsigned char mmc_protected(void);
		#endif
	#endif


#endif
