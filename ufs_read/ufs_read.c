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
 * OK, so let's work up a limit value for block B.
 * Cylinder 306 * 136 = 41616
 * Subtract the block B offset  41616 - 17408 = 24208
 *
 * And after adding this check to the code, I am happy to
 * confirm that no files have blocks in the bad area!
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
#include <string.h>
#include <time.h>

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
#define LIMIT_A		20000

#define OFFSET_B	17408
#define SIZE_B		26112
#define LIMIT_B		24208

void error ( char * );
void dump_fs ( int, int, int );

/* ----------------------------- */

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
static int limit;

void
disk_offset ( int a_off, int a_size, int a_limit )
{
    offset = a_off;
    size = a_size;
    limit = a_limit;
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

    printf ( "Block %d (%08x) ------------ -----------\n", block, block );
    disk_read ( buf, block );
    block_dump ( buf, BSIZE );
}

/* ----------------------------- */

/* From:
 *  include/sys/filsys.h
 *  include/sys/param.h
 *  include/sys/types.h
 *  include/sys/ino.h

 */

/* ino_t conflicts with linux headers */
typedef	unsigned short	xino_t;     	/* i-node number */
typedef	unsigned int	xoff_t;
typedef	unsigned int	xtime_t;

/* These are in core "caches" not relevant to us.
 * but they do take up space in the super block
 */
#define NICFREE 50
#define NICINOD 100             /* number of superblock inodes */

/* Don't ask me what inode 1 is for.
 * No doubt 0 is reserved for "nobody home here"
 */
#define ROOT_INO	2

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
    u_int	stuff;
    xtime_t	time;
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

#define INODES_PER_BLOCK	(BSIZE / sizeof (struct dinode))

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

struct dinode iblock[INODES_PER_BLOCK];

/* I nodes begin at block 2 in the filesystem.
 */
#define INODE_OFFSET	2

struct super sb;

/* These were the days!  14 byte filename limits.
 *  note that there is no guarantee of a null terminator.
 * So directory entries were 16 bytes
 *  and you could get 32 of them in a block
 */
#define DIRSIZ  14
struct  direct
{
        xino_t   d_ino;
        char    d_name[DIRSIZ];
};

#define DIRECT_PER_BLOCK	(BSIZE / sizeof(struct direct))

/* These two work in place */

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

/* These two return the fixed value */

int
f_ifix ( u_int val )
{
    return htonl ( val );
}

int
f_sfix ( u_short val )
{
    return htons ( val );
}


/* ------------------------------------------------------- */
/* ------------------------------------------------------- */
/* ------------------------------------------------------- */

/* The only disk inode stores disk addresses in 3 bytes */
#define NUM_INODE_ADDR		13
#define BYTES_INODE_ADDR	(NUM_INODE_ADDR * 3)

/* We scanned the filesystem and found the biggest file is
 * on the second filesystem.
 * It is ./tmp/floppy_image of size 630784 bytes.
 * This requires 1232 blocks.
 * When I "fix" the inode (convert the disk data structure
 * to an in-memory data structure).  I expand the indirect
 * and double indirect blocks, so that I have just
 * a simple linear list.
 * No files exist on this disk that require a triple
 * indirect block.
 */

#define MAX_INODE_ADDR		1600

struct mem_inode
{
        int   mode;        /* mode and type of file */
        int   nlink;       /* number of links to file */
        int   uid;         /* owner's user id */
        int   gid;         /* owner's group id */
        int   size;        /* number of bytes in file */
        // u_int   addr[NUM_INODE_ADDR];
        u_int   addr[MAX_INODE_ADDR];
	int bcount;
        time_t  atime;       /* time last accessed */
        time_t  mtime;       /* time last modified */
        time_t  ctime;       /* time created */
};

/* While the disk addresses within the inode use 3 bytes each,
 * the addresses in indirect blocks are 4 byte objects.
 */
void
fix_addr_block ( u_char *buf )
{
	u_int *ip;
	u_int *ep;

	ip = (u_int *) buf;
	ep = (u_int *) &buf[BSIZE];
	for ( ; ip<ep; ip++ )
	    if ( *ip )
		ifix(ip);
}

