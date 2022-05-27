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
#include <unistd.h>
#include <fcntl.h>

//#include <endian.h>
#include <arpa/inet.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

char *disk_path = "callan.img";

int disk_fd;

#define BSIZE		512

#define OFFSET_A	136
#define SIZE_A		12376
#define OFFSET_B	17408
#define SIZE_B		26112

void error ( char * );
void dump_fs ( int, int );

/* ----------------------------- */

int
main ( int argc, char **argv )
{
    disk_fd = open ( disk_path, O_RDONLY );
    if ( disk_fd < 0 )
	error ( "could not open disk image" );

    dump_fs ( OFFSET_A, SIZE_A );

    return 0;
}

void
error ( char *msg )
{
    fprintf ( stderr, "Error: %s\n", msg );
    exit ( 1 );
}


/* ----------------------------- */
/* block reads from the image */

static int offset;
static int size;

void
disk_offset ( int a_off, int a_size )
{
    offset = a_off;
    size = a_size;
}

void
disk_read ( u_char *buf, int num )
{
    int block;

    block = offset + num;
    lseek ( disk_fd, block*BSIZE, SEEK_SET );
    read ( disk_fd, buf, BSIZE );
}

#define CHOP	32

void
block_dump ( u_char *buf, int n )
{
    int i;

    if ( n > CHOP ) {
	block_dump ( buf, CHOP );
	block_dump ( &buf[CHOP], n-CHOP );
    } else {
	for ( i=0; i<n; i++ )
	    printf ( " %02x", buf[i] );
	printf ( "\n" );
    }
}

void
block_show ( int block )
{
    u_char buf[BSIZE];

    disk_read ( buf, block );
    block_dump ( buf, BSIZE );
}

/* ----------------------------- */

/* From:
 *  sys/filsys.h
 *  sys/param.h
 *  sys/types.h
 *  sys/ino.h
 */

/* ino_t conflicts with linux headers */
typedef	unsigned short	xino_t;     	/* i-node number */
typedef	unsigned int	xoff_t;
typedef	unsigned int	xtime_t;

#define NICFREE 50
#define NICINOD 100             /* number of superblock inodes */

#define ROOTINO ((xino_t)1)

/* Note that in these ancient days, the super block had no magic number.
 * You just had to know where it was.
 */
struct __attribute__((__packed__)) super
{
    u_short	isize;
    u_int	fsize;	/* size in blocks */
    u_short	nfree;
    u_int	free[NICFREE];
    u_short	ninode;
    xino_t	inode[NICINOD];
    char	_extra[512];
};

/* This is 64 bytes, so you can fit 8 of these in a block.
 */
struct __attribute__((__packed__)) dinode
{
        unsigned short  di_mode;        /* mode and type of file */
        short   di_nlink;       /* number of links to file */
        short   di_uid;         /* owner's user id */
        short   di_gid;         /* owner's group id */
        xoff_t   di_size;        /* number of bytes in file */
        u_char    di_addr[40];    /* disk block addresses */
        xtime_t  di_atime;       /* time last accessed */
        xtime_t  di_mtime;       /* time last modified */
        xtime_t  di_ctime;       /* time created */
};

/* We get 13 addresses of 3 bytes each (and an extra byte) */

/* modes - in octal of all things, but I am too lazy to
 * convert to text, and I might make errors.
 */
#define IFMT    0170000         /* type of file */
#define         IFDIR   0040000 /* directory */
#define         IFCHR   0020000 /* character special */
#define         IFBLK   0060000 /* block special */
#define         IFREG   0100000 /* regular */
#define         IFMPC   0030000 /* multiplexed char special */
#define         IFMPB   0070000 /* multiplexed block special */
#define ISUID   04000           /* set user id on execution */
#define ISGID   02000           /* set group id on execution */
#define ISVTX   01000           /* save swapped text even after use */
#define IREAD   0400            /* read, write, execute permissions */
#define IWRITE  0200
#define IEXEC   0100


#define NIBLOCK	8
struct dinode iblock[NIBLOCK];

struct super sb;

/* These were the days!  14 byte filename limits.
 * So directory entries were 16 bytes
 *  and you could get 32 of them in a block
 */
#define DIRSIZ  14
struct  direct
{
        xino_t   d_ino;
        char    d_name[DIRSIZ];
};


void
ifix ( u_int *val )
{
    u_int tmp;

    tmp = *val;
    *val = htonl ( tmp );
}

void
sfix ( u_short *val )
{
    u_short tmp;

    tmp = *val;
    *val = htons ( tmp );
}

void
dump_inode ( struct dinode *ip )
{
    int i;
    u_int addr;
    u_char *u;

    sfix ( &ip->di_mode );
    if ( (ip->di_mode & IFMT) == IFDIR )
	printf ( " Mode: %04x - directory\n", ip->di_mode );
    else if ( (ip->di_mode & IFMT) == IFREG )
	printf ( " Mode: %04x - reg file\n", ip->di_mode );
    else
	printf ( " Mode: %04x - ?\n", ip->di_mode );
    ifix ( &ip->di_size );
    printf ( " Size: %d\n", ip->di_size );

    for ( i=0; i<39; i += 3 ) {
	u = &ip->di_addr[i];
	addr = u[0]<<16 | u[1]<<8 | u[2];
	if ( addr )
	    printf ( "Block: %d\n", addr );
    }
}

void
dump_iblock ( int block )
{
    int i;

    disk_read ( (u_char *) iblock, block );
    for ( i=0; i<NIBLOCK; i++ ) {
	printf ( "I-node %d\n", i );
	dump_inode ( &iblock[i] );
    }
}

void
dir_show ( int block )
{
    u_char buf[BSIZE];
    struct direct *dp;
    int nd;
    int i;

    nd = BSIZE/sizeof(struct direct);

    disk_read ( buf, block );
    dp = (struct direct *) buf;

    for ( i=0; i<nd; i++ ) {
	sfix ( &dp->d_ino );
	printf ( "%5d: %s\n", dp->d_ino, dp->d_name );
	dp++;
    }
}

/* Inode 1 is the root directory.
 * ( inode 0 is an unused bad block file)
 */
void
dump_root ( void )
{
    int i;
    struct dinode *root;
    u_int addr;
    u_char *u;

    disk_read ( (u_char *) iblock, 2 );
    root = &iblock[1];
    dump_inode ( root );

    for ( i=0; i<39; i += 3 ) {
	u = &root->di_addr[i];
	addr = u[0]<<16 | u[1]<<8 | u[2];
	if ( addr ) {
	    printf ( "Block: %d\n", addr );
	    block_show ( addr );
	    dir_show ( addr );
	}
    }
}

void
read_super ( void )
{
    int tmp;

    // block 0 was called the "boot block".
    // This was not used on the Callan, and I find
    // it filled with 0x55.
    // block 1 holds the super block
    // disk_read ( buf, 0 );

    disk_read ( (u_char *) &sb, 1 );

    sfix ( &sb.isize );
    ifix ( &sb.fsize );
    sfix ( &sb.nfree );
    sfix ( &sb.ninode );

    printf ( "\n" );
    printf ( "Filesystem size: %d\n", sb.fsize );
    printf ( "Filesystem nfree: %d\n", sb.nfree );
    printf ( "Filesystem ninode: %d\n", sb.ninode );
    printf ( "\n" );
    printf ( "Filesystem inode size: %d\n", sizeof(struct dinode) );
}

void
dump_fs ( int offset, int size )
{
    disk_offset ( offset, size );
    read_super ();
    block_show ( 2 );
    // dump_iblock ( 2 );
    dump_root ();
}

/* THE END */
