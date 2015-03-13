/*
 * 	Doku, siehe http://www.mikrocontroller.net/articles/AVR_FAT32
 *  Neuste Version: http://www.mikrocontroller.net/svnbrowser/avr-fat32/
 *	Autor: Daniel R.
 */


#include <avr/io.h>

#include "config.h"
#include "mmc.h"
#include "led.h"

// **********************************************************************************************************************************
// funktionsprototypen von funktionen die nur in dieser datei benutzt werden !
static unsigned char 	mmc_read_byte(void);
static void 		mmc_write_byte(unsigned char);
static unsigned char 	mmc_write_command (unsigned char *);		// sendet ein kommando an die karte. wartet solange bis eine gueltige antwort kommt
static unsigned char 	mmc_identify();
// **********************************************************************************************************************************

#if (MMC_MULTI_BLOCK==TRUE && MMC_OVER_WRITE == FALSE)
	static unsigned char 	mmc_wait_ready (void);					// hilfsfunktion fuer multiblock operationen
#endif

unsigned char card_type=0;		// ist 0 bei mmc/sd und 1 bei sdhc

#if (MMC_SDHC_SUPPORT==TRUE)

// **********************************************************************************************************************************
// pfueft welcher karten typ vorliegt und initialisiert diesen. zur pruefung wird cmd8 herangezogen.
// mmc und sd karten kennen cmd8 nicht und antworten mit R1 mit illegalem commando.
// **********************************************************************************************************************************
static unsigned char mmc_identify(){

	unsigned short i;
	unsigned char response[5];
	unsigned char cmd[] = {0x40,0x00,0x00,0x00,0x00,0x95};		// cmd0

	// mit cmd0 wird unabhaengig ob mmc/sd oder sdhc die karte in den in_idle_state gebracht. das ist noetig um die initialisierung durchzufuehren
	i = 0;
	while(mmc_write_command (cmd) !=0x01){		// cmd0 (go_idle_state), response R1. R1 = 0x01 => in_idle_state, jetzt moeglich zu initialisieren
		if (i++>400){
			MMC_Disable();
			return FALSE; 						// in timeout gleaufen, liegt daran, dass die karte nicht korrekt auf cmd0 antwortet !
		}
	}

	// cmd8 kann laut spezifikation zur unterscheidung der unterschiedlichen kartentypen genommen werden. bei cmd8 und cmd0 muss der crc stimmen
	cmd[0]=0x40+8; cmd[3]=0x01; cmd[4]=0xAA; cmd[5]=0x87;
	response[0]=mmc_write_command (cmd);							// cmd8 (send_if_cond), response R7 (5 bytes), 1. byte von R7 ist R1
	for(i=1;i<5;i++) response[i]=mmc_read_byte();					// weitere 4 response bytes

	// prueft ob cmd8 gueltig ist. gueltig wenn, R1 gueltig, pruefpattern korrekt und karte unterstuetzt die vorgegebene spannung
	// nur sd karten mit standart > 2.0 kennen cmd8, wenn pruefung fehl schlaegt, liegt mmc oder sd, nicht aber sdhc vor!
	// es koennte auch noch sein, dass R1 richtig ist aber das pruefpattern falsch, dann sollte man eigentlich cmd8 wiederholen, mach ich aber nicht :)
	if( response[0]==0x01 && response[4]==0xAA && response[3]==0x01){

		cmd[0]=0x40+58; cmd[3]=0x00; cmd[4]=0x00;			// cmd58
		response[0]=mmc_write_command (cmd);				// cmd58, (read_OCR), response R3 (5 bytes)
		for(i=1;i<5;i++) response[i]=mmc_read_byte();		// weitere 4 response bytes (aber nicht wichtig)

		// wiederholt acmd41 solange bis die karte den status von in_idle_state=1 auf in_idle_state=0 wechselt (in R1), oder timeout
		i=1000;
		do{
			cmd[0]=0x40+55;	cmd[1]=0x00;					// cmd55 sagt der karte nur, dass jetzt ein spezifisches kommando kommt
			response[0]=mmc_write_command (cmd);			// cmd55, (app_cmd), response R1 (1 byte), pruefung hier egal
			cmd[0]=0x40+41;	cmd[1]=0x40;					// acmd41, mit bit(30) HCS = 1 => host kann high capacity (hc)
			response[0]=mmc_write_command (cmd);			// acmd41, (sd_send_op_cond) mit HCS bit gesetzt, response R1 (1 bytes)
		}while (response[0]!=0 && i--);						// karte muss auf in_idle_state=0, oder man lauft in timeout

		// karte ist mit initialisierung fertig, jetzt muss noch cmd58 folgen
		if(response[0]==0){
			cmd[0]=0x40+58;	cmd[1]=0x00;						// cmd58
			response[0]=mmc_write_command (cmd);				// cmd58, (read OCR), response R3 (5 bytes). 1.Byte von R3 ist R1
			for(i=1;i<5;i++) response[i]=mmc_read_byte();		// weitere 4 response bytes. wenn in OCR bit 30 gesetzt ist ist die karte hc faehig

			// bit 30 aus OCR gesetzt ? wenn ja ist karte sdhc
			if((response[1]&0x40)!=0)card_type=1;

			return TRUE;
		}
	  }


	// hier landet man wenn cmd8 nicht erkannt wird, also wenn sd karen standart < 2.0 oder mmc karte vorliegt !
	else{
		// cmd1 (send_op_cond), response R1. fuer mmc oder sd standart < 2.0
		i = 0;
		cmd[0] = 0x40+1;
		cmd[5] = 0x95;
		while( mmc_write_command (cmd) !=0){	// cmd1, (send_op_cond), response R1
			if (i++>400){
				MMC_Disable();
				return FALSE; 	// in timeout gelaufen, weil die karte nicht korrekt auf cmd1 antwortet
			}
		}
		return TRUE;
	}

	return FALSE;

}