void
expand_indir ( int block, u_int *addr, int *count, int level )
{
	u_char buf[BSIZE];
	u_int *ip;
	u_int *ep;

	disk_read ( buf, block );
	fix_addr_block ( buf );

	if ( level == 2 ) {
	    ip = (u_int *) buf;
	    ep = (u_int *) &buf[BSIZE];
	    for ( ; ip<ep; ip++ )
		if ( *ip )
		    expand_indir ( *ip, addr, count, 1 );
	} else {
	    ip = (u_int *) buf;
	    ep = (u_int *) &buf[BSIZE];
	    for ( ; ip<ep; ip++ )
		if ( *ip ) {
		    addr[*count] = *ip;
		    (*count)++;
		}
	}
}

void
fix_inode ( struct mem_inode *mp, struct dinode *dp, char *path )
{
	u_char *u;
	u_int ind_block = 0;
	u_int dind_block = 0;
	u_int tind_block = 0;
	u_int addr;
	int bcount;
	int i;

	// printf ( "Start fix inode\n" );

	mp->mode = f_sfix ( dp->di_mode );
	mp->nlink = f_sfix ( dp->di_nlink );
	mp->uid = f_sfix ( dp->di_uid );
	mp->gid = f_sfix ( dp->di_gid );
	mp->size = f_ifix ( dp->di_size );

	mp->atime = dp->di_atime;
	mp->mtime = dp->di_mtime;
	mp->ctime = dp->di_ctime;

	/* Only directories and regular files have block lists.
	 * Pipes don't exist in the filesystem.
	 */
	if ( ((mp->mode & IFMT) != IFDIR ) &&
	    ((mp->mode & IFMT) != IFREG ) )
		return;

	bcount = 0;
	for ( i=0; i<BYTES_INODE_ADDR; i += 3 ) {
	    u = &dp->di_addr[i];
	    addr = u[0]<<16 | u[1]<<8 | u[2];
	    if ( addr )
		mp->addr[bcount++] = addr;
	}

	if ( bcount > 10 )
	    ind_block = mp->addr[10];
	if ( bcount > 11 )
	    dind_block = mp->addr[11];
	if ( bcount > 12 )
	    tind_block = mp->addr[12];

	if ( tind_block ) {
	    printf ( "Triple indirect block -- we cannot handle these yet\n" );
	    error ( "Triple indirect block -- we cannot handle these yet\n" );
	}

	/* Now, forget any indirect blocks */
	if ( bcount > 10 )
	    bcount = 10;

	if ( ind_block )
	    expand_indir (  ind_block, mp->addr, &bcount, 1 );
	if ( dind_block )
	    expand_indir (  dind_block, mp->addr, &bcount, 2 );

	mp->bcount = bcount;

	/*
	printf ( "Block addresses for %s\n", path );
	for ( i=0; i<bcount; i++ ) {
	    printf ( "%5d: %9d\n", i, mp->addr[i] );
	}
	*/

	/* Now scan for blocks in the bad region */
	if ( ! path )
	    return;

	for ( i=0; i<bcount; i++ ) {
	    // printf ( "Check %d %d \n", mp->addr[i], limit );
	    if ( mp->addr[i] > limit )
		printf ( "BAD BLOCK: %d in %s\n", mp->addr[i], path );
	}

	// printf ( "end fix inode\n" );
}

void
get_inode ( struct mem_inode *mp, int inode, char *path )
{
    int block;
    int index;

    /* It is surprising, but indeed, inode 0 is not stored in the list.
     * So this subtraction is essential.
     */
    block = INODE_OFFSET + (inode-1) / INODES_PER_BLOCK;
    index = (inode-1) % INODES_PER_BLOCK;

    // printf ( "Fetching inode %d -- block %d, index %d\n", inode, block, index );
    // block_show ( block );

    disk_read ( (u_char *) iblock, block );
    fix_inode ( mp, &iblock[index], path );

    // printf ( "IPB = %d\n", INODES_PER_BLOCK );
}

/* A close brother to get_inode, (above)
 */
void
show_inode ( int inode )
{
    int block;
    int index;
    u_char buf[BSIZE];
    struct mem_inode mi;
    struct dinode *dp;
    int addr;
    int i;

    block = INODE_OFFSET + (inode-1) / INODES_PER_BLOCK;
    index = (inode-1) % INODES_PER_BLOCK;

    printf ( "Show inode %d -- block %d, index %d\n", inode, block, index );
    // block_show ( block );

    disk_read ( buf, block );
    dp = (struct dinode *) buf;
    block_dump ( (u_char *) &dp[index], sizeof(struct dinode) );

    fix_inode ( &mi, &dp[index], NULL );
    for ( i=0; i<NUM_INODE_ADDR; i++ ) {
	addr = mi.addr[i];
	// if ( addr )
	    printf ( "Addr: %d = %d\n", i, addr );
    }
}

