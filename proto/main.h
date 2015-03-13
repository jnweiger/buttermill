
//Revisions number
#define SWHIGH  2
#define SWLOW   8
#define SWREV	2
#define LOG_INTERVAL 10 // how many seconds between writing to sd?
#define SD_TIMEOUT 10
#define sbiBF(port,bit)  (port |= (1<<bit))   //set bit in port
#define cbiBF(port,bit)  (port &= ~(1<<bit))  //clear bit in port

#define nop()  __asm__ __volatile__ ("nop" ::)

// main.h

// Public Functions (used in menu.h and others)
//
void Initialization(void);
unsigned char StateMachine(char state, unsigned char stimuli);
char BootFunc(char input);
char PowerSaveFunc(char input);
char AutoPower(char input);
void Delay(unsigned int millisec);
char Revision(char input);
void OSCCAL_calibration(void);
char LogTiming(char input);
void PrintLogTime(char logtime, char logunit);
char UploadFunc(char input);
char ResetFlash(char input); 
void DumpFlash(char	dumpall);
void doLogging(void);
void DF_Print_page(unsigned int page, char recordstoprint, char mode);

//#define F_CPU   4000000 // defined in cpu_mhz.h
#define BOOL    char

#define FALSE   0
#define TRUE    (!FALSE)
#define NULL ((void *)0)

#define AUTO    3

// Macro definitions
// ( GNU compatability Macros )

#ifndef sbi
	#define _BV(bit)			(1 << (bit))
	#define sbi(x,bit)			(x |= _BV(bit)) // set bit  ( GNU compatability )
	#define cbi(x,bit)			(x &= ~_BV(bit)) // clear bit  ( GNU compatability )
#endif
#ifndef bit_is_set
	#define bit_is_set(sfr, bit) (sfr & _BV(bit))
	#define bit_is_clear(sfr, bit) (!(sfr & _BV(bit)))
#endif

// Menu state machine states
#define ST_AVRBF                        10
#define ST_AVRBF_REV                    11
#define ST_TIME                         20
#define ST_TIME_CLOCK                   21
#define ST_TIME_CLOCK_FUNC              22
#define ST_TIME_CLOCK_ADJUST            23
#define ST_TIME_CLOCK_ADJUST_FUNC       24
#define ST_TIME_CLOCKFORMAT_ADJUST      25
#define ST_TIME_CLOCKFORMAT_ADJUST_FUNC 36
#define ST_TIME_DATE                    27
#define ST_TIME_DATE_FUNC               28
#define ST_TIME_DATE_ADJUST             29
#define ST_TIME_DATE_ADJUST_FUNC        30
#define ST_TIME_DATEFORMAT_ADJUST       31
#define ST_TIME_DATEFORMAT_ADJUST_FUNC  32
#define ST_MUSIC                        40
#define ST_SOUND_MUSIC                  41
#define ST_MUSIC_SELECT                 42
#define ST_MUSIC_PLAY                   43
#define ST_VCARD                        50
#define ST_VCARD_FUNC                   51
#define ST_VCARD_NAME                   52
#define ST_VCARD_ENTER_NAME             53
#define ST_VCARD_ENTER_NAME_FUNC        54
#define ST_VCARD_DOWNLOAD_NAME          55
#define ST_VCARD_DOWNLOAD_NAME_FUNC     56
#define ST_TEMPERATURE                  60
#define ST_TEMPERATURE_FUNC             61
#define ST_VOLTAGE                      70
#define ST_VOLTAGE_FUNC                 71
#define ST_LIGHT                        80
#define ST_LIGHT_FUNC                   81
#define ST_OPTIONS                      90
#define ST_OPTIONS_DISPLAY              91
#define ST_OPTIONS_DISPLAY_CONTRAST     92
#define ST_OPTIONS_DISPLAY_CONTRAST_FUNC 93
#define ST_OPTIONS_BOOT                 94
#define ST_OPTIONS_BOOT_FUNC            95
#define ST_OPTIONS_POWER_SAVE           96
#define ST_OPTIONS_POWER_SAVE_FUNC      97
#define ST_OPTIONS_AUTO_POWER_SAVE      98
#define ST_OPTIONS_AUTO_POWER_SAVE_FUNC 99
#define ST_OPTIONS_TEST                 100
#define ST_OPTIONS_TEST_FUNC            101
//#define ST_OPTIONS_START		102
//#define ST_OPTIONS_START_FUNC		103
#define ST_OPTIONS_LOGTIME		104
#define ST_OPTIONS_LOGTIME_FUNC		105
#define ST_OPTIONS_UPLOAD		106
#define ST_OPTIONS_UPLOAD_FUNC		107
#define ST_SPEED			108
#define ST_SPEED_FUNC			109
#define ST_DIR				110
#define ST_DIR_FUNC			111
#define ST_OPTIONS_RESET		112
#define ST_OPTIONS_RESET_FUNC		113

