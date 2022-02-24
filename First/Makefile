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
CC = m68k-linux-gnu-gcc -m68010 -O2 -fno-builtin
AS = m68k-linux-gnu-as -m68010
LD = m68k-linux-gnu-ld

OBJS = rom.o first.o

all: rom_image

#copy:	split
#	cp rom_odd rom_even ..
#
#split:	rom_image
#	xsplit_rom rom_image rom_odd rom_even

clean:
	rm -f $(OBJS) rom_image 
	rm -f rom_even rom_odd
	rm -f rom.elf first.s

# Here is the crucial part, linking.
#  the -s switch says to strip the symbol table.
#  note the handy binary image output option!
rom_image:	$(OBJS)
	$(LD) -T rom.lds --oformat binary -s -o rom_image $(OBJS)

rom.elf:	$(OBJS)
	$(LD) -T rom.lds -s -o rom.elf $(OBJS)

rom.o:		rom.s
	$(AS) -o rom.o rom.s

first.o:	first.c
	$(CC) -c first.c

first.s:	first.c
	$(CC) -S first.c

#-------------------

ld68k.lds:
	ld68k --verbose >ld68k.lds

version:
	cc68k --version
	ld68k --version

# THE END
