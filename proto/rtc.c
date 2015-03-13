#include <avr/io.h>
#include <avr/interrupt.h>
#include "speed.h"
#include "OSCCal.h"

volatile uint32_t seconds = 0;
volatile uint8_t silence = 0;

/** Start the internal quartz and set it to generate an interrupt when it overflows 
    every second **/
void rtc_init(void)
{
    cli();
    TIMSK2 &= ~(1<<TOIE2);
    ASSR = (1<<AS2);                                      // select asynchronous operation of Timer2
    TCNT2 = 0;                                            // clear TCNT2A
    TCCR2A = (1<<CS22) | (0<<CS21) | (1<<CS20);           // select precaler: 32.768 kHz / 128 = 1 sec between each overflow
    while((ASSR & 0x01) | (ASSR & 0x04));                 // wait for TCN2UB and TCR2UB to be cleared
    TIFR2 = 0xFF;                                         // clear interrupt-flags
    TIMSK2 |= (1<<TOIE2);                                 // enable Timer2 overflow interrupt
    sei();                                                // enable global interrupt
}

/** This interrupt is triggered by the quartz every second - it is used to increment the
32bit seconds integer in the same manner as a Unix timestamp. It also reads out the
values of the anemomenters (i.e. how many ticks in the last second **/
ISR(TIMER2_OVF_vect)
{
    uint8_t tmp; // force the counter difference to 8 bit.
    silence = 1;

    // channel 1
    tmp = counter0-last0;
    ppi0 = ppi0+tmp;
    last0 = counter0;
    if (tmp) silence = 0;

    // channel 2
    tmp = counter1-last1;
    ppi1 = ppi1+tmp;
    last1 = counter1;
    if (tmp) silence = 0;

    // channel 3
    tmp = counter2-last2;
    ppi2 = ppi2+tmp;
    last2 = counter2;
    if (tmp) silence = 0;

    // Finally, increment the seconds timestamp
    seconds++;
}
