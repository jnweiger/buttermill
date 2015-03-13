/*
 * 	Doku, siehe http://www.mikrocontroller.net/articles/AVR_FAT32
 *  Neuste Version: http://www.mikrocontroller.net/svnbrowser/avr-fat32/
 *	Autor: Daniel R.
 */

#include "cpu_mhz.h"
#include <string.h>	
#include <stdlib.h>
#include <util/delay.h>
#include "config.h"
#include "mmc.h"
#include "fat.h"
#include "file.h"
#include "led.h"

//*******************************************************************************************************************************
// funktionsprototypen die nur in dieser datei benutzt werden !
static void lsRowsOfClust (fptr uputs_ptr, unsigned long int start_sec);	// zeigt reihen eines clusters an, ab start_sec, muss zeiger auf eine ausgabe funktion überbeben bekommen
static void flushFileData(void);
//*******************************************************************************************************************************


//*******************************************************************************************************************************
// 2 moeglichkeiten beim oeffnen, datei existiert(return MMC_FILE_EXISTS) oder muss angelegt werden(return MMC_FILE_NEW)
// zuerst wird geprüft ob es die datei im verzeichniss gibt. danach wird entschieden, ob die datei geoeffnet wird oder angelegt.
// -beim oeffnen werden die bekannten cluster gesucht maximal MAX_CLUSTERS_IN_ROW in reihe. dann wird der 1. sektor der datei auf
// den puffer fat.sector geladen. jetzt kann man ffread lesen...
// -beim anlegen werden freie cluster gesucht, maximal MAX_CLUSTERS_IN_ROW in reihe. dann wird das struct file gefuellt.
// danach wird der dateieintrag gemacht(auf karte). dort wird auch geprueft ob genügend platz im aktuellen verzeichniss existiert.
// moeglicherweise wird der 1. cluster der datei nochmal geaendert. jetzt ist der erste frei sektor bekannt und es kann geschrieben werden.
//*******************************************************************************************************************************
unsigned char ffopen( unsigned char name[]){
	unsigned char file_flag=fat_loadFileDataFromDir(name);		//prüfung ob datei vorhanden und evetuelles laden des file struct
	if( file_flag==TRUE ){
		fat_getFatChainClustersInRow( file.firstCluster );		// verkettete cluster aus der fat-chain suchen.

		#if (MMC_MULTI_BLOCK==TRUE && MMC_OVER_WRITE==FALSE)
			fat.currentSectorNr=fat.startSectors;
			mmc_multi_block_start_read (fat.startSectors);
			mmc_multi_block_read_sector (fat.sector);
		#else
			fat_loadSector( fat.startSectors );		// lät die ersten 512 bytes der datei auf puffer:sector.
		#endif

		pt=&fat.sector[0];							// zeiger zum lesen auf sektor puffer
		return MMC_FILE_EXISTS;
	}

	#if (MMC_WRITE==TRUE)										// anlegen ist schreiben !
	else{														/** Datei existiert nicht, also anlegen !	(nur wenn schreiben option an ist)**/
		fat_getFreeClustersInRow(file.lastCluster);				// leere cluster suchen, ab letzem bekannten der datei
		strcpy((char*)file.name,(char*)name);					// ---	füllen des file struct, zum abschließenden schreiben.
		file.firstCluster=fat_secToClust(fat.startSectors);		// 		1. cluster der datei
		file.lastCluster=file.firstCluster;						// 		letzter bekannter cluster der datei
		file.attrib=32;											// ---	file.row wird in der funktion fat_getFreeRowOfDir geschrieben !!
		file.length=0;											// damit da nix drin steht ^^
		fat_makeFileEntry(file.name,file.attrib,0);				// DATEI ANLEGEN auf karte
		fat.currentSectorNr=fat_clustToSec(file.firstCluster);	// setzen des ersten sektors
		#if (MMC_MULTI_BLOCK==TRUE && MMC_OVER_WRITE==FALSE)
			mmc_multi_block_start_write(fat.currentSectorNr);
		#endif
		fat.bufferDirty=1;										// puffer dirty weil geschrieben und noch nicht auf karte.
		return MMC_FILE_NEW;
	}
	#endif


	#if (MMC_WRITE==FALSE)
		return MMC_FILE_NOT_EXISTS;								// wird nur bei readonly zurueckgegeben, wenn datei nicht gefunden wurde.
	#endif

}


