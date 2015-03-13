#include <avr/interrupt.h>
#include "cpu_mhz.h"
#include <util/delay.h>
#include "speed.h"
#include "led.h"

/** Set these up as volatile as they are changed by interrupts **/
volatile uint8_t counter0 = 0;
volatile uint8_t counter1 = 0;
volatile uint8_t counter2 = 0;

volatile uint8_t last0 = 0; // holds the last value of counter[0-2]
volatile uint8_t last1 = 0;
volatile uint8_t last2 = 0;

volatile uint16_t ppi0 = 0; // increment these integers with pulses per sec until LOG_INTERVAL_SECS reached
volatile uint16_t ppi1 = 0;
volatile uint16_t ppi2 = 0;


/** Set up the interrupt handling for the anemometer pins **/
void speed_init(void)
{

  // datadir for incoming sensor pulses
  WMILL_DDR &= ~(1<<WMILL1_DDR);
  WMILL_DDR &= ~(1<<WMILL2_DDR);
  WMILL_DDR &= ~(1<<WMILL3_DDR);

  // Pullups
  WMILL_INTERRUPT_PORT |= (1<<WMILL1_DDR);
  WMILL_INTERRUPT_PORT |= (1<<WMILL2_DDR);
  WMILL_INTERRUPT_PORT |= (1<<WMILL3_DDR);
  
  WMILL_PIN_CHANGE_MASK |=(1<<4); 
  WMILL_PIN_CHANGE_MASK |=(1<<5);
  WMILL_PIN_CHANGE_MASK |=(1<<6);
  EIMSK |=(1<<WMILL_EXT_INT_BIT); // enable bitmask for pcmsk1
}

ISR(PCINT0_vect)
{
    static uint8_t sensor0 = 0;
    static uint8_t sensor1 = 0;
    static uint8_t sensor2 = 0;

    if((WMILL_INTERRUPT_PIN & (1<<WMILL1_PIN))) {
        if(sensor0==0) {
            // rising edge on sensor0. Do work here
            counter0++;
            sensor0 = 1;
        }
    } else {
        if(sensor0==1) {
            // falling edge on sensor0
            sensor0 = 0;
        }
    }

    if((WMILL_INTERRUPT_PIN & (1<<WMILL2_PIN))) {
        if(sensor1==0) {
            counter1++;
            sensor1 = 1;
        }
    } else {
        if(sensor1==1) {
            sensor1 = 0;
        }
    }

    if((WMILL_INTERRUPT_PIN & (1<<WMILL3_PIN))) {
        if(sensor2==0) {
            counter2++;
            sensor2 = 1;
        }
    } else {
        if(sensor2==1) {
            sensor2 = 0;
        }
    }

}

