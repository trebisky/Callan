# run mfm_util on Callan transition files

all:	mfm_dump

mfm_dump:	mfm_dump.c
	cc -o mfm_dump mfm_dump.c

test:
	./mfm_dump

# ------------

# You won't be able to do this, but this is what I did on the BBB
# on the Gesswein MFM emulator to obtain the transitions file.
read:
	./mfm_read --transitions_file ../callan_raw1 --drive 1 --heads 8 --cylinders 320