//*******************************************************************************************************************************
// schließt die datei operation ab. eigentlich nur noetig wenn geschrieben/ueberschrieben wurde. es gibt 2 moeglichkeiten :
// 1. die datei wird geschlossen und es wurde über die alte datei länge hinaus geschrieben.
// 2. die datei wird geschlossen und man war innerhalb der datei groeße, dann muss nur der aktuelle sektor geschrieben werden.
// der erste fall ist komplizierter, weil ermittelt werden muss wie viele sektoren neu beschrieben wurden um diese zu verketten
// und die neue datei länge muss ermitt weden. abschließend wird entweder (fall 2) nur der aktuelle sektor geschrieben, oder
// der aktuallisierte datei eintrag und die cluster (diese werden verkettet, siehe fileUpdate() ).
//*******************************************************************************************************************************
unsigned char ffclose(void){


	#if (MMC_MULTI_BLOCK==TRUE && MMC_OVER_WRITE==FALSE)
		if(fat.bufferDirty==0)	mmc_multi_block_stop_read (); 	// stoppen von multiblock aktion
		else mmc_multi_block_stop_write (); 					// stoppen von multiblock aktion
	#endif

	#if (MMC_WRITE==TRUE) 										/** 2 moeglichkeiten beim schließen !!	(lesend spielt keine rolle, nichts muss geupdatet werden) **/
		if( file.length < (file.seek+fat.cntOfBytes) )	{		/** 1.) es wurde über die alte datei groeße hinaus geschrieben **/
			flushFileData();
		}
		else if( fat.bufferDirty==1){							/** 2.) nicht über alte datei länge hinaus **/
			fat_writeSector( fat.currentSectorNr );
		}
	#endif

	fat.cntOfBytes=0;
	file.seek=0;

	return TRUE;
}

#if (MMC_WRITE==TRUE)

// *******************************************************************************************************************************
// füllt grade bearbeiteten  sektor auf, verkette die bis jetzt benutzten cluster. macht datei eintrag update.
// *******************************************************************************************************************************
static void flushFileData(void){

	unsigned char save_name[13];											// zum sichern des dateinamens
	unsigned long int save_length=fat.cntOfBytes + file.seek;

	while( fat.cntOfBytes < 512 ){						// sektor ist beschrieben worden, daher rest mit 00 füllen
		fat.sector[fat.cntOfBytes]=0x00;				// beschreibt ungenutzte bytes mit 0x00
		fat.cntOfBytes++;
	}
	fat_writeSector(fat.currentSectorNr);								// schreibt bearbeiteten sektor auf karte

	strcpy((char *)save_name,(char *)file.name); 								// muss gesichert werden, wird sonst von der karte geladen und verändert !
	fat_setClusterChain(fat_secToClust(fat.startSectors),fat_secToClust(fat.currentSectorNr));	// verketten der geschriebenen cluster
	fat_loadFileDataFromDir(save_name);											// läd sektor, des datei eintrags, und läd daten von karte auf struct file!
	fat_makeRowDataEntry(file.row,save_name,32,file.firstCluster,save_length);	// macht eintrag im puffer
	file.length=save_length;													// damit man falls man weitere datei operationen macht nicht ein ffclose machen muss

	fat_writeSector(fat.currentSectorNr);										// schreibt datei eintrag auf karte

}
#endif

