/*
 * 	Doku, siehe http://www.mikrocontroller.net/articles/AVR_FAT32
 *  Neuste Version: http://www.mikrocontroller.net/svnbrowser/avr-fat32/
 *	Autor: Daniel R.
 */


#ifndef CONFIG_H_
#define CONFIG_H_

	//#####################################################################################
	// BOOL DEFINITIONEN

	#define TRUE 1
	#define FALSE 0

	//#####################################################################################
	// FLAGS

	// flags für ffopen (sind die rückgabewerte von ffopen, auf diese wird geprüft..)
	#define MMC_FILE_NEW 		0		// datei existierte nicht, wurde gerade angelegt !
	#define MMC_FILE_EXISTS 	1		// datei existiert, es kann gelesen werden !
	#define MMC_FILE_NOT_EXISTS     2		// datei existiert nicht, wird nur bei read-only zurueckgegeben, wenn die datei nicht gefunden wurde !


	//#####################################################################################
	// SCHALTER

	// schalter die den funktionsumfang aendern (fat/file):
	#define MMC_SMALL_FILE_SYSTEM 		TRUE// wenn TRUE dann ist kleines file system, wenn FALSE dann komplette file unterstützung ! das bedeutet, es werden manche funktionen mit kompiliert und manche nicht. z.b. ffmkdir oder getTime oder...
	#define MMC_WRITE 			TRUE	// wenn TRUE dann ist write an, wenn FALSE dann read only !
	#define MMC_OVER_WRITE 			FALSE	// wenn TRUE dann kann ffwrite dateien überschreiben (nicht sooo performant), wenn FALSE dann nur normales schreiben !
	#define MMC_MULTI_BLOCK 		FALSE	// wenn TRUE dann werden multiblock schreib/lese funktionen benutzt. ist schneller, wird aber möglicherweise nicht von allen karten unterstützt. wenn FALSE ist normale operation
	#define MMC_SDHC_SUPPORT		TRUE	// wenn TRUE dann mit sdhc unterstuetzung, wenn FALSE, dann nur mmc/sd karten

	// siehe auch abschnitt: ZEIT FUNKTIONEN, weiter unten
	#define MMC_TIME_STAMP 			FALSE 	// wenn FALSE, werden dummy werte in die entsprechenden felder der datei/ordner geschrieben, anstelle des richtigen datums/uhrzeit. wenn TRUE muessen 2 funktionen das korrekte datum/uhrzeit liefern!

	// vorsicht, da die variable die die sektoren zählt ein short ist (MMC_MAX_CLUSTERS_IN_ROW*fat.secPerClust) !!
	#define MMC_MAX_CLUSTERS_IN_ROW 256 	// gibt an wie viele cluster am stück ohne fat-lookup geschrieben bzw gelesen werden können, wenn die fat nicht fragmentiert ist !

	//#####################################################################################
	// SPI EINSTELLUNGEN

	// wenn MAX_SPEED FALSE dann wird die SPI bus geschwindigkeit nicht hoch gesetzt. ist zum testen von hardware die nicht richtig läuft
	#define MMC_MAX_SPEED TRUE
	#define MMC_STATUS_INFO FALSE

	// software spi? wenn TRUE muessen in der mmc.h noch die pins eingestellt werden über die die spi kommunikation laufen soll
	// es werden nur pins an einem port unterstuetzt !
	#define MMC_SOFT_SPI FALSE


	//#####################################################################################
	// TYPDEFINITIONEN

	// pointer typ (fptr) auf eine funktion mit einem arugment (arg) und ohne rückgabewert !
	// wird für die ausgabe benutzt, so kann in der main auch einfach eine eigene
	// uart routine verwendet werden! beispiel aufruf von ffls: "ffls(uputs);" wobei uputs eine funktion
	// der form "uputs(unsigned char *s)" ist
	typedef void(*fptr)(unsigned char* arg);


	//#####################################################################################
	// ZEIT FUNKTIONEN

	// rueckgabe wert in folgendem format:
	// bits [0-4]   => sekunde, gueltige werte: 0-29, laut spezifikation wird diese zahl beim anzeigen mit 2 multipliziert, womit man dann auf eine anzeige von 0-58 sekunden kommt !
	// bits [5-10]  => minuten, gueltige werte: 0-59
	// bits [11-15] => stunden, gueltige werte: 0-23
	// macht insgesammt 16 bit also eine 2 byte variable !
	/**extern unsigned short fat_getTime(); DIESE FUNKTION WURDE SCHON IN DER fat.c ERSTELLT UND IN DER fat.h DEKLARIERT !**/

	// rueckgabe wert in folgendem format:
	// bits [0-4]  => tag es monats, gueltige werte: 1-31
	// bits [5-8]  => monat des jahres, gueltige werte: 1-12
	// bits [9-15] => anzahl jahre seit 1980, gueltige werte: 0-127 (moegliche jahre demnach: 1980-2107)
	// macht insgesammt 16 bit also eine 2 byte variable !
	/**extern unsigned short fat_getDate(); DIESE FUNKTION WURDE SCHON IN DER fat.c ERSTELLT UND IN DER fat.h DEKLARIERT !**/


#endif
