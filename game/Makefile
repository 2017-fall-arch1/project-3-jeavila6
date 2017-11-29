all: libShape.a libLcd.a libp2sw.a main.elf

CPU             = msp430g2553
CFLAGS          = -mmcu=${CPU} -Os -I../h 
LDFLAGS 	= -L/opt/ti/msp430_gcc/include -L../lib

#switch the compiler (for the internal make rules)
CC              = msp430-elf-gcc
AS              = msp430-elf-as
AR              = msp430-elf-ar

OBJECTS         = p2switches.o shape.o region.o rect.o vec2.o layer.o rarrow.o

libShape.a: $(OBJECTS)
	$(AR) crs $@ $^

libLcd.a: font-11x16.o font-5x7.o font-8x12.o lcdutils.o lcddraw.o
	$(AR) crs $@ $^

lcddraw.o: lcddraw.c lcddraw.h lcdutils.h

lcdutils.o: lcdutils.c lcdutils.h

libp2sw.a: $(OBJECTS)
	$(AR) crs $@ $^

$(OBJECTS): p2switches.h shape.h

install: libLcd.a
	mkdir -p ../h ../lib
	mv $^ ../lib
	cp *.h ../h

clean:
	rm -f libLcd.a *.o *.elf

main.elf: main.o libLcd.a libp2sw.a libShape.a
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lTimer 

load: main.elf
	mspdebug rf2500 "prog $^"