struct mem_direct {
    int		inode;
    char	name[16];
};

void
fix_direct ( struct mem_direct *dp, struct direct *diskp )
{
	dp->inode = f_sfix ( diskp->d_ino );
	strncpy ( dp->name, diskp->d_name, 14 );
	dp->name[14] = 0;
}

/* This stops on the first entry with a zero i-node,
 * which I think (hope) is correct.
 * dp - the directory entry that is fetched.
 * mp - the inode of the directory being scanned.
 * entry - the index of the directory entry that is wanted.
 *
 * This does read a disk block for every directory entry.
 *  It is a sad world sometimes.
 */
int
get_dir_entry ( struct mem_direct *dp, struct mem_inode *mp, int entry )
{
    int index;
    int bindex;
    u_char dir_block[BSIZE];
    struct direct *dbp;

    index = entry % DIRECT_PER_BLOCK;
    bindex = entry / DIRECT_PER_BLOCK;
    // printf ( "index %d of %d\n", index, DIRECT_PER_BLOCK );

    if ( bindex >= 10 )
	error ( "whoaa -- dir needs an indirect block" );

    /* We are done when we hit a zero block pointer.
     * (or when we run out of block pointers).
     * See above for running out of block pointers
     * XXX - someday we may have a big list due to
     * indirect blocks and will need to know how big
     * that list is.
     */
    if ( ! mp->addr[bindex] )
	return 0;

    // printf ( "Read block %d (%d) for directory entries\n", mp->addr[bindex], bindex );

    disk_read ( dir_block, mp->addr[bindex] );
    dbp = (struct direct *) dir_block;
    fix_direct ( dp, &dbp[index] );

#ifdef WRONG
    /* It is wrong to stop on the first zero inode.
     * The unlink call simply clears the inode value
     * in the directory and valid entries may follow.
     * So we should return the zero entries and the
     * caller must ignore them.
     */
    if ( ! dp->inode )
	return 0;
#endif

    return 1;
}

/* Inode 1 looks like this (whatever it is)
00000400 8000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000
00000420 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 18d3 f718 18d3 f718 18d3 f718

 * and Inode 2 (the root directory)
00000440 41ff 000d 0000 0000 0000 0210 0001 f100 2470 0000 0000 0000 0000 0000 0000 0000
00000460 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 2763 1c52 2753 223c 2753 223c
 */

#ifdef notdef
int
count_addr_block ( int block, int level )
{
	u_char buf[BSIZE];
	u_int *ip;
	u_int *ep;
	int n;

	disk_read ( buf, block );
	fix_addr_block ( buf );

	if ( level == 2 ) {
	    n = 0;
	    ip = (u_int *) buf;
	    ep = (u_int *) &buf[BSIZE];
	    for ( ; ip<ep; ip++ )
		if ( *ip )
		    n += count_addr_block ( *ip, 1 );
	} else {
	    n = 0;
	    ip = (u_int *) buf;
	    ep = (u_int *) &buf[BSIZE];
	    for ( ; ip<ep; ip++ )
		if ( *ip )
		    n++;
	}

	return n;
}

/* This worked nicely when we had the original 13 inode blocks,
 * but it certainly doesn't work now that we have the entire
 * list neatly expanded.
 */
void
copy_file ( struct mem_inode *ip, char *path )
{
	u_char buf[BSIZE];
	int i;
	u_int *bp;
	int n;

	/* We certainly see these */
	if ( ip->addr[10] ) {
	    n = count_addr_block ( ip->addr[10], 1 );
	    // // printf ( "%20s (%8d) %4d:  indirect\n", path, ip->size, n );
	    // block_show ( ip->addr[10] );
	}

	/* and also these */
	if ( ip->addr[11] ) {
	    n = count_addr_block ( ip->addr[11], 2 );
	    // // printf ( "%20s (%8d) %4d: Dindirect\n", path, ip->size, n );
	    /*
	    block_show ( ip->addr[11] );
	    disk_read ( buf, ip->addr[11] );
	    fix_addr_block ( buf );
	    bp = (u_int *) buf;
	    for ( i=0; i<(BSIZE/sizeof(u_int)); i++ ) {
		if ( *bp )
		    block_show ( *bp );
		bp++;
	    }
	    */
	    // exit ( 99 );
	}

	/* But none of these */
	if ( ip->addr[12] )
	    printf ( "%20s (%8d): Tindirect\n", path, ip->size );
}
#endif
void
copy_file ( struct mem_inode *ip, char *path )
{
}

