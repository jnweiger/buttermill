/*
 * 	Doku, siehe http://www.mikrocontroller.net/articles/AVR_FAT32
 *  Neuste Version: http://www.mikrocontroller.net/svnbrowser/avr-fat32/
 *	Autor: Daniel R.
 */
#include "cpu_mhz.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <util/delay.h>
#include "config.h"
#include "fat.h"
#include "file.h"
#include "mmc.h"
#include "led.h"
struct Fat fat;   					// wichtige daten/variablen der fat
struct File file;					// wichtige dateibezogene daten/variablen

//***************************************************************************************************************
// funktionsprototypen die nur in dieser datei benutzt werden !
static unsigned char	fat_loadFileDataFromCluster(unsigned long int sec ,  unsigned char name []);	// durchsucht die reihen des geladenen sektors
static unsigned char fat_getFreeRowOfCluster(unsigned long secStart);
//***************************************************************************************************************


#if (MMC_WRITE==TRUE)

//***************************************************************************************************************
// schreibt sektor nummer:sec auf die karte (puffer fat.sector) !!
// setzt bufferFlag=0 da puffer nicht dirty sein kann nach schreiben !
//***************************************************************************************************************
unsigned char fat_writeSector(unsigned long int sec){	
 
	fat.bufferDirty=0;								// buffer kann nicht dirty sein weil wird geschrieben		
	return (mmc_write_sector(sec,fat.sector));		// schreiben von sektor puffer
}

#endif


// ***************************************************************************************************************
// umrechnung cluster auf 1.sektor des clusters (möglicherweise mehrere sektoren/cluster) !
// ***************************************************************************************************************
unsigned long int fat_clustToSec(unsigned long int clust){

	return (fat.dataDirSec+2+ ((clust-2) * fat.secPerClust) );			// errechnet den 1. sektor der sektoren des clusters
}

// ***************************************************************************************************************
// umrechnung sektor auf cluster (nicht die position im cluster selber!!)
// ***************************************************************************************************************
unsigned long int fat_secToClust(unsigned long int sec){
 
  return ((sec-fat.dataDirSec-2+2*fat.secPerClust)/fat.secPerClust);	// umkerhrfunktion von fat_clustToSec
}


//***************************************************************************************************************
// läd sektor:sec auf puffer:sector zum bearbeiten im ram !
// setzt currentSectorNr auf richtigen wert (also den sektor der gepuffert ist). es wird geprueft
// ob der gepufferte sektor geändert wurde, wenn ja muss erst geschrieben werden, um diese daten nicht zu verlieren !
//***************************************************************************************************************
unsigned char fat_loadSector(unsigned long int sec){
	
  if(sec!=fat.currentSectorNr){							// nachladen nötig
	#if (MMC_WRITE==TRUE)
	  if(fat.bufferDirty==1) {
		  fat.bufferDirty=0;							// buffer kann nicht dirty sein weil wird geschrieben
		  mmc_write_sector(fat.currentSectorNr,fat.sector);			// schreiben von sektor puffer
	  }
	#endif
	mmc_read_sector(sec,fat.sector);					// neuen sektor laden
	fat.currentSectorNr=sec;							// aktualisiert sektor nummer (nummer des gepufferten sektors)
	return TRUE;
	}	
		
  else return TRUE;										// alles ok, daten sind schon da (sec==fat.currentSectorNr)

  return FALSE;											// fehler
}





// datei lesen funktionen:

// fat_loadSector -> fat_loadRowOfSector -> fat_loadFileDataFromCluster -> fat_loadFileDataFromDir -> fat_cd   "daten chain"

//***************************************************************************************************************
// läd die reihe:row des gepufferten sektors auf das struct:file. dort stehen dann
// alle wichgigen daten wie: 1.cluster,länge bei dateien, name des eintrags, reihen nummer (im sektor), attribut use...
//***************************************************************************************************************
void fat_loadRowOfSector(unsigned short row){

	void *vsector;								// voidpointer, damit man schoen umbiegen kann :)
	unsigned char i;							// laufindex fuer namen

	vsector=&fat.sector[row<<5];				// row wird mit 32 multipliziert um immer auf das anfangsbyte eines 32 byte eintrags zu kommen. (zeile 0=0,zeile 1=32,zeile 2=62 usw).

	for(i=0;i<11;i++,vsector++)								// datei/ordner name (0,10)
		file.name[i]=*(unsigned char*)vsector;

	file.attrib=*(unsigned char*)vsector;					// datei attribut, byte 11.	(11,1)
	vsector+=9;

	file.firstCluster=*(unsigned short*)vsector;			// high word von first.cluster (20,2)
	file.firstCluster=file.firstCluster<<16;
	vsector+=6;

	file.firstCluster|=*(unsigned short*)vsector;			// low word von first.cluster (26,2)
	vsector+=2;

	file.length=*(unsigned long int*)vsector;				// 4 byte von file.length (28,4)
}