//Button definitions
//
#define KEY_NULL    0
#define KEY_ENTER   1
#define KEY_NEXT    2
#define KEY_PREV    3
#define KEY_PLUS    4
#define KEY_MINUS   5

//constants to enable Channels on logger
//
#define EN_LOG_DATE	1
#define EN_LOG_CLOCK	1
#define EN_LOG_LIGHT	1
#define EN_LOG_BATTERY	1
#define EN_LOG_SHT	0
#define EN_LOG_SPEED	1
#define EN_LOG_TEMP	1
#define EN_LOG_DS1820	0
#define EN_LOG_DIR	1

//number of ADCs used by direction. 4 channels max
#define DIRECTION_ADCS	3

//Number of 1-Wire Temperature sensors
#define DS1820_COUNT	2


// Constants for controlling the behaviour of the flash
// when logging
//
#define BYTESPERPAGE		264
#define TOTALPAGESINFLASH	2048
#define DATESIZE		(2 * EN_LOG_DATE) + (3 * EN_LOG_CLOCK)
#define NOOFPARAMS		(EN_LOG_LIGHT+ (EN_LOG_DIR*DIRECTION_ADCS)+ EN_LOG_TEMP+ EN_LOG_SPEED+ EN_LOG_BATTERY+ (2 * EN_LOG_SHT))
#define BYTESPERPARAMETER	2
#define NONSTDPARAMS		(EN_LOG_TEMP + EN_LOG_SPEED+ EN_LOG_BATTERY+(2 * EN_LOG_SHT))
#define RECORDSIZE		(DATESIZE + (NOOFPARAMS * BYTESPERPARAMETER) + (3 * EN_LOG_DS1820 * DS1820_COUNT))
#define RECORDSPERPAGE		(BYTESPERPAGE/RECORDSIZE)

#define DUMP_NORMAL	0
#define DUMP_ALL	1
#define DUMP_BIN	2

// Global Variables defined in main.c
//
extern BOOL gEnableRollover;		// allow flash to reset after filling completly
extern char gRolloverFlash;		// number of times flash has filled
extern unsigned char gDataPosition;	// position in page of next write to df   
extern unsigned int gDataPage;		// current page to be written to the flash
extern BOOL gUploading;			// Let other functions know we are uploading from the df
extern BOOL gLogging;			// Enable automatic logging
extern BOOL gLogNow;			// Log at next oportunity
extern char gPowerSaveTimeout;		// time out period in minutes
extern BOOL gAutoPowerSave;		// Variable to enable/disable the Auto Power Save func
extern char gPowerSave;			// current state of Power save mode
extern BOOL glogSpeed;			// from speed.c to prevent sleep while timming speed.
extern char gUART;			// prevent entering power save, when using the UART

//switch on complete serial interface (can be removed to save space)
#define EXTENDED_USART	

//switch off some non serial interfaces to save space
#define MINIMAL_MENU


// WARNING!
// I have only ever used these next three options together
// I have no idea if they work individually
//

//switch off entire menu statemachine 
#define NO_MENUS 0

//disable the use of the joystick to reduce code size 
#define NO_JOYSTICK 1

// almost NO LCD display is implemented other than 
// start of frame interrupt routine for button debouncing
#define NO_LCD 0

//
//


//default setup
#define DEFAULT_POWERSAVETIMEOUT	5
#define DEFAULT_EN_POWERSAVE		TRUE;
#define DEFAULT_EN_LOGGING		TRUE;
#define DEFAULT_EN_ROLLOVER		FALSE;
#define DEFAULT_SECONDSTOLOG		1;

// Normal calibration
//#define OSC_MAX		6250
//#define	OSC_MIN		6120
//#define	OSC_NORM

// Alt osc for 0.9216Mhz (x4 = 3.6864)
#define F_OSC		3686400
#define OSC_MAX		5760
#define	OSC_MIN		5640
#define OSC_AL