int biggest_size = 0;
char biggest_path[256];

void
walk_dir ( int inode, char *path )
{
    struct mem_inode m_inode;
    struct mem_inode entry_inode;
    struct mem_direct m_dir;
    int entry;
    int code;
    char ent_path[256];

    // printf ( "Start walk for inode %d\n", inode );

    // show_inode ( inode );

    // get_inode ( &m_inode, inode, NULL );
    get_inode ( &m_inode, inode, path );

    if ( (m_inode.mode & IFMT) != IFDIR )
	error ( "oops - not a directory" );

    /* First pass,
     * - list full contents
     * - process "leaf" nodes.
     */
    for ( entry = 0; get_dir_entry ( &m_dir, &m_inode, entry ); entry++ ) {
	if ( ! m_dir.inode )
	    continue;

	strcpy ( ent_path, path );
	strcat ( ent_path, "/" );
	strcat ( ent_path, m_dir.name );

	// printf ( "Fetch inode %d for %s\n", m_dir.inode, m_dir.name );
	// show_inode ( m_dir.inode );
	get_inode ( &entry_inode, m_dir.inode, ent_path );

	// printf ( "mem inode mode = %08x\n", entry_inode.mode );
	code = ((entry_inode.mode & IFMT) == IFDIR) ? 'D' : 'R';

	printf ( "%5d %c (%d %d) %s\n", m_dir.inode, code,
	    entry_inode.nlink, entry_inode.size, m_dir.name );

#ifdef notdef
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
#endif
	if ( (entry_inode.mode & IFMT) == IFDIR ) {
	    /* make the directory */
	    if ( entry_inode.size > biggest_size ) {
		biggest_size = entry_inode.size;
		strcpy ( biggest_path, ent_path );
	    }
	    if ( entry_inode.nlink > 1 )
		printf ( "DLINK: %d %s\n", entry_inode.nlink, ent_path );
	} else if ( (entry_inode.mode & IFMT) == IFREG ) {
	    /* regular file */
	    copy_file ( &entry_inode, ent_path );
	    if ( entry_inode.size > biggest_size ) {
		biggest_size = entry_inode.size;
		strcpy ( biggest_path, ent_path );
	    }
	    if ( entry_inode.nlink > 1 )
		printf ( "FLINK: %d %s\n", entry_inode.nlink, ent_path );
	} else {
	    /* some kind of special file */
	    printf ( "SPECIAL: %s\n", ent_path );
	    if ( entry_inode.nlink > 1 )
		printf ( "SLINK: %d %s\n", entry_inode.nlink, ent_path );
	}
    }

    // printf ( "Start walk pass 2 for inode %d\n", inode );

    /* Second pass, recurse into subdirectories.
     * Avoid "." and ".."
     */
    for ( entry = 0; get_dir_entry ( &m_dir, &m_inode, entry ); entry++ ) {
	if ( ! m_dir.inode )
	    continue;

	strcpy ( ent_path, path );
	strcat ( ent_path, "/" );
	strcat ( ent_path, m_dir.name );

	if ( strcmp ( m_dir.name, "." ) == 0 )
	    continue;
	if ( strcmp ( m_dir.name, ".." ) == 0 )
		continue;

	// get_inode ( &entry_inode, m_dir.inode, NULL );
	get_inode ( &entry_inode, m_dir.inode, ent_path );

	if ( (entry_inode.mode & IFMT) == IFDIR ) {
	    printf ( "Enter: %s\n", ent_path );
	    walk_dir ( m_dir.inode, ent_path );
	    printf ( "Done: %s\n", ent_path );
	}
    }

    // printf ( "Walk done for inode %d\n", inode );
}

/* Start at the root and this should recurse through the entire filesystem
 */
void
walk_it ( void )
{
    walk_dir ( ROOT_INO, "." );

    printf ( "Biggest file: %d  %s\n", biggest_size, biggest_path );
}

static void
encode_time ( char *str, xtime_t tt )
{
    // time_t current_time;
    struct tm * time_info;
    // char timeString[9];  // space for "HH:MM:SS\0"
    time_t big_time;

    // time(&current_time);
    // time_info = localtime(&current_time);

    big_time = tt;
    time_info = localtime(&big_time);

    // strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);
    strftime ( str, 32, "%x %H:%M:%S", time_info);
    // puts(timeString);
}