//***************************************************************************************************************
// geht reihen weise durch sektoren des clusters mit dem startsektor:sec, und sucht nach der datei mit dem 
// namen:name. es werden die einzelnen sektoren nachgeladen auf puffer:sector vor dem bearbeiten.
// wird die datei in dem cluster gefunden ist return 0 , sonst return1.
//***************************************************************************************************************
static unsigned char fat_loadFileDataFromCluster(unsigned long int sec ,  unsigned char name []){
	
  unsigned char r;  						// variable um durch zeilen nummern zu zaehlen
  unsigned char s=0;						// variable um durch sektoren zu zaehlen

  do{										// sektoren des clusters pruefen
	r=0;									// neuer sektor, dann reihen von 0 an.
	mmc_read_sector(sec+s,fat.sector);		// läd den sektor sec auf den puffer fat.sector
	fat.currentSectorNr=sec+s;				// setzen des aktuellen sektors
	do{										// reihen des sektors pruefen
		fat_loadRowOfSector(r);				// zeile 0-15 auf struct file laden
		if(file.name[0]==0){				// wenn man auf erste 0 stößt muesste der rest auch leer sein!
			file.row=r;						// zeile sichern.
			return FALSE;
			}
		if(0==strncmp((char*)file.name,(char*)name,10)){		// zeile r ist gesuchte
		  file.row=r;					// zeile sichern.
		  return TRUE;
		  }
		r++;
		}while(r<16);					// zählt zeilennummer (16(zeilen) * 32(spalten) == 512 bytes des sektors)
	s++;
	}while(s<fat.secPerClust);			// geht durch sektoren des clusters

	return FALSE;						// fehler (datei nicht gefunden, oder fehler beim lesen)
}


//***************************************************************************************************************
// wenn dir == 0 dann wird das root direktory durchsucht, wenn nicht wird der ordner cluster-chain gefolgt, um
// die datei zu finden. es wird das komplette directory in dem man sich befindet durchsucht.
// bei fat16 wird der rootDir berreich durchsucht, bei fat32 die cluster chain des rootDir.
//***************************************************************************************************************
unsigned char fat_loadFileDataFromDir( unsigned char name []){

  unsigned int s;														// variable um durch sektoren zu zaehlen
    
  if(fat.dir==0 && fat.fatType==16){									// IM ROOTDIR.	fat16	
	for(s=0;s<(unsigned int)(fat.dataDirSec+2-fat.rootDir);s++){		// zählt durch RootDir sektoren (errechnet anzahl rootDir sektoren).
	  if(TRUE==fat_loadFileDataFromCluster(fat.rootDir+s,name)){		// sucht die datei, wenn da, läd daten (1.cluster usw)
		  return TRUE;
		  }
	  }
	}		
	
  else {	
	unsigned long int i;
	if(fat.dir==0 && fat.fatType==32)
        {
          i=fat.rootDir;
        }					// IM ROOTDIR.	fat32
	else
        { // we come through here
          i=fat.dir;
        }
	while(!((i>=0x0ffffff8&&fat.fatType==32)||(i>=0xfff8&&fat.fatType==16))){// prueft ob weitere sektoren zum lesen da sind (fat32||fat16)
	  if(TRUE==fat_loadFileDataFromCluster(fat_clustToSec(i),name)){	 // lät die daten der datei auf struct:file. datei gefunden (umrechnung auf absoluten sektor)
		  return TRUE;
	  }
	  i=fat_getNextCluster(i);// liest nächsten cluster des dir-eintrags (unterverzeichniss größer 16 einträge)
	  }
	}
  red_on();
  _delay_ms(2000);
  red_off();
  return FALSE;																// datei/verzeichniss nicht gefunden
}


#if (MMC_SMALL_FILE_SYSTEM==FALSE)

