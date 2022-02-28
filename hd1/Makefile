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
# We get a 112 byte executable without optimzation
# 84 bytes if we optimize.
# The optimized code runs just fine.
CC = m68k-linux-gnu-gcc -m68000 -O2 -fno-builtin
#CC = m68k-linux-gnu-gcc -m68000 -fno-builtin

AS = m68k-linux-gnu-as -m68000
LD = m68k-linux-gnu-ld
DUMP = m68k-linux-gnu-objdump -d

OBJS = start.o main.o hd.o

all: cwc.img

# Here is the crucial part, linking.
#  the -s switch says to strip the symbol table.
#  note the handy binary image output option!
cwc.img:	$(OBJS)
	$(LD) -T callan.lds --oformat binary -s -o cwc.img $(OBJS)

cwc.elf:	$(OBJS)
	$(LD) -T callan.lds -s -o cwc.elf $(OBJS)

cwc.dump:	cwc.elf
	$(DUMP) cwc.elf >cwc.dump

start.o:		start.s
	$(AS) -o start.o start.s

hd.o:	hd.c
	$(CC) -c hd.c

main.o:	main.c
	$(CC) -c main.c

clean:
	rm -f $(OBJS) cwc.img 
	rm -f *.o *.dump
	rm -f cwc.elf cwc.s

# THE END
