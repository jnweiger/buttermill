#include <inttypes.h>
// Define what pins the anemometers are connected to
/**
According to http://is.gd/f284E and the 169 handbook, it is 
possible to use the USI port to capture pin change interrupts
coming from the 3 anemometers. The relevant ports are
PE4, PE5, PE6 on USI. The remaining USI port in GND. The pin
change interrupts corresponding to the above are

PE4 -> PCINT4
PE5 -> PCINT5
PE6 -> PCINT6

The pin change mask register for PORTE/USI is PCMSK0
**/

#define WMILL_INTERRUPT_PIN PINE
#define WMILL1_PORT PORTE4
#define WMILL2_PORT PORTE5
#define WMILL3_PORT PORTE6

#define WMILL1_PIN PINE4
#define WMILL2_PIN PINE5
#define WMILL3_PIN PINE6

#define WMILL_DDR  DDRE
#define WMILL1_DDR DDE4
#define WMILL2_DDR DDE5
#define WMILL3_DDR DDE6
#define WMILL_INTERRUPT_PORT PORTE

#define WMILL_PIN_CHANGE_MASK PCMSK0 // the pin change mask register
#define WMILL_EXT_INT_MASK PCIE0
#define WMILL_EXT_INT_BIT 6 // the bit in the EIMSK register that has to be on for USI pin change interrupt

/** Set these up as volatile as they are changed by interrupts **/
extern volatile uint8_t counter0;
extern volatile uint8_t counter1;
extern volatile uint8_t counter2;

extern volatile uint8_t last0;
extern volatile uint8_t last1;
extern volatile uint8_t last2;

extern volatile uint16_t ppi0;
extern volatile uint16_t ppi1;
extern volatile uint16_t ppi2;

// Function declarations
void speed_init(void);