//***************************************************************************************************************
// start immer im root Dir (root dir=0).
// es MUSS in das direktory gewechselt werden, in dem die datei zum lesen/anhängen ist (außer root, da startet mann)!
//***************************************************************************************************************
unsigned char fat_cd( unsigned char name []){

  if(name[0]==0){									// ZUM ROOTDIR FAT16/32
	fat.dir=0;										// root dir 
	return TRUE;
	}

  if(TRUE==fat_loadFileDataFromDir(name)){			// NICHT ROOTDIR	(fat16/32)
	fat.dir=file.firstCluster;						// zeigt auf 1.cluster des dir	(fat16/32)	
	return TRUE;
	}

  return FALSE;									// dir nicht gewechselt (nicht da?) !!
}

#endif





#if (MMC_WRITE==TRUE)

// datei anlegen funktionen :

// ***************************************************************************************************************
// sucht leeren eintrag (zeile) im cluster mit dem startsektor:secStart.
// wird dort kein freier eintrag gefunden ist return (1).
// wird ein freier eintrag gefunden, ist die position der freien reihe auf file.row abzulesen und return (0).
// der sektor mit der freien reihe ist auf dem puffer fat.sector gepuffert.
// ****************************************************************************************************************
static unsigned char fat_getFreeRowOfCluster(unsigned long secStart){
    
  unsigned int b;								// variabl zum durchgenen der sektor bytes
  unsigned char s=0;							// variable zum zaehlen der sektoren eines clusters.
  
  do{
	file.row=0;									// neuer sektor(oder 1.sektor), reihen von vorne.
	if(TRUE==fat_loadSector(secStart+s)){		// laed sektor auf puffer fat.sector
	  for(b=0;b<512;b=b+32){					// zaehlt durch zeilen (0-15).
		if(fat.sector[b]==0x00||fat.sector[b]==0xE5) // prueft auf freihen eintrag (leer oder geloescht gefunden?).
			return TRUE;
		file.row++;								// naechste reihe im sektor.
		}
	  }
	s++;										// sektoren des clusters ++ weil einen geprueft.
	}while(s<fat.secPerClust);					// geht die sektoren des clusters durch (moeglicherweise auch nur 1. sektor).
  return FALSE;								// nicht gefunden in diesem cluster (== nicht OK!).
}


// ***************************************************************************************************************
// sucht leeren eintrag (zeile) im directory mit dem startcluster:dir.
// geht die cluster chain des direktories durch. dabei werden auch alle sektoren der cluster geprueft.
// wird dort kein freier eintrag gefunden, wird ein neuer leerer cluster gesucht, verkettet und der 
// 1. sektor des freien clusters geladen. die reihe wird auf den ersten eintrag gesetzt, da frei.
// anhand der reihe kann man nun den direktory eintrag vornehmen, und auf die karte schreiben.
// ****************************************************************************************************************
void fat_getFreeRowOfDir(unsigned long int dir){
  
  unsigned long int start=dir;

   // solange bis ende cluster chain.
  while( !((dir>=0x0ffffff8&&fat.fatType==32)||(dir>=0xfff8&&fat.fatType==16)) ){
	if(TRUE==fat_getFreeRowOfCluster(fat_clustToSec(dir)))return;	// freien eintrag in clustern, des dir gefunden !!
	start=dir;		
	dir=fat_getNextCluster(dir);	
	}									// wenn aus schleife raus, kein freier eintrag da -> neuer cluster nötig.

  dir=fat_secToClust(fat.startSectors);	// dir ist jetzt neuer cluster zum verketten !
  fat_setCluster(start,dir);			// cluster-chain mit neuem cluster verketten
  fat_setCluster(dir,0x0fffffff);		// cluster-chain ende markieren

  //es muessen neue gesucht werden, weil der bekannte aus file.c ja grade verkettet wurden. datei eintrag passte nicht mehr ins dir...
  fat_getFreeClustersInRow(2);							// neue freie cluster suchen, fuer datei.
  file.firstCluster=fat_secToClust(fat.startSectors);	// 1. cluster der datei
  file.lastCluster=fat_secToClust(fat.startSectors+fat.cntSecs);		// letzter bekannter cluster der datei

  fat.currentSectorNr=fat_clustToSec(dir);	// setzen des richtigen sektors, also auf den 1. der neu verketteten

  unsigned int j=511;
  do{
    fat.sector[j]=0x00;						//schreibt puffer fat.sector voll mit 0x00==leer
  	}while(j--);

  unsigned char i=1;						// ab 1 weil der 1.sektor des clusters eh noch beschrieben wird...
  do{
	fat_writeSector(fat.currentSectorNr+i);	// löschen des cluster (ueberschreibt mit 0x00), wichtig bei ffls,
	i++;
	}while(i<fat.secPerClust);

  file.row=0;								// erste reihe frei, weil grad neuen cluster verkettet !
}


