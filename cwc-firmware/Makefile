# Disassemble the firmware from the ROM images
#
# Tom Trebisky  3-19=2022

all: cwc.dis

.PHONY: cwc.dis

cwc.dis:
	#./8x300_dis -l cwc30.bin cwc58.bin >cwc.dis
	./8x300_dis -l --8x305 cwc58.bin cwc30.bin cwc79.bin >cwc.dis

xx.dis:
	./8x300_dis -l cwc30.bin cwc58.bin >xx.dis

cwc79.odx:
	odx cwc79.bin >cwc79.odx

clean:
	rm -f cwc.dis
