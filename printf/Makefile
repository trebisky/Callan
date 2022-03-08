# Try to use our vxworks cross tools to build
# Code to run on the Callan
#
# Tom Trebisky 2-27-2022
#
# This is my old vxworks cross compiler
#  It is gcc 2.8.1 along with (probably) binutils 2.9.1
#COPTS = -m68000 -O2
#CC = cc68k -m68000 -O2
#AS = as68k -m68010
#LD = ld68k
# Note that the compiler does NOT support the -m68010 (-mc68010)
# option, only 68000/68020/68030/68040 ....

# This is from a Fedora package for F34 as of 2-2022
# It does have the m68010 compiler switch
# However, the chip in the Callan is a 68000

# The optimized code runs just fine.
#CC = m68k-linux-gnu-gcc -m68000 -O2 -fno-builtin
CC = m68k-linux-gnu-gcc -m68000 -fno-builtin -g -ffreestanding

AS = m68k-linux-gnu-as -m68000
LD = m68k-linux-gnu-ld
DUMP = m68k-linux-gnu-objdump -d

OBJS = start.o main.o prf.o uart.o

LOPT = -T callan.lds -L .

# My prf.c requires these routines,which are in -lgcc
# undefined reference to `__modsi3'
# undefined reference to `__divsi3'
LIBS = -lm68k

.c.o:
	$(CC) $< -c

.S.o:
	$(AS) $< -o $@

#	$(AS) -o start.o start.S

all: callan.img

getlib:
	cp ../libgcc/libm68k.a .

# Address should match what is in callan.lds
test: callan.img
	sloader callan.img 0x10000

# As of 3-1-2022, I am using gcc 11.2.1
version:
	$(CC) --version

# Here is the crucial part, linking.
#  the -s switch says to strip the symbol table.
#  note the handy binary image output option!
callan.img:	$(OBJS)
	$(LD) $(LOPT) --oformat binary -s -o callan.img $(OBJS) $(LIBS)

callan.elf:	$(OBJS)
	$(LD) $(LOPT) -s -o callan.elf $(OBJS) $(LIBS)

callan.dump:	callan.elf
	$(DUMP) callan.elf >callan.dump

clean:
	rm -f $(OBJS) callan.img 
	rm -f *.o *.dump
	rm -f callan.elf callan.s

# THE END