//***************************************************************************************************************
// erstellt 32 byte eintrag einer datei, oder verzeichnisses im puffer:sector.
// erstellt eintrag in reihe:row, mit namen:name usw... !!  
// muss noch auf die karte geschrieben werden ! nicht optimiert auf geschwindigkeit.
//***************************************************************************************************************
void fat_makeRowDataEntry(unsigned short row, unsigned char name [],unsigned char attrib,unsigned long int cluster,unsigned long int length){

  fat.bufferDirty=1; 			// puffer beschrieben, also neue daten darin(vor lesen muss geschrieben werden)
  
  row=row<<5; 					// multipliziert mit 32 um immer auf zeilen anfang zu kommen (zeile 0=0,zeile 1=32,zeile 2=62 ... zeile 15=480)

  unsigned char i; 			// byte zähler in reihe von sektor (32byte eintrag)
  void *vsector; 				// void zeiger auf sector, um beliebig casten zu können
  vsector=&fat.sector[row]; 	// anfangs adresse holen ab der stelle auf sector geschrieben werden soll

  #if (MMC_TIME_STAMP==TRUE)
	  unsigned char new=FALSE;
	  // pruefung ob eintrag neu ist.
	  if(fat.sector[row]==0xE5||fat.sector[row]==0x00)	 new=TRUE;
  #endif

  // namen schreiben (0,10)
  for(i=0;i<11;i++,vsector++)  *(unsigned char*)vsector=name[i];

  // attrib schreiben (11,1)
  *(unsigned char*)vsector=attrib;
  vsector++;

  #if (MMC_TIME_STAMP==TRUE)

	  unsigned short time=fat_getTime();
	  unsigned short date=fat_getDate();

	  // reserviertes byte und milisekunden der erstellung (12,2)
	  *(unsigned short*)vsector=0x0000;
	  vsector+=2;

	  if(new==TRUE){
		  // creation time,date (14,4)
		  *(unsigned long int*)vsector=date;
		  *(unsigned long int*)vsector=*(unsigned long int*)vsector<<16;
		  *(unsigned long int*)vsector|=time;
	  }
	  vsector+=4;

	  // last access date (18,2)
	  *(unsigned short*)vsector=date;
	  vsector+=2;

	  // low word von cluster (20,2)
	  *(unsigned short*)vsector=(cluster&0xffff0000)>>16;
	  vsector+=2;

	  // last write time,date (22,4)
	  *(unsigned long int*)vsector=date;
	  *(unsigned long int*)vsector=*(unsigned long int*)vsector<<16;
	  *(unsigned long int*)vsector|=time;
	  vsector+=4;

	  // high word von cluster (26,2)
	  *(unsigned short*)vsector=(cluster&0x0000ffff);
	  vsector+=2;

	  // laenge (28,4)
	  *(unsigned long int*)vsector=length;
   #endif

   #if (MMC_TIME_STAMP==FALSE)

	  // 	unnoetige felder (12,8)
	  *(unsigned long int*)vsector=0x00000000;
	  vsector+=4;
	  *(unsigned long int*)vsector=0x00000000;
	  vsector+=4;

	  // low word	von cluster (20,2)
	  *(unsigned short*)vsector=(cluster&0xffff0000)>>16;
	  vsector+=2;

	  // unnoetige felder (22,4)
	  *(unsigned long int*)vsector=0x00000000;
	  vsector+=4;

	  // high word von cluster (26,2)
	  *(unsigned short*)vsector=(cluster&0x0000ffff);
	  vsector+=2;

	  // laenge (28,4)
	  *(unsigned long int*)vsector=length;
   #endif
}


//***************************************************************************************************************
// macht den datei eintrag im jetzigen verzeichniss (fat.dir).
// file.row enthält die reihen nummer des leeren eintrags, der vorher gesucht wurde, auf puffer:sector ist der gewuenschte
// sektor gepuffert. fuer fat16 im root dir muss andere funktion genutzt werden, als fat_getFreeRowOfDir (durchsucht nur dirs).
// fat.rootDir enthält bei fat32 den start cluster des directory, bei fat16 den 1. sektor des rootDir bereichs!
//***************************************************************************************************************
void fat_makeFileEntry( unsigned char name [],unsigned char attrib,unsigned long int length){
  
  unsigned int s;														// zähler fuer root dir sektoren fat16
 
  if(fat.dir==0&&fat.fatType==32)fat_getFreeRowOfDir(fat.rootDir);		// IM ROOT DIR (fat32)

  else if(fat.dir==0 && fat.fatType==16){								// IM ROOT DIR (fat16)
	for(s=0;s<(unsigned int)(fat.dataDirSec+2-fat.rootDir);s++){		// zählt durch RootDir sektoren (errechnet anzahl rootDir sektoren).
	  if(TRUE==fat_getFreeRowOfCluster(fat.rootDir+s))break;				// geht durch sektoren des root dir.
	  }
	}  

  else fat_getFreeRowOfDir(fat.dir);									// NICHT ROOT DIR fat32/fat16
		
  fat_makeRowDataEntry(file.row,name,attrib,file.firstCluster,length);	// schreibt file eintrag auf puffer
  fat_writeSector(fat.currentSectorNr);									// schreibt puffer auf karte
}