#endif

// **********************************************************************************************************************************
// grundlegende initialisierung. pin initialisierung und vorbereiten der karte auf erkennung. es muss eine erkennung
// der karte durchgefuehrt werden, um sdhc von mmc/sd zu unterscheiden.
// **********************************************************************************************************************************
unsigned char mmc_init (void){
	unsigned char a;

	// mz: added status support
	#if(MMC_STATUS_INFO == TRUE)
		configure_pin_present();
		if (! mmc_present())
			return MMC_NOT_PRESENT;
	#endif

	#if(MMC_STATUS_INFO == TRUE && MMC_WRITE == TRUE)
		configure_pin_protected();
		if (mmc_protected())
			return MMC_PROTECTED;
	#endif

	// port configuration der mmc/sd/sdhc karte
	MMC_Direction_REG &=~(1<<SPI_MISO);         // miso auf input
	MMC_Direction_REG |= (1<<SPI_Clock);      	// clock auf output
	MMC_Direction_REG |= (1<<SPI_MOSI);         // mosi auf output
	MMC_Direction_REG |= (1<<MMC_Chip_Select);	// chip select auf output
	MMC_Direction_REG |= (1<<SPI_SS);			// wird nicht gebraucht, muss aber gesetzt werden, ss auf output
	MMC_Write |= (1<<MMC_Chip_Select);       	// chip selet auf high, karte anwaehlen

	for(a=0;a<254;a++) nop(); 					// kurze pause

	// mmc/sd in hardware spi
	#if (MMC_SOFT_SPI==FALSE)
		// hardware spi: bus clock = idle low, spi clock / 128 , spi master mode
		SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<SPR1);
	#endif

	// init sequenz, eigentlicher hintergrund ist, dass die karte eine stabile spannung hat (supply ramp up time)
	for (a = 0;a<10;a++) mmc_write_byte(0xFF);		// senden von 74+ clocks an die karte, hier 10*8=80

	MMC_Enable();
	#if (MMC_SDHC_SUPPORT==TRUE)
		// pruefung welcher kartentyp vorliegt
		if(FALSE==mmc_identify()){
			MMC_Disable();
			return FALSE;
		}
	#endif

	#if (MMC_SDHC_SUPPORT==FALSE)

		unsigned char cmd[] = {0x40,0x00,0x00,0x00,0x00,0x95};		// cmd0
		unsigned short b;

		// mit cmd0 wird unabhaengig ob mmc/sd oder sdhc die karte in den in_idle_state gebracht. das ist noetig um die initialisierung durchzufuehren
		b = 0;
		while(mmc_write_command (cmd) !=0x01){		// cmd0 (go_idle_state), response R1. R1 = 0x01 => in_idle_state, jetzt moeglich zu initialisieren
			if (b++>400){
				MMC_Disable();
				return FALSE; 						// in timeout gleaufen, liegt daran, dass die karte nicht korrekt auf cmd0 antwortet !
			}
		}

		// cmd1 (send_op_cond), response R1. fuer mmc oder sd standart < 2.0
		b = 0;
		cmd[0] = 0x40+1;
		while( mmc_write_command (cmd) !=0){	// cmd1, (send_op_cond), response R1
			if (b++>500){
				MMC_Disable();
				return FALSE; 	// in timeout gelaufen, weil die karte nicht korrekt auf cmd1 antwortet
			}
		}
	#endif

	// mmc/sd/sdhc in hardware spi
	#if (MMC_SOFT_SPI==FALSE)
	#if (MMC_MAX_SPEED==TRUE)
		//SPI Bus auf max Geschwindigkeit
		SPCR &= ~((1<<SPR0) | (1<<SPR1));
		SPSR |= (1<<SPI2X);
	#endif
	#endif
	// mmc/sd/sdhc deaktivieren

	MMC_Disable();
	return TRUE;
}