// *******************************************************************************************************************************
// offset byte wird übergeben. es wird durch die sektoren der datei gespult (gerechnet), bis der sektor mit dem offset byte erreicht
// ist, dann wird der sektor geladen und der zähler für die bytes eines sektors gesetzt. wenn das byte nicht in den sektoren ist,
// die "vorgesucht" wurden, müssen noch weitere sektoren der datei gesucht werden (sec > fat.endSectors).
// *******************************************************************************************************************************
void ffseek(unsigned long int offset){

	unsigned long int sec;								// sektor variable zum durchgehen durch die sektoren

	#if (MMC_MULTI_BLOCK==TRUE && MMC_OVER_WRITE==FALSE)
		unsigned char flag=0;

		if(offset==file.length) {			// zum ende spulen, kann dann nur schreiben sein...
			flag=1;
		}
		if(fat.bufferDirty==1){
			mmc_multi_block_stop_write();
		}
		else {
			mmc_multi_block_stop_read();
		}
	#endif

	#if (MMC_OVER_WRITE==TRUE && MMC_WRITE==TRUE)
		if( (file.seek+fat.cntOfBytes) > file.length ){		// man  muss den dateieintrag updaten, um die daten zu retten, falls vorher beschrieben!!
			flushFileData();								// fat verketten und datei update auf der karte !
		}
	#endif

	fat_getFatChainClustersInRow(file.firstCluster);	// suchen von anfang der cluster chain aus !
	sec=fat.startSectors;								// sektor variable zum durchgehen durch die sektoren
	file.seek=0;										// weil auch von anfang an der chain gesucht wird mit 0 initialisiert

	while( offset>=512 ){ 	 							/** suchen des sektors in dem offset ist  **/
		sec++;											// da byte nicht in diesem sektor ist, muss hochgezählt werden
		offset-=512;									// ein sektor weniger in dem das byte sein kann
		file.seek+=512;									// file.seek update, damit bei ffclose() die richtige file.length herauskommt
		fat.cntSecs-=1;									// damit die zu lesenden/schreibenden sektoren stimmen
		if ( 0==fat.cntSecs && offset!=0){								// es müssen mehr sektoren der datei gesucht werden
			fat_getFatChainClustersInRow(fat_getNextCluster( file.lastCluster ) );	// nachladen von clustern in der chain
			sec=fat.startSectors;						// setzen des 1. sektors der neu geladenen, zum weitersuchen !
		}
	}

	fat_loadSector(sec);  								// sektor mit offset byte laden
	fat.cntOfBytes = offset;							// setzen des lese zählers
	pt=&fat.sector[offset];								// zeiger zum lesen auf sektor puffer
	//file.seek+=offset;									// damit seek die richtige zahl hat.

	#if (MMC_MULTI_BLOCK==TRUE && MMC_OVER_WRITE==FALSE)
		if(flag==0) {
			mmc_multi_block_start_read (sec);			// starten von multiblock aktion
		}
		else {
			mmc_multi_block_start_write (sec);			// starten von multiblock aktion
			fat.bufferDirty=1;
		}
	#endif
}




#if (MMC_SMALL_FILE_SYSTEM==FALSE)

// *******************************************************************************************************************************
// wechselt verzeichniss. start immer im root Dir.
// es MUSS in das direktory gewechselt werden, in dem die datei zum lesen/schreiben ist !
// *******************************************************************************************************************************
unsigned char ffcd( unsigned char name[]){
	return fat_cd(name);
}


// *******************************************************************************************************************************
// zeigt reihen eines clusters an, wird für ffls benoetigt !
// es wird ab dem start sektor start_sec, der dazugehoerige cluster angezeigt. geprüft wird ob es ein richtiger
// eintrag in der reihe ist (nicht geloescht, nicht frei usw). die sektoren des clusters werden nachgeladen.
// die dateien werden mit namen und datei groeße angezeigt.
// *******************************************************************************************************************************
static void lsRowsOfClust (fptr uputs_ptr,unsigned long int start_sec){

  unsigned char row;					// reihen
  unsigned char sec=0;				// sektoren
  unsigned char tmp[12];				// tmp string zur umwandlung
  
  do{
	fat_loadSector(start_sec + sec);	// sektoren des clusters laden
	for(row=0;row<16;row++){			// geht durch reihen des sektors
	  fat_loadRowOfSector(row);			// reihe eines sektors (auf den puffer) laden
	  if( (file.attrib==0x20||file.attrib==0x10) && (file.name[0]!=0xE5 && file.name[0]!=0x00) ){	 		  
		  uputs_ptr(file.name);
		  uputs_ptr((unsigned char*)" ");
		  ultoa(file.length,(char*)tmp,10);
		  uputs_ptr(tmp);
		  uputs_ptr((unsigned char*)"\n");
		  }
	  }	
	}while(++sec<fat.secPerClust);
}