#endif





// fat funktionen:

//***************************************************************************************************************
// sucht folge Cluster aus der fat !
// erster daten cluster = 2, ende einer cluster chain 0xFFFF (fat16) oder 0xFFFFFFF (fat32),
// stelle des clusters in der fat, hat als wert, den nächsten cluster. (1:1 gemapt)!
//***************************************************************************************************************
unsigned long int fat_getNextCluster(unsigned long int oneCluster){
	unsigned long int i;
	unsigned long int j;
	void *bytesOfSec;
	// FAT 16
	if(fat.fatType==16){
		i=oneCluster>>8;;				// (i=oneCluster/256)errechnet den sektor der fat in dem oneCluster ist (rundet immer ab)
		j=(oneCluster<<1)-(i<<9);		// (j=(oneCluster-256*i)*2 == 2*oneCluster-512*i)errechnet das low byte von oneCluster
		fat_loadSector(i+fat.fatSec);	// ob neu laden nötig wird von fat_loadSector geprueft
		bytesOfSec=&fat.sector[j];		// zeiger auf puffer
		return *(unsigned short*)bytesOfSec;	// da der ram auch little endian ist, kann einfach gecastet werden
		}

	// FAT 32
	else{
		i=oneCluster>>7;				// (i=oneCluster/128)errechnet den sektor der fat in dem oneCluster ist (rundet immer ab)
		j=(oneCluster<<2)-(i<<9);		// (j=(oneCluster-128*i)*4 == oneCluster*4-512*i)errechnet das low byte von oneCluster
		fat_loadSector(i+fat.fatSec);	// ob neu laden nötig wird von fat_loadSector geprueft
		bytesOfSec=&fat.sector[j];		// zeiger auf puffer
		return *(unsigned long int*)bytesOfSec;	// da der ram auch little endian ist, kann einfach gecastet werden
	}


}


//***************************************************************************************************************
// sucht verkettete cluster einer datei, die in einer reihe liegen. worst case: nur ein cluster.
// sieht in der fat ab dem cluster offsetCluster nach. sucht die anzahl von MAX_CLUSTERS_IN_ROW,
// am stueck,falls möglich. prueft ob der cluster neben offsetCluster dazu gehört...
// setzt dann fat.endSectors und fat.startSectors. das -1 weil z.b. [95,98] = {95,96,97,98} = 4 sektoren
//***************************************************************************************************************
void fat_getFatChainClustersInRow( unsigned long int offsetCluster){

	unsigned short cnt=0;

	fat.startSectors=fat_clustToSec(offsetCluster);		// setzen des 1. sektors der datei
	fat.cntSecs=fat.secPerClust;

	while( cnt<MMC_MAX_CLUSTERS_IN_ROW ){
		if( (offsetCluster+cnt+1)==fat_getNextCluster(offsetCluster+cnt) )		// zählen der zusammenhängenden sektoren
			fat.cntSecs+=fat.secPerClust;
		else {
			file.lastCluster=offsetCluster+cnt;				// hier landet man, wenn es nicht MAX_CLUSTERS_IN_ROW am stueck gibt, also vorher ein nicht passender cluster gefunden wurde.
			return;
		}
		cnt+=1;
	}

	file.lastCluster=offsetCluster+cnt;						// hier landet man, wenn MAX_CLUSTERS_IN_ROW gefunden wurden
}


#if (MMC_WRITE==TRUE)

