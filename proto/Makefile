#Makefile for project $(NAME)
# Distribute under GPLv2, use with care.
#
# 2008-09-25, jw
MCU = atmega169
FORMAT = binary
DEVID = 0x1e9405
SWVERSIONMAJOR = 1
SWVERSIONMINOR = 4
BOOTLDRSIZE = 0x800
CFLAGS += -DDEVID=$(DEVID)
CFLAGS += -DSWVERSIONMAJOR=$(SWVERSIONMAJOR)
CFLAGS += -DSWVERSIONMINOR=$(SWVERSIONMINOR)

#FLASHSIZE - BOOTLDRSIZE - 8
BOOTLDRINFOSTART = 0x37F8 # i.e. bootloader start (0x3800) minus 8 bytes
NAME          = main
CFILES        = main.c speed.c led.c rtc.c file.c fat.c mmc.c usart.c LCD_functions.c LCD_driver.c BCD.c ADC.c #OSCCal.c 
CPU           = mega169
PROG_HW       = usbtiny
PROG_SW       = avrdude

TOP_DIR       = .


include $(TOP_DIR)/avr_common.mk
#LDFLAGS =  -Wl,-u,vfprintf -lprintf_flt -lm
O2HEX_T         = $(O2HEX) -j .text -j .data -j .bootldrinfo
LDFLAGS	+= -Wl,--section-start=.bootldrinfo=$(BOOTLDRINFOSTART)
CFLAGS += -DDEVID=$(DEVID)
CFLAGS += -DSWVERSIONMAJOR=$(SWVERSIONMAJOR)
CFLAGS += -DSWVERSIONMINOR=$(SWVERSIONMINOR)
CFLAGS += -std=gnu99
LDFLAGS += -lm
distclean:: 
	rm -f download* ee_data.* 

## header file dependencies
#############################
include depend.mk