#if (MMC_MULTI_BLOCK==TRUE && MMC_OVER_WRITE == FALSE)


// **********************************************************************************************************************************
// stopt multiblock lesen
// **********************************************************************************************************************************
unsigned char mmc_multi_block_stop_read (void){

	unsigned char cmd[] = {0x40+12,0x00,0x00,0x00,0x00,0xFF};	// CMD12 (stop_transmission), response R1b (kein fehler, dann 0)
	unsigned char response;

	response = mmc_write_command (cmd);		// r1 antwort auf cmd12

	response = mmc_read_byte();				// dummy byte nach cmd12

	MMC_Disable();
	return response;
}


// **********************************************************************************************************************************
// stop multiblock schreiben
// **********************************************************************************************************************************
unsigned char mmc_multi_block_stop_write (void){

	unsigned char cmd[] = {0x40+13,0x00,0x00,0x00,0x00,0xFF};	// CMD13 (send_status), response R2
	unsigned char response;

	mmc_write_byte(0xFD);					// stop token

	mmc_wait_ready();

	response=mmc_write_command (cmd);		// cmd13, alles ok?

	mmc_wait_ready();

	MMC_Disable();
	return response;
}


// **********************************************************************************************************************************
// starten von multi block read. ab sektor addr wird der reihe nach gelesen. also addr++ usw...
// **********************************************************************************************************************************
unsigned char mmc_multi_block_start_read (unsigned long int addr){

	unsigned char cmd[] = {0x40+18,0x00,0x00,0x00,0x00,0xFF};	// CMD18 (read_multiple_block), response R1
	unsigned char response;

	// addressiertung bei mmc und sd (standart < 2.0) in bytes, also muss sektor auf byte adresse umgerechnet werden.
	// sd standart > 2.0, adressierung in sektoren, also 512 byte bloecke
	if(card_type==0) addr = addr << 9; //addr = addr * 512, nur wenn mmc/sd karte vorliegt

	cmd[1] = ((addr & 0xFF000000) >>24 );
	cmd[2] = ((addr & 0x00FF0000) >>16 );
	cmd[3] = ((addr & 0x0000FF00) >>8 );
	cmd[4] = (addr &  0x000000FF);

	MMC_Enable();

	mmc_wait_ready ();

	response=mmc_write_command (cmd);		// commando senden und response speichern

	while (mmc_read_byte() != 0xFE){		// warten auf start byte
		nop();
	};

	return response;
}


