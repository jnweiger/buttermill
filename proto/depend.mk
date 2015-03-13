main.o: main.c cpu_mhz.h rtc.h speed.h led.h main.h LCD_driver.h \
  LCD_functions.h ADC.h config.h file.h fat.h mmc.h
speed.o: speed.c cpu_mhz.h speed.h led.h
led.o: led.c cpu_mhz.h led.h
rtc.o: rtc.c speed.h OSCCal.h cpu_mhz.h
file.o: file.c cpu_mhz.h config.h mmc.h fat.h file.h led.h
fat.o: fat.c cpu_mhz.h config.h fat.h file.h mmc.h led.h
mmc.o: mmc.c config.h mmc.h led.h
usart.o: usart.c cpu_mhz.h usart.h
LCD_functions.o: LCD_functions.c LCD_driver.h LCD_functions.h BCD.h \
  main.h
LCD_driver.o: LCD_driver.c main.h LCD_driver.h
BCD.o: BCD.c
ADC.o: ADC.c pgmspacehlp.h main.h ADC.h BCD.h
