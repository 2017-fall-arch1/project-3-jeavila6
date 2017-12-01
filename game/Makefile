all: libShape.a libLcd.a libp2sw.a main.elf

CPU             = msp430g2553
CFLAGS          = -mmcu=${CPU} -Os -I../h 
LDFLAGS 	= -L/opt/ti/msp430_gcc/include -L../lib

#switch the compiler (for the internal make rules)
CC              = msp430-elf-gcc
#AS              = msp430-elf-as
AS              = msp430-elf-gcc -mmcu=${CPU} -c
AR              = msp430-elf-ar

OBJECTS         = p2switches.o shape.o region.o rect.o vec2.o layer.o rarrow.o

clean:
	rm -f libLcd.a *.o *.elf

main.elf: ${COMMON_OBJECTS} main.o wdt_handler.o buzzer.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lTimer -lLcd -lShape -lCircle -lp2sw

load: main.elf
	mspdebug rf2500 "prog $^"