// **********************************************************************************************************************************
//multi block lesen von sektoren. bei aufruf wird immer ein sektor gelesen und immer der reihe nach
// **********************************************************************************************************************************
void mmc_multi_block_read_sector (unsigned char *Buffer){

	unsigned short a; 							// einfacher zähler fuer bytes eines sektors

	// mmc/sd in hardware spi, block lesen
	#if (MMC_SOFT_SPI==FALSE)
	   unsigned char tmp; 						// hilfs variable zur optimierung
	   a=512;
	   SPDR = 0xff;								// dummy byte
		do{										// 512er block lesen
			loop_until_bit_is_set(SPSR,SPIF);
			tmp=SPDR;
			SPDR = 0xff;						// dummy byte
			*Buffer=tmp;
			Buffer++;
		}while(--a);

	// mmc/sd/sdhc in software spi, block lesen
	#else
		a=512;
		do{
			*Buffer++ = mmc_read_byte();
		}while(--a);
	#endif

	mmc_read_byte();						// crc byte
	mmc_read_byte();						// crc byte

	while (mmc_read_byte() != 0xFE){		// warten auf start byte 0xFE, damit fängt jede datenuebertragung an...
		nop();
		}
}


// **********************************************************************************************************************************
// starten von multi block write. ab sektor addr wird der reihe nach geschrieben. also addr++ usw...
// **********************************************************************************************************************************
unsigned char mmc_multi_block_start_write (unsigned long int addr){

	unsigned char cmd[] = {0x40+25,0x00,0x00,0x00,0x00,0xFF};	// CMD25 (write_multiple_block),response R1
	unsigned char response;

	// addressiertung bei mmc und sd (standart < 2.0) in bytes, also muss sektor auf byte adresse umgerechnet werden.
	// sd standart > 2.0, adressierung in sektoren, also 512 byte bloecke
	if(card_type==0) addr = addr << 9; //addr = addr * 512

	cmd[1] = ((addr & 0xFF000000) >>24 );
	cmd[2] = ((addr & 0x00FF0000) >>16 );
	cmd[3] = ((addr & 0x0000FF00) >>8 );
	cmd[4] = (addr &  0x000000FF);

	MMC_Enable();

	response=mmc_write_command (cmd);		// commando senden und response speichern

	return response;
}


// **********************************************************************************************************************************
//multi block schreiben von sektoren. bei aufruf wird immer ein sektor geschrieben immer der reihe nach
// **********************************************************************************************************************************
unsigned char mmc_multi_block_write_sector (unsigned char *Buffer){

	unsigned short a;			// einfacher zähler fuer bytes eines sektors
	unsigned char response;

	mmc_write_byte(0xFC);

	// mmc/sd in hardware spi, block schreiben
	#if (MMC_SOFT_SPI==FALSE)
		unsigned char tmp;			// hilfs variable zur optimierung
		a=512;				// do while konstrukt weils schneller geht
		tmp=*Buffer;			// holt neues byte aus ram in register
		Buffer++;			// zeigt auf naechstes byte
		do{
			SPDR = tmp;    //Sendet ein Byte
			tmp=*Buffer;	// holt schonmal neues aus ram in register
			Buffer++;
			loop_until_bit_is_set(SPSR,SPIF);
		}while(--a);

	// mmc/sd in software spi, block schreiben
	#else
		a=512;
		do{
			mmc_write_byte(*Buffer++);
		}while(--a);
	#endif

	//CRC-Bytes schreiben
	mmc_write_byte(0xFF); //Schreibt Dummy CRC
	mmc_write_byte(0xFF); //CRC Code wird nicht benutzt

	response=mmc_read_byte();

	mmc_wait_ready();

	if ((response&0x1F) == 0x05 ){			// daten von der karte angenommen, alles ok.
		return TRUE;
	}

	return FALSE;							// daten nicht angenommen... hiernach muss stop token gesendet werden !

}


// **********************************************************************************************************************************
// wartet darauf, dass die mmc karte in idle geht
// **********************************************************************************************************************************
static unsigned char mmc_wait_ready (void){

	unsigned char response;
	unsigned short ticks=100;

	do{
		response = mmc_read_byte();
		ticks--;
	}while ( (response != 0xFF) && (ticks>0) );

	return ticks;
}


#endif


