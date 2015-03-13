/*
 * 	Doku, siehe http://www.mikrocontroller.net/articles/AVR_FAT32
 *  Neuste Version: http://www.mikrocontroller.net/svnbrowser/avr-fat32/
 *	Autor: Daniel R.
 */


#ifndef _FILE_H

  #define _FILE_H

  //#######################################################################################################################
  // funktionen

  extern unsigned char ffread(void);                    // liest byte-weise aus der datei (puffert immer 512 bytes zwischen)
  extern void ffwrite( unsigned char c);		// schreibt ein byte in die geöffnete datei
  extern void ffwrites( unsigned char *s );		// schreibt string auf karte
  extern unsigned char ffopen( unsigned char name[]);	// kann immer nur 1 datei bearbeiten.
  extern unsigned char ffclose(void);                   // muss aufgerufen werden bevor neue datei bearbeitet wird.
  extern void ffseek(unsigned long int offset);	        // setzt zeiger:bytesOfSec auf position in der geöffneten datei.
  extern unsigned char ffcd( unsigned char name[]);     // wechselt direktory
  extern void ffls(fptr uputs_ptr);                     // zeigt direktory inhalt an, muss zeiger auf eine ausgabe funktion übergeben bekommen
  extern unsigned char ffcdLower(void);                 // geht ein direktory zurück, also cd.. (parent direktory)
  extern unsigned char ffrm( unsigned char name[]);     // löscht datei aus aktuellem verzeichniss.
  extern void ffmkdir( unsigned char name[]);           // legt ordner in aktuellem verzeichniss an.
  
  //#######################################################################################################################
  



#endif