//***************************************************************************************************************
// sucht freie zusammenhängende cluster aus der fat. maximal MAX_CLUSTERS_IN_ROW am stueck.
// erst wir der erste frei cluster gesucht, ab offsetCluster(iklusive) und dann wird geschaut, ob der
// daneben auch frei ist. setzt dann fat.endSectors und fat.startSectors. das -1 weil z.b. [95,98] = {95,96,97,98} = 4 sektoren
//***************************************************************************************************************
void fat_getFreeClustersInRow(unsigned long int offsetCluster){

	unsigned short cnt=1; 							// variable fuer anzahl der zu suchenden cluster

	while(fat_getNextCluster(offsetCluster))		// suche des 1. freien clusters
		offsetCluster++;

	fat.startSectors=fat_clustToSec(offsetCluster);	// setzen des startsektors der freien sektoren (umrechnen von cluster zu sektoren)
	fat.cntSecs=fat.secPerClust;					// da schonmal mindestens einer gefunden wurde kann hier auch schon cntSecs damit gesetzt werden

	do{												// suche der nächsten freien
		if(0==fat_getNextCluster(offsetCluster+cnt) )	// zählen der zusammenhängenden sektoren
			fat.cntSecs+=fat.secPerClust;
		else return;								// cluster daneben ist nicht frei
		cnt+=1;
	}while( cnt<MMC_MAX_CLUSTERS_IN_ROW );				// wenn man hier raus rasselt, gibt es mehr freie zusammenhängende als MAX_CLUSTERS_IN_ROW
}

 
//***************************************************************************************************************
// verkettet ab startCluster bis einschließlich endClu
// es ist wegen der fragmentierung der fat nötig, sich den letzten bekannten cluster zu merken, 
// damit man bei weiteren cluster in einer reihe die alten cluster noch dazu verketten kann (so sind luecken im verketten möglich).
//***************************************************************************************************************
void fat_setClusterChain(unsigned long int startCluster, unsigned long int endCluster){

  fat_setCluster(file.lastCluster,startCluster);	// ende der chain setzen, bzw verketten der ketten
  
  while(startCluster!=endCluster){  
	 startCluster++;
	 fat_setCluster( startCluster-1 ,startCluster );// verketten der cluster der neuen kette
	 }
	 
  fat_setCluster(startCluster,0xfffffff);			// ende der chain setzen
  file.lastCluster=endCluster;						// ende cluster der kette updaten
  fat_writeSector(fat.currentSectorNr);				// schreiben des fat sektors auf die karte

}


//***************************************************************************************************************
// setzt den cluster inhalt. errechnet den sektor der fat in dem cluster ist, errechnet das low byte von
// cluster und setzt dann byteweise den inhalt:content.
// prueft ob buffer dirty (zu setztender cluster nicht in jetzt gepuffertem).
// pruefung erfolgt in fat_loadSector, dann wird alter vorher geschrieben, sonst gehen dort daten verloren !!
//***************************************************************************************************************
void fat_setCluster( unsigned long int cluster, unsigned long int content){

	unsigned long int i;
	unsigned long int j;

	// FAT 16
	if(fat.fatType==16){					
		i=cluster>>8;			// (i=cluster/256)errechnet den sektor der fat in dem cluster ist (rundet immer ab)
		j=(cluster<<1)-(i<<9);	// (j=(cluster-256*i)*2 == 2*cluster-512*i)errechnet das low byte von cluster

		if(TRUE==fat_loadSector(i+fat.fatSec)){		//	neu laden (fat_loadSector prueft ob schon gepuffert)
			void *bytesOfSec=&fat.sector[j];		// init des zeigers auf unterste adresse
			*(unsigned short*)bytesOfSec=content;// weil ram auch little endian ist geht das so :)
			}
		}

	// FAT 32
	else{
		i=cluster>>7;			// (i=cluster/128)errechnet den sektor der fat in dem cluster ist (rundet immer ab)
		j=(cluster<<2)-(i<<9);	//(j=(cluster-128*i)*4 == cluster*4-512*i)errechnet das low byte von cluster

		if(TRUE==fat_loadSector(i+fat.fatSec)){		//	neu laden (fat_loadSector prueft ob schon gepuffert)
			void *bytesOfSec=&fat.sector[j];		// init des zeigers auf unterste adresse
			*(unsigned long int*)bytesOfSec=content;		// weil ram auch little endian ist geht das so :)
			}
		}

	fat.bufferDirty=1;						// zeigt an, dass im aktuellen sector geschrieben wurde
}