// **********************************************************************************************************************************
//Sendet ein Commando an die MMC/SD-Karte
// **********************************************************************************************************************************
static unsigned char mmc_write_command (unsigned char *cmd){

    unsigned char tmp ;
    unsigned short Timeout ;

    // dummy write, wird von manchen karten benötigt
    mmc_write_byte(0xFF);

    mmc_write_byte(cmd[0]);
	mmc_write_byte(cmd[1]);
	mmc_write_byte(cmd[2]);
	mmc_write_byte(cmd[3]);
	mmc_write_byte(cmd[4]);
	mmc_write_byte(cmd[5]);

	// warten auf gueltige antwort nach senden des kommandos
	Timeout=0;
	tmp=0xFF;
	do{
		tmp = mmc_read_byte();
		if (Timeout++ > 1000){
			break; // in timeout gelaufen, da die karte nicht korrekt antwortet
		}
	}while (tmp == 0xFF);
	return(tmp);
}


// **********************************************************************************************************************************
//Routine zum Empfangen eines Bytes von der MMC-Karte 
// **********************************************************************************************************************************
static unsigned char mmc_read_byte (void){

	// mmc/sd in hardware spi
	#if (MMC_SOFT_SPI==FALSE)
	  SPDR = 0xff;
	  loop_until_bit_is_set(SPSR,SPIF);
	  return (SPDR);

	// mmc/sd in software spi
	#else
	    unsigned char Byte=0;
	    unsigned char a;
		for (a=8; a>0; a--){							//das Byte wird Bitweise nacheinander Empangen MSB First
			MMC_Write |=(1<<SPI_Clock);					//setzt Clock Impuls wieder auf (High)
			if (bit_is_set(MMC_Read,SPI_MISO) > 0){ 	//Lesen des Pegels von MMC_MISO
				Byte |= (1<<(a-1));
			}
			else{
				Byte &=~(1<<(a-1));
			}
			MMC_Write &=~(1<<SPI_Clock); 				//erzeugt ein Clock Impuls (Low)
		}
		return (Byte);
	#endif
}


// **********************************************************************************************************************************
//Routine zum Senden eines Bytes zur MMC-Karte
// **********************************************************************************************************************************
static void mmc_write_byte (unsigned char Byte){

	#if (MMC_SOFT_SPI==TRUE)
		unsigned char a;
	#endif

	// mmc/sd in hardware spi
	#if (MMC_SOFT_SPI==FALSE)
		SPDR = Byte;    					//Sendet ein Byte
		loop_until_bit_is_set(SPSR,SPIF);

	// mmc/sd in software spi
	#else
		for (a=8; a>0; a--){					//das Byte wird Bitweise nacheinander Gesendet MSB First
			if (bit_is_set(Byte,(a-1))>0){		//Ist Bit a in Byte gesetzt
				MMC_Write |= (1<<SPI_MOSI); 	//Set Output High
			}
			else{
				MMC_Write &= ~(1<<SPI_MOSI); 	//Set Output Low
			}
			MMC_Write |= (1<<SPI_Clock); 		//setzt Clock Impuls wieder auf (High)
			MMC_Write &= ~(1<<SPI_Clock);		//erzeugt ein Clock Impuls (LOW)
		}
		MMC_Write |= (1<<SPI_MOSI);				//setzt Output wieder auf High
	#endif
}


