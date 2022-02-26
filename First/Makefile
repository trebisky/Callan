# Try to use our vxworks cross tools to build a rom
# image for the heurikon HK68/ME
#
# Tom Trebisky 10-30-2006
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
#CC = m68k-linux-gnu-gcc -m68010 -O2 -fno-builtin
CC = m68k-linux-gnu-gcc -m68000 -fno-builtin
AS = m68k-linux-gnu-as -m68000
LD = m68k-linux-gnu-ld
DUMP = m68k-linux-gnu-objdump -d

OBJS = rom.o first.o

all: first.img

# Here is the crucial part, linking.
#  the -s switch says to strip the symbol table.
#  note the handy binary image output option!
first.img:	$(OBJS)
	$(LD) -T rom.lds --oformat binary -s -o first.img $(OBJS)

first.elf:	$(OBJS)
	$(LD) -T rom.lds -s -o first.elf $(OBJS)

first.dump:	first.elf
	$(DUMP) first.elf >first.dump

rom.o:		rom.s
	$(AS) -o rom.o rom.s

first.o:	first.c
	$(CC) -c first.c

first.s:	first.c
	$(CC) -S first.c

clean:
	rm -f $(OBJS) first.img 
	rm -f rom_even rom_odd
	rm -f first.elf first.s

# THE END