//***************************************************************************************************************
// löscht cluster chain, beginnend ab dem startCluster.
// sucht cluster, setzt inhalt usw.. abschließend noch den cluster-chain ende markierten cluster löschen.
//***************************************************************************************************************
void fat_delClusterChain(unsigned long int startCluster){

  unsigned long int nextCluster=startCluster;		// tmp variable, wegen verketteter cluster..  
	
  do{
	 startCluster=nextCluster;
	 nextCluster=fat_getNextCluster(startCluster);
	 fat_setCluster(startCluster,0x00000000);  	
  }while(!((nextCluster>=0x0ffffff8&&fat.fatType==32)||(nextCluster>=0xfff8&&fat.fatType==16)));

  fat_writeSector(fat.currentSectorNr);
}
 
#endif


//***************************************************************************************************************
// Initialisiert die Fat(16/32) daten, wie: root directory sektor, daten sektor, fat sektor...
// siehe auch Fatgen103.pdf. ist NICHT auf performance optimiert!
// byte/sector, byte/cluster, anzahl der fats, sector/fat ... (halt alle wichtigen daten zum lesen ders datei systems!)
//*****************************************************************<**********************************************
unsigned char fat_loadFatData(void){

										// offset,size
	unsigned short  	rootEntCnt;		// 17,2				groeße die eine fat belegt
	unsigned short  	fatSz16;		// 22,2				sectors occupied by one fat16
	unsigned long int fatSz32;		// 36,4				sectors occupied by one fat32
	unsigned long int secOfFirstPartition;				// ist 1. sektor der 1. partition aus dem MBR


	if(TRUE==mmc_read_sector(0,fat.sector)){						//startsektor bestimmen

		void *vsector=&fat.sector[454];

		secOfFirstPartition=*(unsigned long int*)vsector;

		//pruefung ob man schon im VBR gelesen hat (0x6964654d = "Medi")
		if(secOfFirstPartition==0x6964654d){					//ist superfloppy sonst ist secOfFirstPartition der sektor den man laden muss
			secOfFirstPartition=0;
		}
		else{													// ist nicht superfloppy, deshalb laden des vbr sektors
			mmc_read_sector(secOfFirstPartition,fat.sector);	// lesen von fat sector und bestimmen der wichtigen berreiche
		}

		fat.secPerClust=fat.sector[13];		// fat.secPerClust, 13 only (power of 2)

		vsector=&fat.sector[14];
		fat.fatSec=*(unsigned short*)vsector;

		vsector=&fat.sector[17];
		rootEntCnt=*(unsigned short*)vsector;

		vsector=&fat.sector[22];
		fatSz16=*(unsigned short*)vsector;

		fat.rootDir	 = ( (rootEntCnt <<5) + 511 ) /512;	// ist 0 bei fat 32, sonst der root dir sektor

		if(fat.rootDir==0){									// FAT32 spezifisch (die pruefung so, ist nicht spezifikation konform !).
			vsector=&fat.sector[36];
			fatSz32=*(unsigned long int *)vsector;

			vsector=&fat.sector[44];
			fat.rootDir=*(unsigned long int *)vsector;

			fat.dataDirSec = fat.fatSec + (fatSz32 * fat.sector[16]);	// data sector (beginnt mit cluster 2)
			fat.fatType=32;									// fat typ
			}

		else{												// FAT16	spezifisch
			fat.dataDirSec = fat.fatSec + (fatSz16 * fat.sector[16]) + fat.rootDir;		// data sektor (beginnt mit cluster 2)
			fat.rootDir=fat.dataDirSec-fat.rootDir;			// root dir sektor, da nicht im datenbereich (cluster)
			fat.rootDir+=secOfFirstPartition;				// addiert den startsektor auf 	"
			fat.fatType=16;									// fat typ
			}

		fat.fatSec+=secOfFirstPartition;					// addiert den startsektor auf
		fat.dataDirSec+=secOfFirstPartition;				// addiert den startsektor auf (umrechnung von absolut auf real)
		fat.dataDirSec-=2;									// zeigt auf 1. cluster
		fat.dir=0;											// dir auf '0'==root dir, sonst 1.Cluster des dir
		return TRUE;
		}

return FALSE;			// sector nicht gelesen, fat nicht initialisiert!!
}


#if (MMC_SMALL_FILE_SYSTEM==FALSE)

// *****************************************************************************************************************
// gibt akuelles datum zurueck
// *****************************************************************************************************************
unsigned short fat_getDate(void){
	// rueckgabe wert in folgendem format:
	// bits [0-4]  => tag es monats, gueltige werte: 1-31
	// bits [5-8]  => monat des jahres, gueltige werte: 1-12
	// bits [9-15] => anzahl jahre seit 1980, gueltige werte: 0-127 (moegliche jahre demnach: 1980-2107)
	// macht insgesammt 16 bit also eine 2 byte variable !

	// 22.12.2009 	(2009-1980=29)
	unsigned short date=0;
	date|=22|(12<<5)|(29<<9);
	// hier muss code hin, der richtige werte auf date erstellt. im format: siehe config.h

	return date;
}


