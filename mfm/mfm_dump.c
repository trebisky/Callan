/* mfm_dump.c
 *
 * Tom Trebisky  5-25-2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

char * tran_path = "callan_raw1";

/* ------------------------------ */
void tran_read_all ( char * );
void tran_read_deltas ( char *, int, int, u_short *, int * );

#define TRAN_HEADER_ID_SIZE	8
unsigned char valid_id[] = { 0xee, 0x4d, 0x46, 0x4d, 0x0d, 0x0a, 0x1a, 0x00};

/* There is more in the actual header, but we just read the first part,
 * and seek past things we don't care about to the first track record.
 */
struct tran_header {
    u_char id[TRAN_HEADER_ID_SIZE];
    u_int version;
    u_int fh_size;
    u_int th_size;
    int	cyl;
    int head;
    u_int rate;
};

struct track_header {
    int cyl;
    int head;
    int size;
};

/* On My file anyway, I see a maximum of about  81,500 bytes worth of raw deltas
 *  for a single track.
 */
#define RAW_DELTA_SIZE	85000
#define DELTA_SIZE	85000

u_char raw_deltas[RAW_DELTA_SIZE];
u_short deltas[DELTA_SIZE];
int ndeltas;

/* ------------------------------------------------ */

void
error ( char *msg )
{
    fprintf ( stderr, "%s\n", msg );
    exit ( 1 );
}

int
main ( int argc, char **argv )
{
    // tran_read_all ( tran_path );
    tran_read_deltas ( tran_path, 100, 3, deltas, &ndeltas );

    return 0;
}

void
tran_read_all ( char *path )
{
    struct tran_header hdr;
    struct track_header track_hdr;
    int fd;
    int n;
    off_t pos;

    fd = open ( path, O_RDONLY );

    read ( fd, &hdr, sizeof(hdr) );
    if ( memcmp ( hdr.id, valid_id, sizeof(hdr.id) ) != 0 )
	error ( "Bad file header" );

    // Version: 01020200
    printf ( "Version: %08x\n", hdr.version );
    printf ( "Sample rate: %d\n", hdr.rate );
    printf ( "fh: %d\n", hdr.fh_size );

    pos = hdr.fh_size;

    for ( ;; ) {
	lseek ( fd, pos, SEEK_SET );
	n = read ( fd, &track_hdr, sizeof(track_hdr) );
	if ( n <= 0 )
	    break;
	if ( track_hdr.cyl == -1 &&  track_hdr.head == -1 )
	    break;
	printf ( "Track for %d:%d -- %d bytes\n", track_hdr.cyl, track_hdr.head, track_hdr.size );
	/* Why add 4?  Extra 4 bytes at end of data? */
	pos += track_hdr.size + sizeof(track_hdr) + 4;
    }

    close ( fd );
}

/* My data doesn't have any 2 or 3 bytes counts,
 * so this boils down to a plain copy.
 */
void
unpack_deltas ( u_char *raw, int nraw, u_short *deltas, int *count )
{
    int i;
    int ndx;
    u_int value;

    i = 0;
    ndx = 0;
    while ( i < nraw ) {
	if ( raw[i] == 255) {
	    /* XXX how will this fit into 2 bytes ?? */
	    error ( "three byte value" );
            value = raw[i+1] | ((u_int) raw[i+2] << 8) | ((u_int) raw[i+3] << 16);
            i += 4;
         } else if ( raw[i] == 254 ) {
            value = raw[i+1] | ((u_int) raw[i+2] << 8);
            i += 3;
         } else {
            value = raw[i++];
         }

         if ( ndx >= DELTA_SIZE)
	    error ( "Too many deltas" );
         deltas[ndx++] = value;
      }
     printf ( "Unpacked %d deltas\n", ndx );
     *count = ndx;
}

void
tran_read_deltas ( char *path, int cyl, int head, u_short *deltas, int *ndeltas )
{
    struct tran_header hdr;
    struct track_header track_hdr;
    int fd;
    int n;
    off_t pos;

    fd = open ( path, O_RDONLY );

    read ( fd, &hdr, sizeof(hdr) );
    if ( memcmp ( hdr.id, valid_id, sizeof(hdr.id) ) != 0 )
	error ( "Bad file header" );
    // printf ( "Version: %08x\n", hdr.version );
    // printf ( "Sample rate: %d\n", hdr.rate );
    // printf ( "fh: %d\n", hdr.fh_size );

    pos = hdr.fh_size;

    for ( ;; ) {
	lseek ( fd, pos, SEEK_SET );
	n = read ( fd, &track_hdr, sizeof(track_hdr) );
	if ( n <= 0 )
	    break;
	if ( track_hdr.cyl == -1 &&  track_hdr.head == -1 )
	    break;

	pos += track_hdr.size + sizeof(track_hdr) + 4;
	if ( track_hdr.cyl != cyl ||  track_hdr.head != head )
	    continue;

	printf ( "Track for %d:%d -- %d bytes\n", track_hdr.cyl, track_hdr.head, track_hdr.size );
	read ( fd, raw_deltas, track_hdr.size );
	unpack_deltas ( raw_deltas, track_hdr.size, deltas, ndeltas );
	return;
    }

    close ( fd );

    error ( "Did not find requested track" );
}

/* THE END */