// *******************************************************************************************************************************
// zeigt inhalt eines direktory an.
// unterscheidung ob man sich im rootDir befindet noetig, weil bei fat16 im root dir eine bestimmt anzahl sektoren durchsucht
// werden müssen und bei fat32 ab einem start cluster ! ruft lsRowsOfClust auf um cluster/sektoren anzuzeigen.
// *******************************************************************************************************************************
void ffls(fptr uputs_ptr){

  unsigned long int clust;									// cluster
  unsigned int s;												// fat16 root dir sektoren

  if(fat.dir==0 && fat.fatType==16){							// IM ROOTDIR.	fat16		
	for(s=0;s<(unsigned int)(fat.dataDirSec+2-fat.rootDir);s++){	// zählt durch RootDir sektoren (errechnet anzahl rootDir sektoren).	
	  lsRowsOfClust(uputs_ptr,fat.rootDir+s);								// zeigt reihen eines root dir clust an
	  }
	}

  else {		
	if(fat.dir==0 && fat.fatType==32)clust=fat.rootDir;			// IM ROOTDIR.	fat32
	else clust=fat.dir;										// NICHT ROOT DIR
	while( !((clust>=0x0ffffff8&&fat.fatType==32)||(clust>=0xfff8&&fat.fatType==16)) ){	// prüft ob weitere sektoren zum lesen da sind (fat32||fat16)
	  lsRowsOfClust(uputs_ptr,fat_clustToSec(clust));												// zeigt reihen des clusters an
	  clust=fat_getNextCluster(clust);													// liest nächsten cluster des dir-eintrags
	  }
	}  
}


//*******************************************************************************************************************************
// wechselt in das parent verzeichniss (ein verzeichniss zurück !)
// die variable fat.dir enthält den start cluster des direktory in dem man sich grade befindet, anhand diesem,
// kann der "." bzw ".." eintrag im ersten sektor des direktory ausgelesen und das parent direktory bestimmt werden.
//*******************************************************************************************************************************
unsigned char ffcdLower(void){

  if(fat.dir==0)return FALSE;				// im root dir, man kann nicht hoeher !

  fat_loadSector(fat_clustToSec(fat.dir));	// laed 1. sektor des aktuellen direktory.
  fat_loadRowOfSector(1);					// ".." eintrag (parent dir) ist 0 wenn parent == root
  fat.dir=file.firstCluster;				// dir setzen
	
  return TRUE;
}


#ifndef __AVR_ATmega8__ 

#if (MMC_WRITE==TRUE)

// *******************************************************************************************************************************
// erstellt einen dir eintrag im aktuellen verzeichniss.
// prüft ob es den den dir-namen schon gibt, dann wird nichts angelegt.
// wenn ok, dann wird ein freier cluster gesucht, als ende markiert, der eintrag ins dir geschrieben.
// dann wird der cluster des dirs aufbereitet. der erste sektor des clusters enthält den "." und ".." eintrag.
// der "." hat den 1. cluster des eigenen dirs. der ".." eintrag ist der 1. cluster des parent dirs.
// ein dir wird immer mit 0x00 initialisiert ! also alle einträge der sektoren des clusters ( bis auf . und .. einträge)!
// *******************************************************************************************************************************
void ffmkdir( unsigned char name[]){

	unsigned char i;								// variable zum zaehlen der sektoren eines clusters.
	unsigned int j;								// variable zum zaehlen der sektor bytes auf dem puffer fat.sector.

	if(TRUE==fat_loadFileDataFromDir(name))			// prueft ob dirname im dir schon vorhanden, wenn ja, abbruch !
		return ;

	// cluster in fat setzen, und ordner eintrg im aktuellen verzeichniss machen.
	fat_getFreeClustersInRow(2);									// holt neue freie cluster, ab 2 ...
	fat_setCluster(fat_secToClust(fat.startSectors),0x0fffffff);	// fat16/32 cluster chain ende setzen.	(neuer ordner in fat)
	file.firstCluster=fat_secToClust(fat.startSectors);				// damit fat_makeFileEntry den cluster richtig setzen kann
	fat_makeFileEntry(name,0x10,0); 			// macht dir eintrag im aktuellen verzeichniss (legt ordner im partent verzeichniss an)

	// aufbereiten des puffers
	j=511;
	do{
		fat.sector[j]=0x00;						//schreibt puffer fat.sector voll mit 0x00==leer
	}while(j--);

	// aufbereiten des clusters
	for(i=1;i<fat.secPerClust;i++){				// ein dir cluster muss mit 0x00 initialisiert werden !
		fat_writeSector(fat.startSectors+i);	// loeschen des cluster (ueberschreibt mit 0x00), wichtig bei ffls!
	} 

	// aufbereiten des neuen dir sektors mit "." und ".." eintraegen !
	fat_makeRowDataEntry(0,(unsigned char*)".           ",0x10,file.firstCluster,0);	// macht "." eintrag des dirs
	fat_makeRowDataEntry(1,(unsigned char*)"..          ",0x10,fat.dir,0);			// macht ".." eintrag des dirs
	fat_writeSector(fat_clustToSec(file.firstCluster));									// schreibt eintraege auf karte !
}
#endif
#endif