// *****************************************************************************************************************
// gibt aktuelle zeit zurueck
// *****************************************************************************************************************
unsigned short fat_getTime(void){
	// rueckgabe wert in folgendem format:
	// bits [0-4]   => sekunde, gueltige werte: 0-29, laut spezifikation wird diese zahl beim anzeigen mit 2 multipliziert, womit man dann auf eine anzeige von 0-58 sekunden kommt !
	// bits [5-10]  => minuten, gueltige werte: 0-59
	// bits [11-15] => stunden, gueltige werte: 0-23
	// macht insgesammt 16 bit also eine 2 byte variable !

	// 16:58 und 20 sekunden (10*2=20 muss so laut spezifikation !)
	unsigned short time=0;
	time|=10|(58<<5)|(16<<11);
	// hier muss code hin, der richtige werte auf time erstellt !!

	return time;
}


// *****************************************************************************************************************
// zaehlt die freien cluster in der fat. gibt die anzahl der freien bytes zurueck !
// vorsicht, kann lange dauern !
// ist nicht 100% exakt, es werden bis zu ein paar hundert bytes nicht mit gezaehlt, da man lieber ein paar bytes verschwenden
// sollte, als zu versuchen auf bytes zu schreiben die nicht da sind ;)
// *****************************************************************************************************************
unsigned long long int fat_getFreeBytes(void){
	unsigned long long int bytes;							// die tatsaechliche anzahl an bytes
	unsigned long int count;								// zaehler fuer cluster in der fat
	unsigned long int fatSz;								// zuerst groeße der fat in sektoren, dann letzter sektor der fat in clustern
	unsigned short tmp;									// hilfsvariable zum zaehlen

	// bestimmen des 1. sektors der 1. partition (ist nach der initialisierung nicht mehr bekannt...)
	fat_loadSector(0);
	void *vsector=&fat.sector[454];
	fatSz=*(unsigned long int*)vsector;
	if(fatSz==0x6964654d){
		fatSz=0;
		}
	else{
		fat_loadSector(fatSz);
		}

	// anzahl sektoren bestimmen die von einer fat belegt werden
	if(fat.fatType==32){
		vsector=&fat.sector[36];
		fatSz=(*(unsigned long int *)vsector)-1;
		fatSz*=128;										// multipliziert auf cluster/sektor in der fat, also ein sektor der fat beinhaltet 128 cluster nummern
		}
	else{
		vsector=&fat.sector[22];
		fatSz=(*(unsigned short*)vsector)-1;
		fatSz*=256;										// multipliziert auf cluster/sektor in der fat, also ein sektor der fat beinhaltet 256 cluster nummern
		}

	// variablen initialisieren
	tmp=0;
	bytes=0;
	count=0;

	// zaehlen der freien cluster in der fat, mit hilfsvariable um nicht eine 64 bit variable pro freien cluster hochzaehlen zu muessen
	do{
		if(0==fat_getNextCluster(count)) tmp++;
		if(tmp==254){
			bytes+=254;
			tmp=0;
		}
		count++;
	}while(count<fatSz);
	bytes+=tmp;

	// multiplizieren um auf bytes zu kommen
	return bytes*fat.secPerClust*512;
}


// *****************************************************************************************************************
// bereitet str so auf, dass man es auf die mmc/sd karte schreiben kann.
// wandelt z.b. "t.txt" -> "T        TXT" oder "main.c" in "MAIN    C  " => also immer 8.3 und upper case letter
// VORSICHT es werden keine Pruefungen gemacht !
// *****************************************************************************************************************
unsigned char * fat_str(unsigned char *str){

  unsigned char i;
  unsigned char j=0;
  unsigned char c;
  unsigned char tmp[12];					// tmp string zum einfacheren umwandeln
  
  strcpy((char*)tmp,(char*)str);

  for(i=0;i<11;i++)str[i]=' ';				// als vorbereitung alles mit leerzeichen fuellen
  str[11]='\0';

  i=0;

  do{
	c=toupper(tmp[j]);
	if(c=='\0')return str;
	if(c!='.')str[i]=c;
	else i=7;
	i++;
	j++;
	}while(i<12);

  return str;
  
}

#endif

