/*
 * 	Doku, siehe http://www.mikrocontroller.net/articles/AVR_FAT32
 *  Neuste Version: http://www.mikrocontroller.net/svnbrowser/avr-fat32/
 *	Autor: Daniel R.
 */



#ifndef _FAT_H
  #define _FAT_H

	// #######################################################################################################################
	// "daten" ketten siehe doku...
	// 1. fat_getFreeRowOfCluster -> fat_getFreeRowOfDir -> fat_makeRowDataEntry -> fat_makeFileEntry -> fat_writeSector  "eintrag gemacht !!"
	// 2. fat_loadSector -> fat_loadRowOfSector -> fat_loadFileDataFromCluster -> fat_loadFileDataFromDir (-> fat_cd)   "daten chain"

	// #######################################################################################################################
	// funktionen

  extern unsigned long int fat_clustToSec(unsigned long int);					// rechnet cluster zu 1. sektor des clusters um
  extern unsigned long int fat_secToClust(unsigned long int sec);				// rechnet sektor zu cluster um!
  extern unsigned long int fat_getNextCluster(unsigned long int oneCluster);	// fat auf nächsten, verketteten cluster durchsuchen
  extern unsigned long long int fat_getFreeBytes(void);						// berechnet den freien platz der karte in bytes!
  extern unsigned char 	fat_writeSector(unsigned long int sec);				// schreibt sektor auf karte
  extern unsigned char 	fat_loadSector(unsigned long int sec);				// läd übergebenen absoluten sektor
  extern unsigned char 	fat_loadFileDataFromDir( unsigned char name []);	// durchsucht das aktuelle directory
  extern unsigned char	fat_cd( unsigned char *);								// wechselt directory (start im rootDir)
  extern unsigned char	fat_loadFatData(void);								// läd fat daten
  extern void				fat_loadRowOfSector(unsigned short);				// läd reihe des geladen sektors auf struct:file
  extern unsigned char 	*fat_str(unsigned char *str);							// wandelt einen string so, dass er der fat konvention entspricht !
  extern void 				fat_setCluster( unsigned long int cluster, unsigned long int content); 	// setzt cluster inhalt in der fat
  extern void 				fat_delClusterChain(unsigned long int startCluster);						// loescht cluster-chain in der fat
  extern void 				fat_getFreeRowOfDir(unsigned long int dir);									// durchsucht directory nach feiem eintrag
  extern void 				fat_makeFileEntry( unsigned char name [],unsigned char attrib,unsigned long int length); // macht einen datei/ordner eintrag
  extern void 				fat_getFreeClustersInRow(unsigned long int offsetCluster);			// sucht zusammenhängende freie cluster aus der fat
  extern void 				fat_getFatChainClustersInRow( unsigned long int offsetCluster);	// sucht fat-chain cluster die zusammenhängen
  extern void 				fat_makeRowDataEntry(unsigned short row, unsigned char name [],unsigned char attrib,unsigned long int cluster,unsigned long int length);
  extern void 				fat_setClusterChain(unsigned long int startCluster, unsigned long int endCluster); // verkettet cluster zu einer cluster-chain
  extern unsigned short 	fat_getTime(void); // infos, siehe config.h in sektion: SCHALTER und ZEIT FUNKTIONEN
  extern unsigned short 	fat_getDate(void);


  // #######################################################################################################################
  // variablen

  extern struct Fat{						// fat daten (1.cluster, root-dir, dir usw.)
	unsigned short 		cntOfBytes;		// zäht geschriebene bytes eines sektors
	unsigned short	 		cntSecs;		// anzahl der sektoren am stück
	unsigned char 			bufferDirty;	// puffer wurde beschrieben, sector muss geschrieben werden bevor er neu geladen wird
	unsigned long int 	currentSectorNr;// aktuell geladener Sektor (in sector)  //beschleunigt wenn z.b 2* 512 byte puffer vorhanden, oder bei fat operationen im gleichen sektor
	unsigned long int 	startSectors;	// der erste sektor in einer reihe (freie oder verkettete)
	unsigned long int 	dir; 		  	// Direktory zeiger rootDir=='0' sonst(1.Cluster des dir; start auf root)
	unsigned long int  	rootDir;		// Sektor(f16)/Cluster(f32) nr root directory
	unsigned long int  	dataDirSec;		// Sektor nr data area 
	unsigned long int  	fatSec;	 		// Sektor nr fat area
	unsigned char 			secPerClust;	// anzahl der sektoren pro cluster
	unsigned char  		fatType;		// fat16 oder fat32 (16 oder 32)
	unsigned char 			sector[512];	// der puffer für sektoren !
	}fat;  

  extern struct File{					// datei infos usw.
	unsigned char 			name[13];		// 0,10			datei Name.ext (8.3 = max 11)(MUSS unsigned char weil E5)
	unsigned char 			attrib;			// 11,1			datei Attribut: 8=value name, 32=datei, 16=Verzeichniss, 15=linux kleingeschrieben eintrag
	unsigned char 			row;			// reihe im sektor in der die datei infos stehen (reihe 0-15)
	unsigned long int 	firstCluster;	// 20,2 /26,2	datei 1.cluster hi,low(moeglicherweise der einzige)	(4-byte)
	unsigned long int 	length;			// 28,4			datei Laenge (4-byte)
	unsigned long int 	lastCluster;	// -nicht direkt aus dem dateisystem- letzter cluster der ersten kette
	unsigned long int 	seek;			// schreib position in der datei
	}file;	

  unsigned char *pt;


#endif


 