void
read_super ( void )
{
    int tmp;
    char time_str[32];

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
    ifix ( &sb.time );
    encode_time ( time_str, sb.time );

    printf ( "\n" );
    printf ( "Filesystem isize: %d\n", sb.isize );	/* in blocks */
    printf ( "Filesystem size: %d\n", sb.fsize );
    printf ( "Filesystem nfree: %d\n", sb.nfree );
    printf ( "Filesystem ninode: %d\n", sb.ninode );
    printf ( "Filesystem time: %d %s\n", sb.time, time_str );
    printf ( "\n" );
    printf ( "Filesystem inode size: %d\n", sizeof(struct dinode) );
}


void
dump_fs ( int offset, int size, int limit )
{
    disk_offset ( offset, size, limit );
    read_super ();

    printf ( "Start Filesystem **************************\n" );
    walk_it ();
}

int
main ( int argc, char **argv )
{
    int first = 1;
    char *p;

    argc--;
    argv++;

    if ( argc > 1 ) {
	p = argv[0];
	if ( *p == 'b' || *p == 'B' )
	    first = 0;
    }

    disk_fd = open ( disk_path, O_RDONLY );
    if ( disk_fd < 0 )
	error ( "could not open disk image" );

    if ( first )
	dump_fs ( OFFSET_A, SIZE_A, LIMIT_A );
    else
	dump_fs ( OFFSET_B, SIZE_B, LIMIT_B );

    return 0;
}


#ifdef OLD_STUFF
XX/* The following is code I wrote when I was first starting this project.
XX * Although it is no longer used, it was a stepping stone to the
XX * code above.
XX */
XXvoid
XXdump_inode ( struct dinode *ip )
XX{
XX    int i;
XX    u_int addr;
XX    u_char *u;
XX
XX    sfix ( &ip->di_mode );
XX    if ( (ip->di_mode & IFMT) == IFDIR )
XX	printf ( " Mode: %04x - directory\n", ip->di_mode );
XX    else if ( (ip->di_mode & IFMT) == IFREG )
XX	printf ( " Mode: %04x - reg file\n", ip->di_mode );
XX    else
XX	printf ( " Mode: %04x - ?\n", ip->di_mode );
XX    ifix ( &ip->di_size );
XX    printf ( " Size: %d\n", ip->di_size );
XX
XX    for ( i=0; i<39; i += 3 ) {
XX	u = &ip->di_addr[i];
XX	addr = u[0]<<16 | u[1]<<8 | u[2];
XX	if ( addr )
XX	    printf ( "Block: %d\n", addr );
XX    }
XX}
XX
XXvoid
XXdump_iblock ( int block )
XX{
XX    int i;
XX
XX    disk_read ( (u_char *) iblock, block );
XX    for ( i=0; i<INODES_PER_BLOCK; i++ ) {
XX	printf ( "I-node %d\n", i );
XX	dump_inode ( &iblock[i] );
XX    }
XX}
XX
XXvoid
XXdir_show ( int block )
XX{
XX    u_char buf[BSIZE];
XX    struct direct *dp;
XX    int nd;
XX    int i;
XX
XX    nd = BSIZE/sizeof(struct direct);
XX
XX    disk_read ( buf, block );
XX    dp = (struct direct *) buf;
XX
XX    for ( i=0; i<nd; i++ ) {
XX	sfix ( &dp->d_ino );
XX	printf ( "%5d: %s\n", dp->d_ino, dp->d_name );
XX	dp++;
XX    }
XX}
XX
XX/* Inode 1 is the root directory.
XX * ( inode 0 is an unused bad block file)
XX */
XXvoid
XXdump_root ( void )
XX{
XX    int i;
XX    struct dinode *root;
XX    u_int addr;
XX    u_char *u;
XX
XX    disk_read ( (u_char *) iblock, 2 );
XX    root = &iblock[1];
XX    dump_inode ( root );
XX
XX    for ( i=0; i<39; i += 3 ) {
XX	u = &root->di_addr[i];
XX	addr = u[0]<<16 | u[1]<<8 | u[2];
XX	if ( addr ) {
XX	    printf ( "Block: %d\n", addr );
XX	    block_show ( addr );
XX	    dir_show ( addr );
XX	}
XX    }
XX}
XX
XXvoid
XXdump_it ( void )
XX{
XX    /* Block 2 is the start of inodes */
XX    block_show ( 2 );
XX    // dump_iblock ( 2 );
XX    dump_root ();
XX}
XX
#endif /* OLD_STUFF */

/* THE END */