#if (MMC_WRITE==TRUE)

//*******************************************************************************************************************************
// loescht datei/ordner aus aktuellem verzeichniss, wenn es die/den datei/ordner gibt.
// loescht dateien und ordner rekursiv !
//*******************************************************************************************************************************
unsigned char ffrm( unsigned char name[]){

  if(TRUE==fat_loadFileDataFromDir(name)){	// datei oder ordner ist vorhanden, nur dann loesch-operation

	if(file.attrib!=0x10){				// ist datei.
	  fat.sector[file.row<<5]=0xE5;		// datei geloescht markieren (file.row*32, damit man auf reihen anfang kommt)
	  if((file.row-1)>=0){				// gibt es einen eintrag davor ?
		  if(fat.sector[(file.row<<5)-21]==0x0f)fat.sector[(file.row<<5)-32]=0xE5;	// langer datei eintag auch geloescht.
		  }
	  fat.bufferDirty=1;				// eintrag in puffer gemacht.
	  if(0==fat_getNextCluster(file.firstCluster)) return TRUE;	// 1.cluster ist leer ?!?
	  fat_delClusterChain(file.firstCluster);		// loescht die zu der datei gehoerige cluster-chain aus der fat.
	  return TRUE;
	  }
 
  //TODO noch nicht optimal. zu viele schreib vorgänge beim loeschen von datei einträgen. max bis zu 16/sektor !
  else{							// ist ordner, dann rekursiv loeschen !!
	#ifndef __AVR_ATmega8__			// mega8 zu klein für die funktionalität....
	unsigned long int parent;
	unsigned long int own;
	unsigned long int clustsOfDir;		// um durch die cluster chain eines dirs zu gehen.
	unsigned char cntSecOfClust=0;
	unsigned char i=0;

	fat.sector[file.row<<5]=0xE5;			// loescht dir eintrag
		if((file.row-1)>=0){						// gibt es einen eintrag davor (langer datei eintrag)?
		  if(fat.sector[(file.row<<5)-21]==0x0f)fat.sector[(file.row<<5)-32]=0xE5;	// langer datei eintag auch geloescht
		  }
		fat.bufferDirty=1;									// puffer eintrag gemacht

		parent=fat.dir;										// der ordner in dem der zu loeschende ordner ist.
		own=file.firstCluster;								// der 1. cluster des ordners der zu loeschen ist.
		clustsOfDir=file.firstCluster;						// die "cluster" des zu loeschenden ordners

		do{													// geht durch cluster des dirs
		  fat_loadSector(fat_clustToSec(clustsOfDir));		// sektor des dirs laden
		  do{												// geht durch sektoren des clusters
			do{	  											// geht durch reihen des sektors
			  fat_loadRowOfSector(i);

			  if(file.attrib!=0x10 && file.attrib!=0x00 && file.name[0]!=0xE5){	// ist kein ordner,noch nicht geloescht, kein freier eintrag
				fat.sector[i<<5]=0xE5;						// erster eintrag der reihe als geloescht markiert
				fat.bufferDirty=1;							// puffer eintrag gemacht
				if(file.attrib==0x20){						// ist datei!
				  fat_delClusterChain(file.firstCluster);	// ist datei, dann cluster-chain der datei loeschen
				  fat_loadSector(fat_clustToSec(clustsOfDir)+cntSecOfClust);	// sektor neu laden, weil loeschen der chain, den puffer nutzt.
				  }
				}

			  if(file.attrib==0x10 && file.name[0]=='.'){	// "." oder ".." eintrag erkannt, loeschen !
				fat.sector[i<<5]=0xE5;						// eintrag als geloescht markiert
				fat.bufferDirty=1;							// puffer eintrag gemacht
				}

			  if(file.attrib==0x10 && file.name[0]!='.' && file.name[0]!=0xE5 && file.name[0]!=0){	// ordner erkannt !
				fat.sector[i<<5]=0xE5;						// dir eintrag als geloescht markiert
				fat.bufferDirty=1;							// puffer eintrag gemacht
				fat_loadSector(fat_clustToSec(file.firstCluster));	// sektor des dirs laden
				clustsOfDir=file.firstCluster;				// eigenes dir ist file.firstCluster
				own=file.firstCluster;						// eigener start cluster/dir
				fat_loadRowOfSector(1);						// ".." laden um parent zu bestimmen
				parent=file.firstCluster;					// parent sichern.
				cntSecOfClust=0;							// init von gelesenen sektoren und reihen !
				i=0;
				continue;
				}

			  if(file.name[0]==0x00){						// ende des dirs erreicht, wenn nicht voll !!
				if(parent==fat.dir){						// erfolgreich alles geloescht
				  fat_delClusterChain(own);					// cluster chain des ordners loeschen
				  return TRUE;
				  }
				fat_delClusterChain(own);					// cluster chain des ordners loeschen
				clustsOfDir=parent;							// parent ist jetzt wieder arbeisverzeichniss.
				own=parent;									// arbeitsverzeichniss setzten
				fat_loadSector(fat_clustToSec(own));		// sektor des dirs laden
				fat_loadRowOfSector(1);						// ".." laden um parent zu bestimmen
				parent=file.firstCluster;	  		  		// parent sichern.
				cntSecOfClust=0;							// init von gelesenen sektoren und reihen !
				i=0;
				continue;
				}

			  i++;
			  }while(i<16);								// geht durch reihen des sektors.
		
			i=0;											// neuer sektor -> reihen von vorne.
			cntSecOfClust++;
			fat_loadSector(fat_clustToSec(clustsOfDir)+cntSecOfClust);		// läd sektoren des clusters nach
			}while(cntSecOfClust<fat.secPerClust);							// geht durch sektoren des clusters.
	
		  cntSecOfClust=0;													// neuer cluster -> sektoren von vorne.
		  clustsOfDir=fat_getNextCluster(clustsOfDir);						// sucht neuen cluster der cluster-chain.
		  }while( !((clustsOfDir>=0x0ffffff8 && fat.fatType==32) || (clustsOfDir==0xfff8 && fat.fatType==16)) );		// geht durch cluster des dirs.
		  fat_delClusterChain(own);				// hier landet man, wenn der ordner voll war (auf cluster "ebene")!!
		  #endif
	  }
	
  }
  

  return FALSE;				// fehler, nicht gefunden?
}