// **********************************************************************************************************************************
//Routine zum schreiben eines Blocks(512Byte) auf die MMC/SD-Karte
// **********************************************************************************************************************************
unsigned char mmc_write_sector (unsigned long addr,unsigned char *Buffer){

	unsigned short a;
	unsigned char b;
	unsigned char tmp;

	// cmd24 zum schreiben eines blocks auf die karte
	unsigned char cmd[] = {0x40+24,0x00,0x00,0x00,0x00,0xFF}; 		//CMD24 (write_block),response R1
   
	// addressiertung bei mmc und sd (standart < 2.0) in bytes, also muss sektor auf byte adresse umgerechnet werden.
	// sd standart > 2.0, adressierung in sektoren, also 512 byte bloecke
	if(card_type==0) addr = addr << 9; //addr = addr * 512

	cmd[1] = ((addr & 0xFF000000) >>24 );
	cmd[2] = ((addr & 0x00FF0000) >>16 );
	cmd[3] = ((addr & 0x0000FF00) >>8 );
	cmd[4] = (addr &  0x000000FF);

	MMC_Enable();

	// sendet cmd24 an die karte, mit adresse als parameter. danach kann wenn die karte korrekt antwortet 512 bytes geschrieben werden
	if ( 0 != mmc_write_command(cmd) )  return FALSE;

	//Wartet einen Moment und sendet einige Clocks an die MMC/SD-Karte
	for (b=0;b<100;b++){
		mmc_read_byte();
	}

	//Sendet Start Byte an MMC/SD-Karte
	mmc_write_byte(0xFE);

	// mmc/sd in hardware spi, block schreiben
	#if (MMC_SOFT_SPI==FALSE)
		a=512;				// do while konstrukt weils schneller geht
		tmp=*Buffer;			// holt neues byte aus ram in register
		Buffer++;			// zeigt auf naechstes byte
		do{
			SPDR = tmp;    //Sendet ein Byte
			tmp=*Buffer;	// holt schonmal neues aus ram in register
			Buffer++;
			loop_until_bit_is_set(SPSR,SPIF);
		}while(--a);

	// mmc/sd in software spi, block schreiben
	#else
		a=512;
		do{
			mmc_write_byte(*Buffer++);
		}while(--a);
	#endif

	//CRC-Byte schreiben
	mmc_write_byte(0xFF); //Schreibt Dummy CRC
	mmc_write_byte(0xFF); //CRC Code wird nicht benutzt

	//Fehler beim schreiben? (Data Response XXX00101 = OK)
	if((mmc_read_byte()&0x1F) != 0x05) return FALSE;

	//Wartet auf MMC/SD-Karte Bussy
	while (mmc_read_byte() != 0xff){
		nop();
	}
	MMC_Disable();
	return TRUE;
}



// **********************************************************************************************************************************
//Routine zum lesen eines Blocks(512Byte) von der MMC/SD-Karte
// **********************************************************************************************************************************
unsigned char mmc_read_sector (unsigned long addr,unsigned char *Buffer){
   unsigned short a;
   // cmd17 zum lesen eines einzelnen blocks von der karte
   unsigned char cmd[] = {0x40+17,0x00,0x00,0x00,0x00,0xFF}; // CMD17 (read_single_block), response R1.
   
   // addressiertung bei mmc und sd (standart < 2.0) in bytes, also muss sektor auf byte adresse umgerechnet werden.
   // sd standart > 2.0, adressierung in sektoren, also 512 byte bloecke
   if(card_type==0) addr = addr << 9; //addr = addr * 512

   cmd[1] = ((addr & 0xFF000000) >>24 );
   cmd[2] = ((addr & 0x00FF0000) >>16 );
   cmd[3] = ((addr & 0x0000FF00) >>8 );
   cmd[4] = (addr &  0x000000FF);

   MMC_Enable();

   //Sendet Commando cmd an MMC/SD-Karte
   if( 0 != mmc_write_command(cmd) )  return FALSE;

   //Wartet auf Start Byte von der MMC/SD-Karte (0xFE/Start Byte)
   while (mmc_read_byte() != 0xFE){};

	// mmc/sd in hardware spi, block lesen
	#if (MMC_SOFT_SPI==FALSE)
	   unsigned char tmp;
	   a=512;
	   SPDR = 0xff;								// dummy byte
		do{										// 512er block lesen
			loop_until_bit_is_set(SPSR,SPIF);
			tmp=SPDR;
			SPDR = 0xff;						// dummy byte
			*Buffer=tmp;
			Buffer++;
		}while(--a);

	// mmc/sd in software spi, block lesen
	#else
		a=512;
		do{
			*Buffer++ = mmc_read_byte();
		}while(--a);
	#endif

   //CRC-Byte auslesen
   mmc_read_byte();//CRC - Byte wird nicht ausgewertet
   mmc_read_byte();//CRC - Byte wird nicht ausgewertet

   MMC_Disable();
   return TRUE;
}


// **********************************************************************************************************************************
#if(MMC_STATUS_INFO == TRUE)
  unsigned char mmc_present(void) {
	  return get_pin_present() == 0x00;
  }
#endif


  // **********************************************************************************************************************************
#if(MMC_STATUS_INFO == TRUE && MMC_WRITE == TRUE)
  unsigned char mmc_protected(void) {
	  return get_pin_protected() != 0x00;
  }
#endif


