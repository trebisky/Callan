/* merge_rom.c
 * Tom Trebisky  2-17-2022
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_ROM		8*1024

unsigned char im1[MAX_ROM];
unsigned char im2[MAX_ROM];
unsigned char im_out[2*MAX_ROM];

void
error ( char *msg )
{
	fprintf ( stderr, "%s\n", msg );
	_exit ( 1 );
}

int
main ( int argc, char **argv ) 
{
	char *file1, *file2;
	int fd1, fd2;
	int n1, n2;
	int i;
	unsigned char *p;
	int fd;
	int n;

	--argc;
	++argv;

	if ( argc != 2 )
	    error ( "usage" );

	file1 = argv[0];
	file2 = argv[1];

	fd1 = open ( file1, O_RDONLY );
	fd2 = open ( file2, O_RDONLY );
	n1 = read ( fd1, im1, MAX_ROM );
	n2 = read ( fd2, im2, MAX_ROM );
	if ( n1 != n2 )
	    error ( "mismatch" );

	// printf ( " %d %d\n", n1, n2 );

	p = im_out;
	for ( i=0; i<n1; i++ ) {
	    *p++ = im1[i];
	    *p++ = im2[i];
	}

	fd = open ( "merge.out", O_CREAT | O_WRONLY,0664 );
	n = write ( fd, im_out, n1+n2 );
	close ( fd );
	if ( n != n1+n2 )
	    error ( "oops" );

	return 0;
}

/* THE END */