#endif
#endif



// *******************************************************************************************************************************
// liest 512 bytes aus dem puffer fat.sector. dann werden neue 512 bytes der datei geladen, sind nicht genuegend verkettete
// sektoren in einer reihe bekannt, wird in der fat nachgeschaut. dann werden weiter bekannte nachgeladen...
// *******************************************************************************************************************************
unsigned char ffread(void){

	unsigned long int nc;

	if( pt ==&fat.sector[512] ){								// EINEN SEKTOR GLESEN (ab hier 2 moeglichkeiten !)

		fat.cntSecs=fat.cntSecs-1;								// anzahl der sektoren am stück werden weniger, bis zu 0 dann müssen neue gesucht werden...

		if( 0==fat.cntSecs ){		 						//1.) noetig mehr sektoren der chain zu laden (mit ein bisschen glück nur alle 512*MAX_CLUSTERS_IN_ROW bytes)

			#if (MMC_MULTI_BLOCK==TRUE && MMC_OVER_WRITE==FALSE)
				mmc_multi_block_stop_read ();					// stoppen von multiblock aktion
			#endif

			nc=fat_secToClust( fat.currentSectorNr );		// umrechnen der aktuellen sektor position in cluster
			nc=fat_getNextCluster(nc);						// in der fat nach neuem ketten anfang suchen
			fat_getFatChainClustersInRow(nc);				// zusammenhängende cluster/sektoren der datei suchen
			fat.currentSectorNr=fat.startSectors-1;			// hier muss erniedrigt werden, da nach dem if immer ++ gemacht wird

			#if (MMC_MULTI_BLOCK==TRUE && MMC_OVER_WRITE==FALSE)
				mmc_multi_block_start_read (fat.startSectors);	// starten von multiblock lesen ab dem neu gesuchten start sektor der kette.
			#endif
		}

		pt=&fat.sector[0];

		#if (MMC_MULTI_BLOCK==TRUE && MMC_OVER_WRITE==FALSE)
			fat.currentSectorNr+=1;
			mmc_multi_block_read_sector (fat.sector);			//2.) bekannte sektoren reichen noch, also einfach nachladen..
		#else
			fat_loadSector(fat.currentSectorNr+1);				/** 2.) die bekannten in einer reihe reichen noch.(nur alle 512 bytes)**/
		#endif
	}

	//file.seek++;
	return *pt++;
}

