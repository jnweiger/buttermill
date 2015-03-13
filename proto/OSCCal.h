#ifndef OSCCAL_H
#define OSCCAL_H

   // INCLUDES:
   #include <cpu_mhz.h>
   #include <avr/io.h>
   #include <avr/interrupt.h>

   // CODE DEFINES:
   #define ATOMIC_BLOCK(exitmode)   { exitmode cli();
   #define END_ATOMIC_BLOCK         }
   
   #define ATOMIC_RESTORESTATE      inline void __irestore(uint8_t *s) { SREG = *s; }         \
                                    uint8_t __isave __attribute__((__cleanup__(__irestore))) = SREG;
   #define ATOMIC_ASSUMEON          inline void __irestore(uint8_t *s) { sei(); *s = 0; }     \
                                    uint8_t __isave __attribute__((__cleanup__(__irestore))) = 0;

   // CONFIG DEFINES:
   #define OSCCAL_TARGETCOUNT         (uint16_t)(F_CPU / (32768 / 256)) // (Target Freq / Reference Freq)   
   
   // PROTOTYPES:
   void OSCCAL_Calibrate(void);
   
#endif
