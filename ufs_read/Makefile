# ufs_read - copy files from a UFS disk image

all:	ufs_read

CC = cc -Wno-address-of-packed-member

ufs_read:	ufs_read.c
	$(CC) -o ufs_read ufs_read.c

test:
	(cd root ; rm -rf *)
	(cd usr ; rm -rf *)
	./ufs_read

test2:
	(cd root ; rm -rf *)
	(cd usr ; rm -rf *)
	./ufs_read b

fclean:
	(cd root ; rm -rf *)
	(cd usr ; rm -rf *)

clean:
	rm -f *.o ufs_read

# -------------------------------------

parta:
	dd if=callan.img of=parta.img bs=512 skip=136 count=12376

partb:
	dd if=callan.img of=partb.img bs=512 skip=17408