#if (MMC_WRITE==TRUE)
#if (MMC_OVER_WRITE==FALSE)			// nicht überschreibende write funktion

//*******************************************************************************************************************************
// schreibt 512 byte bloecke auf den puffer fat.sector. dann wird dieser auf die karte geschrieben. wenn genügend feie
// sektoren zum beschreiben bekannt sind(datenmenge zu groß), muss nicht in der fat nachgeschaut werden. sollten nicht genügend
// zusammenhängende sektoren bekannt sein, werden die alten verkettet und neue gesucht. es ist noetig sich den letzten bekannten
// einer kette zu merken -> file.lastCluster, um auch nicht zusammenhängende cluster verketten zu koennen (fat_setClusterChain macht das)!
//*******************************************************************************************************************************
void ffwrite( unsigned char c){

	fat.sector[fat.cntOfBytes++]=c;								// schreiben des chars auf den puffer sector und zähler erhoehen (pre-increment)

	if( fat.cntOfBytes==512 ){								/** SEKTOR VOLL ( 2 moeglichkeiten ab hier !) **/
		file.seek+=512;												// position in der datei erhoehen, weil grade 512 bytes geschrieben
		fat.cntOfBytes=0;	  										// rücksetzen des sektor byte zählers
		fat.cntSecs-=1;									// noch maximal cntSecs sekoren zum beschreiben bekannt...

		#if (MMC_MULTI_BLOCK==TRUE)
			mmc_multi_block_write_sector(fat.sector);		/** 1.) genügend leere zum beschreiben bekannt **/
		#else
			mmc_write_sector(fat.currentSectorNr,fat.sector);			//genügend leere zum beschreiben bekannt
		#endif

		fat.currentSectorNr+=1;											// nächsten sektor zum beschreiben.

		if( fat.cntSecs==0 ){								/** 2.) es ist noetig, neue freie zu suchen und die alten zu verketten (mit ein bischen glück nur alle 512*MAX_CLUSTERS_IN_ROW bytes) **/
			#if (MMC_MULTI_BLOCK==TRUE)
				mmc_multi_block_stop_write();
			#endif
			fat.bufferDirty=0;											// sonst würde das suchen von clustern wieder eine schreiboperation ausloesen... siehe fat_writeSector
			fat_setClusterChain( fat_secToClust(fat.startSectors) , fat_secToClust(fat.currentSectorNr-1) );	// verketten der beschriebenen
			fat_getFreeClustersInRow(file.lastCluster);					// suchen von leeren sektoren.
			fat.currentSectorNr=fat.startSectors;						// setzen des 1. sektors der neuen reihe zum schreiben.
			fat.bufferDirty=1;
			#if (MMC_MULTI_BLOCK==TRUE)
				mmc_multi_block_start_write(fat.currentSectorNr);
			#endif
		}
	}
}

