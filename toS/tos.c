/* tos.c
 *
 * Convert a binary file to S-records
 *
 * Tom Trebisky 2-27-2022
 *
 * Uses srecord.c and srecord.h verbatim from:
 *  https://github.com/vsergeev/libGIS.git
 *
 * This is just a front-end to those routines.
 *
 * S record types:
 *  0 - header type (just forget this).
 *  2 - data record with 3 byte address
 *  8 - trailer with 3 byte address.
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "srecord.h"

unsigned int base_addr = 0x1000;

void
error ( char *msg )
{
    fprintf ( stderr, "%s\n", msg );
    exit ( 1 );
}

#define MAX_IMAGE	64*1024

unsigned char image[MAX_IMAGE+2];
int n_image;

void
read_image ( char *path )
{
    int fd;
    int n;

    fd = open ( path, O_RDONLY );
    if ( fd < 0 )
	error ( "Cannot open file" );

    n = read ( fd, image, MAX_IMAGE + 2);
    if ( n > MAX_IMAGE )
	error ( "Image too big" );
    if ( n < 2 )
	error ( "Image empty" );

    printf ( "Image read, %d bytes\n", n );
    n_image = n;

    close ( fd );
}

void
emit_srec ( FILE *out_file, unsigned int addr, unsigned char *buf, int n )
{
    SRecord srec;
    int s;

    s = New_SRecord ( SRECORD_TYPE_S2, addr, buf, n, &srec );
    if ( s != SRECORD_OK )
	error ( "Fail to make new S record" );

    s = Write_SRecord ( &srec, out_file );
    if ( s != SRECORD_OK )
	error ( "Fail to write data S record" );
}

void
emit_trailer ( FILE *out_file, unsigned int addr )
{
    SRecord srec;
    int s;

    // s = New_SRecord ( SRECORD_TYPE_S8, base_addr, image, n_image, &srec );
    s = New_SRecord ( SRECORD_TYPE_S8, base_addr, image, 0, &srec );
    if ( s != SRECORD_OK )
	error ( "Fail to make trailer S record" );
    s = Write_SRecord ( &srec, out_file );
    if ( s != SRECORD_OK )
	error ( "Fail to write trailer S record" );
}

#define CHUNK	32

void
gen_srec ( FILE *out_file )
{
    unsigned int addr;
    int nio;
    int nout;

    addr = base_addr;
    nout = 0;

    while ( nout < n_image ) {
	nio = n_image - nout;
	if ( nio > CHUNK ) nio = CHUNK;
	printf ( "Write: %d\n", nio );
	emit_srec ( out_file, addr, &image[nout], nio );
	addr += nio;
	nout += nio;
    }

    emit_trailer ( out_file, addr );
}

int
main ( int argc, char **argv )
{
    FILE *sfile;

    argc--;
    argv++;

    if ( argc != 1 )
	error ( "usage: tos file" );

    read_image ( argv[0] );

    sfile = fopen ( "test.srec", "w" );
    gen_srec ( sfile );
    fclose ( sfile );
	    
    return 0;
}

/* THE END */
