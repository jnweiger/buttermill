#include "cpu_mhz.h"
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include "rtc.h"
#include "speed.h"
#include "led.h"
#include "main.h"
//#include "usart.h"
//#include "BCD.h"
#include "LCD_driver.h"
#include "LCD_functions.h"
#include "ADC.h"
#include "config.h"
#include "file.h"
#include "fat.h"
#include "mmc.h"
//#include "OSCCal.h"

#define SLEEP_MODE_IDLE 0
#define SLEEP_MODE_ADC _BV(SM0)
#define SLEEP_MODE_PWR_DOWN _BV(SM1)
#define SLEEP_MODE_PWR_SAVE (_BV(SM0) | _BV(SM1))
#define SLEEP_MODE_STANDBY (_BV(SM1) | _BV(SM2))
#define SLEEP_MODE_EXT_STANDBY (_BV(SM0) | _BV(SM1) | _BV(SM2)) 

typedef struct
{
	unsigned long dev_id;
	unsigned short app_version;
	unsigned short crc;
} bootldrinfo_t;
 
const bootldrinfo_t bootlodrinfo __attribute__ ((section (".bootldrinfo"))) = {DEVID, SWVERSIONMAJOR << 8 | SWVERSIONMINOR, 0x0000};

// Define the files on the sd card which are used for config and actual logging
unsigned char data_log_file[13]  = "DATA    TXT";
unsigned char config_file[13] = "CONF    TXT";
uint16_t real_log_interval = 0; // changed by conf.txt or main.h
//FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

extern unsigned int LCD_character_table[] PROGMEM;

char gAutoPressJoystick = FALSE;                // global variable used in "LCD_driver.c"
unsigned char state;                                    // helds the current state, according to 

/** Disable JTAG to have access to more IO pins **/
void disable_jtag(void)
{
        unsigned char sreg;
        sreg = SREG;
        cli();
        MCUCR |= ( 1 <<JTD );
        MCUCR |= ( 1 <<JTD );
        SREG = sreg;
}

int main(void)
{
  disable_jtag();
  lights_on();
  _delay_ms(500);
  _delay_ms(500);
#if 0
  OSCCAL_Calibrate();
  usart_9600_1mhz();
  stdin = stdout = &uart_str;
#endif
  rtc_init();
  LCD_Init();
  led_init();
  speed_init();

  char adc_temp[5];
  char adc_light[5];

  while (FALSE==mmc_init())
  {
    nop();
  }

  if(TRUE == fat_loadFatData())
  {
    if(MMC_FILE_EXISTS == ffopen(config_file))
    {
      unsigned long int seek=file.length;
      unsigned long int i = 0;
      do
      {
        // need to feed return of ffread into a string buffer
        unsigned char byte_iter = ffread();
        if (byte_iter == 't' && ffread() == '=')
        {
          uint8_t j = 0;
          // need to implement atoi as we only get our digits bytewise
          while(j++<10)
          {
            seconds = 10*seconds+ffread()-'0';
          }
        }

        else if(byte_iter == 'l' && ffread() ==  '=')
        {
          uint8_t k = 0;
          while(k++<10)
          {
            real_log_interval = 10*real_log_interval+ffread()-'0';
          }
        }

        while(byte_iter != '\n' && i++ < seek)
        {
          byte_iter = ffread();
        } 
      }while(i++ < seek);
      ffclose();
    }
  }

  if(real_log_interval == 0) // no log interval found in conf.txt
  {
    red_on();
    real_log_interval = LOG_INTERVAL;
    _delay_ms(400);
    red_off();
  }
  else
  {
    green_on();
    _delay_ms(500);
    green_off();
  }
  uint16_t countdown = real_log_interval;
  char str[100];
  for(;;)
  {
    uint32_t myseconds = seconds;
    while(seconds == myseconds)
    {
      //sleep_mode(); // wait for any interrupt
      set_sleep_mode(SLEEP_MODE_PWR_SAVE);
      cli();
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();
      sei();

    }
    if (countdown == 1)
    {
      ADC_init(TEMPERATURE_SENSOR);
      ADC_periphery(adc_temp);
      ADC_init(LIGHT_SENSOR);
      ADC_periphery(adc_light);
      ADC_periphery(adc_light); // call again in case the old temperature value is still in the system
      LCD_puts(adc_temp,1);
      //sprintf_P(str,PSTR("secs=%10lu w1=%3u w2=%3u w3=%3u temp=%3s light=%3s\n"),seconds,ppi0,ppi1,ppi2,adc_temp,adc_light);
      sprintf(str,"secs=%10lu w1=%3u w2=%3u w3=%3u temp=%3s light=%3s\n",seconds,ppi0,ppi1,ppi2,adc_temp,adc_light);
      //printf("%s",str);
      if(TRUE == fat_loadFatData())
      {
        if(MMC_FILE_EXISTS == ffopen(data_log_file))
        {
          ffseek(file.length);
          ffwrites(str);
          ffclose();
          green_on();
          _delay_ms(100);
          green_off();
        }
      }
      ppi0 = 0;
      ppi1 = 0;
      ppi2 = 0;
      countdown = real_log_interval;
    }
    else
    {
      countdown--;
    }
  }
  return 0;
}