#endif

#if (MMC_OVER_WRITE==TRUE)			// überschreibende write funktion, nicht performant, weil immer auch noch ein sektor geladen werden muss

//*******************************************************************************************************************************
// schreibt 512 byte bloecke auf den puffer fat.sector. dann wird dieser auf die karte geschrieben. wenn genuegend feie
// sektoren zum beschreiben bekannt sind, muss nicht in der fat nachgeschaut werden. sollten nicht genuegend zusammenhängende
// sektoren bekannt sein(datenmenge zu groß), werden die alten verkettet und neue gesucht. es ist noetig sich den letzten bekannten einer
// kette zu merken -> file.lastCluster, um auch nicht zusammenhaengende cluster verketten zu koennen (fat_setClusterChain macht das)!
// es ist beim überschreiben noetig, die schon beschriebenen sektoren der datei zu laden, damit man die richtigen daten
// hat. das ist bloed, weil so ein daten overhead von 50% entsteht. da lesen aber schneller als schreiben geht, verliert man nicht 50% an geschwindigkeit.
//*******************************************************************************************************************************
void ffwrite( unsigned char c){
	
	fat.sector[fat.cntOfBytes++]=c;								// schreiben des chars auf den puffer sector und zähler erhoehen (pre-increment)

	if( fat.cntOfBytes==512 ){									/** SEKTOR VOLL ( 3 moeglichkeiten ab hier !) **/

		fat.cntOfBytes=0;	  										// rücksetzen des sektor byte zählers.
		file.seek+=512;												// position in der datei erhoehen, weil grade 512 bytes geschrieben.
		mmc_write_sector(fat.currentSectorNr,fat.sector);		/** 1.) vollen sektor auf karte schreiben, es sind noch sektoren bekannt**/
		fat.currentSectorNr=fat.currentSectorNr+1;					// nächsten sektor zum beschreiben.
		fat.cntSecs=fat.cntSecs-1;

		if(fat.cntSecs==0){
			if( file.seek > file.length ){						/** 2.) außerhalb der datei, jetzt ist es noetig die beschriebenen cluster zu verketten und neue freie zu suchen	**/
				fat.bufferDirty=0;									// damit nicht durch z.b. fat_getNextCluster nochmal dieser sektor gescchrieben wird, siehe fat_loadSector
				fat_setClusterChain( fat_secToClust(fat.startSectors) , fat_secToClust(fat.currentSectorNr-1) );	// verketten der beschriebenen.
				fat_getFreeClustersInRow( file.lastCluster );		// neue leere sektoren benoetigt, also suchen.
				fat.currentSectorNr=fat.startSectors;				// setzen des 1. sektors der neuen reihe zum schreiben.
				fat.bufferDirty=1;
			}
			else {												/** 3.) noch innerhalb der datei, aber es muessen neue verkettete cluster gesucht werden, zum ueberschreiben **/
				fat_getFatChainClustersInRow( fat_getNextCluster(file.lastCluster) );		// noch innerhalb der datei, deshlab verkettete suchen.
				fat.currentSectorNr=fat.startSectors;				// setzen des 1. sektors der neuen reihe zum schreiben.
			}
		}
		if(file.seek<=file.length){
			mmc_read_sector(fat.currentSectorNr,fat.sector);		// wegen ueberschreiben, muss der zu beschreibende sektor geladen werden (zustand 3)...
		}
	}
}

#endif

//#if (MMC_SMALL_FILE_SYSTEM==FALSE)
// *******************************************************************************************************************************
// schreibt string auf karte, siehe ffwrite()
// ein string sind zeichen, '\0' bzw. 0x00 bzw dezimal 0 wird als string ende gewertet !!
// wenn sonderzeichen auf die karte sollen, lieber ffwrite benutzen!
// *******************************************************************************************************************************
void ffwrites( unsigned char *s ){
    while (*s) ffwrite(*s++);
    fat.bufferDirty=1;
  }

//#endif
#endif








