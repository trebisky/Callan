# disassemble Callan boot roms

DUMP = m68k-linux-gnu-objdump -m68000 -d

all:	merge_rom wrap callan13.dis callan24.dis

merge_rom: merge_rom.c
	cc -o merge_rom merge_rom.c

wrap: wrap.c
	cc -o wrap wrap.c

callan13.bin:
	#./merge_rom callan1.bin callan3.bin
	./merge_rom callan3.bin callan1.bin
	mv merge.out callan13.bin

callan24.bin:
	./merge_rom callan4.bin callan2.bin
	mv merge.out callan24.bin

callan13.elf: callan13.bin
	./wrap callan13.bin callan13.elf

callan24.elf: callan24.bin
	./wrap callan24.bin callan24.elf

callan13.dis: callan13.elf
	$(DUMP) callan13.elf >callan13.tmp
	./hexfix callan13.tmp >callan13.dis
	rm callan13.tmp

callan24.dis: callan24.elf
	$(DUMP) callan24.elf >callan24.tmp
	./hexfix callan24.tmp >callan24.dis
	rm callan24.tmp

clean:
	rm -f merge_rom merge.out
	rm -f wrap
	rm -f *.elf
	rm -f callan13.bin
	rm -f callan24.bin
	rm -f *.dis
	rm -f *.tmp
