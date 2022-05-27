/* ufs_read.c
 *
 * Tom Trebisky  5-27-2022
 *
 * ufs_read - read/extract files from the Callan disk image.
 *
 * The Callan was an edition7 unix system running on a MC68010
 * The idea with this program is to read (extract) all the files
 * from this image, placing them into a directory in a modern
 * linux system.  There they can be studied, and the entire
 * collection can be gathered into a tar file so as to be
 * archived or shared with others.
 *
 * Some considerations up front:
 *
 * endian - the data on the disk is big endian.
 *  This of course should be preserved on the copied files,
 *  but pointers and such within the unix filesystem will
 *  need to be "fixed" within the program itself.
 *
 * special files - what do we do when we get to the /dev directory.
 *  We will cross that bridge when we get there.
 *  One possibility would be to generate a script/list that
 *  gets placed in the /dev directory for later study.
 *  We really don't need (or even want) the special files themselves.
 *
 * links - what do we do here?
 *  I intend to convert hard links to symbolic links.
 *  The way to do this is to keep an eye open for a link count
 *  that is not "1".  The first time it is encountered, just
 *  copy the file contents like any other, but add an entry to
 *  a table.  This table will be searched for all such subsequent
 *  encounters, and when it finds the contents already have been
 *  copied, it will make the symbolic link.  This means that the
 *  first link encountered will become the "hard name".
 *
 * -----------------------------------------------
 *
 * The Callan disk was a Rodime 204 (nominal 20M capacity) MFM drive.
 * It had 320 cylinders and 8 heads.
 * It had 512 byte sectors, 17 of them per track.
 * We will call 512 byte sectors "blocks" in the program.
 *
 * So a cylinder has 8*17 blocks = 136 blocks per cylinder.
 * So the disk has 320*136 blocks = 43520 blocks per disk.
 *
 * The disk image is 22282240 bytes.
 * This is 43520 blocks, which is just right.
 * However, there was trouble reading the disk beyond cylinder 305.
 * So we could consider that we have 306 good cylinders of data.
 * So good data extends to block 41616 (more or less)
 * Any files that reside in blocks beyond this will need to be
 * considered as suspect.
 *
 * ------------------------
 *
 * Back in those days, disks did not contain partition tables.
 * A partition table was compiled into each disk driver.
 * By good fortune, I have a folder with notes about this system.
 * And my notes include information about the partitions,
 * as follows:
 *
 *  Blocks 0-135	bblk		  1 cylinder, 136 blocks
 *  Blocks 136-12511	partition "a"	 91 cylinders, 12376 blocks
 *  Blocks 12512-17407  swap		 36 cylinders, 4896 blocks
 *  Blocks 17408-43519  partition "b"	192 cylinders, 26112 blocks
 *
 */

#include <stdio.h>
#include <stdlib.h>

char *disk_path = "callan.img";

int disk_fd;

void
error ( char *msg )
{
    fprintf ( stderr, "Error: %s\n", msg );
    exit ( 1 );
}

int
main ( int argc, char **argv )
{
    return 0;
}

/* THE END */
