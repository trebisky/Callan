# Try to use our vxworks cross tools to build
# Code to run on the Callan
#
# Tom Trebisky 2-27-2022
#
# The optimized code runs just fine.
#CC = m68k-linux-gnu-gcc -m68000 -O2 -fno-builtin
CC = m68k-linux-gnu-gcc -m68000 -fno-builtin -g -ffreestanding

AS = m68k-linux-gnu-as -m68000
LD = m68k-linux-gnu-ld
AR = m68k-linux-gnu-ar

DUMP = m68k-linux-gnu-objdump -d

.c.o:
	$(CC) $< -c

.S.o:
	$(AS) $< -o $@

all: libm68k.a

#ASMFUNCS = _mulsi3 _udivsi3 _divsi3 _umodsi3 _modsi3 \
#   _double _float _floatex \
#   _eqdf2 _nedf2 _gtdf2 _gedf2 _ltdf2 _ledf2 \
#   _eqsf2 _nesf2 _gtsf2 _gesf2 _ltsf2 _lesf2

ASMFUNCS = _mulsi3 _udivsi3 _divsi3 _umodsi3 _modsi3

libm68k.a:
	cp lb1sf68.S lib.S
	for name in $(ASMFUNCS); \
        do \
          echo $${name}; \
          $(CC) -c -DL$${name} lib.S; \
          if [ $$? -eq 0 ] ; then true; else exit 1; fi; \
          mv lib.o $${name}.o; \
          $(AR) rc libm68k.a $${name}.o; \
          rm -f $${name}.o; \
        done
	rm lib.S

list:
	$(AR) t libm68k.a

get:
	cp /u1/tom/GCC/gcc-4.7.2/libgcc/config/m68k/lb1sf68.S .

# As of 3-1-2022, I am using gcc 11.2.1
version:
	$(CC) --version

clean:
	rm -f *.o
	rm -f *.a

# THE END
