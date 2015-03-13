#include "cpu_mhz.h"
#include <avr/io.h>
#include <string.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/crc16.h>
#include <util/delay.h>
#include "fat16.h"
#include "led.h"
#include <stdlib.h>
// cfarrell: changed these to match buttermill layout
#define USE_FLASH_LED
#define nop()  __asm__ __volatile__ ("nop" ::)

typedef struct
{
	unsigned long dev_id;
	unsigned short app_version;
	unsigned short crc;
} bootldrinfo_t;


uint16_t startcluster;
uint16_t updatecluster; //is set when update is available
uint32_t filesize;
bootldrinfo_t current_bootldrinfo;

void (*app_start)(void) = 0x0000;
/** Disable JTAG because leds hang here **/

void disable_jtag(void)
{
        unsigned char sreg;
        sreg = SREG;
        cli();
        MCUCR |= ( 1 <<JTD );
        MCUCR |= ( 1 <<JTD );
        SREG = sreg;
}

static inline void check_file(void)
{

	if (filesize != (FLASHEND - BOOTLDRSIZE + 1)) // in orig code this is !=
        {
	  return;
        }

	bootldrinfo_t *file_bootldrinfo;
	fat16_readfilesector(startcluster, (FLASHEND - BOOTLDRSIZE + 1) / 512 - 1);
	
	file_bootldrinfo =  (bootldrinfo_t*) (uint8_t*) (fat_buf + (FLASHEND - BOOTLDRSIZE - sizeof(bootldrinfo_t) + 1) % 512);
	
	//Check DEVID - if they are not the same return (firmware not for our device)
	if (file_bootldrinfo->dev_id != DEVID)// DEVID comes from Makefile and is 0x1e9405
        {
          return;
        }
		
	//Check application version
	if (file_bootldrinfo->app_version <= current_bootldrinfo.app_version)
        {
	  return;
        }
        // Update candidate available
	current_bootldrinfo.app_version = file_bootldrinfo->app_version;
	updatecluster = startcluster;
}

static inline void flash_update(void)
{
	uint16_t filesector, j;
	uint8_t i;
	uint16_t *lpword;
	uint16_t adr;
        yellow_on();	
	for (filesector = 0; filesector < (FLASHEND - BOOTLDRSIZE + 1) / 512; filesector++)
	{
		lpword = (uint16_t*) fat_buf;
		fat16_readfilesector(updatecluster, filesector);
	
		for (i=0; i<(512 / SPM_PAGESIZE); i++)
		{
                   red_on();
			adr = (filesector * 512) + i * SPM_PAGESIZE;
			boot_page_erase(adr);
			while (boot_rww_busy())
				boot_rww_enable();
			
			for (j=0; j<SPM_PAGESIZE; j+=2)
				boot_page_fill(adr + j, *lpword++);

			boot_page_write(adr);
			while (boot_rww_busy())
				boot_rww_enable();
                   red_off();
		}
	}
        yellow_off();
	
}

int main(void)
{
	uint16_t i;
        disable_jtag();
	led_init();
        lights_on();
        _delay_ms(100);
        lights_on();
        _delay_ms(100);
        lights_on();
        _delay_ms(100);
        // copy from ((0x3800)-8bytes+1) until ((0x3800)-8bytes+1)+8bytes ???
        // would copy 8bytes from flash into the reserved space for current_bootldrindo (?)
	memcpy_P(&current_bootldrinfo, (uint8_t*) FLASHEND - BOOTLDRSIZE - sizeof(bootldrinfo_t) + 1, sizeof(bootldrinfo_t));
	
	if (current_bootldrinfo.app_version == 0xFFFF) // 0xFFFF = 65535
	{ 
	  current_bootldrinfo.app_version = 0; //application not flashed yet
        }

	if (fat16_init() == 0)
	{ // Found an SD Card
		for (i=0; i<512; i++)
		{
		  startcluster = fat16_readRootDirEntry(i);
                  if (startcluster == 0xFFFF)
                  {
                    continue;
                  }
                  else
                  {
                    check_file();
                  }
		}
		
		if (updatecluster)
                {
		  flash_update();
                }
	}
        else
        { // No SD Card in the slot. Blink red until we get one
          for(;;){
            red_on();
            _delay_ms(200);
            red_off();
            _delay_ms(200);
          }
        }


	unsigned short flash_crc = 0xFFFF;
	
	unsigned short adr;
	
	for (adr=0; adr<FLASHEND - BOOTLDRSIZE + 1; adr++)
		flash_crc = _crc_ccitt_update(flash_crc, pgm_read_byte(adr));
		
	if (flash_crc == 0)
	{
          app_start();
	}
	
	while (1);
}
