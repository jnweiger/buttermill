#include "cpu_mhz.h"
#include <util/delay.h>
#include <avr/io.h>
#include "led.h"

/** set up the pins and datadirections for the leds **/
void led_init(void)
{
    LED_DDR |=  (1<<GREEN);
    LED_DDR |=  (1<<RED);
    LED_DDR |=  (1<<YELLOW);
}

void debug(uint8_t val)
{
  if (val & 1)
	green_on();
  else
	green_off();
  if (val & 2)
	red_on();
  else
	red_off();
  if (val & 4)
	yellow_on();
  else
	yellow_off();
}

void green_on(void)
{
    LED_PORT |= (1<<GREEN);
}

void green_off(void)
{
    LED_PORT &= ~(1<<GREEN);
}

void red_on(void)
{
    LED_PORT |= (1<<RED);
}

void red_off(void)
{
    LED_PORT &= ~(1<<RED);
}

void yellow_on(void)
{
    LED_PORT |= (1<<YELLOW);
}

void yellow_off(void)
{
    LED_PORT &= ~(1<<YELLOW);
}

/** turn all leds on briefly **/
void lights_on(void)
{
  green_on();
  yellow_on();
  red_on();
  _delay_ms(150);
  green_off();
   yellow_off();
  red_off();
}

