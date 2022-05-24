/* first.c
 *
 * The first bit of C code that got loaded
 * onto the Callan board and run.
 *  2-25-2022  Tom Trebisky
 *
 * This spits out endless "A" characters.
 * It relies on the bootrom already having
 * initialized the baud rate (9600) and uart.
 */

#include "protos.h"

typedef volatile unsigned short vu_short;
typedef volatile unsigned char vu_char;

static char buf[32];

/* Yields an 0.35 second delay */
void
delay_x ( void )
{
	volatile int count;
	
	count = 50000;
	while ( count-- )
	    ;
}

/* This should be about 1 second */
void
delay_one ( void )
{
	volatile unsigned int count;
	
	count = 142857;
	while ( count-- )
	    ;
}

void
delay_ms ( int ms )
{
	volatile unsigned int count;

	count = 143 * ms;
	
	while ( count-- )
	    ;
}

int bss_rubbish[4];

void
bss_clear ( unsigned int *b1, unsigned int *b2 )
{
	while ( b1 < b2 )
	    *b1++ = 0;
}

#ifdef notdef
static char buffer[512];

void
fill_buffer ( int val )
{
	int i;

	for ( i=0; i<512; i++ )
	    buffer[i] = val;
}
#endif

/* 16 lines of 32 bytes */
void
send_buffer ( char *buf )
{
	int i;
	int lc;

	lc = 0;
	for ( i=0; i<512; i++ ) {
	    printf ( "%x", buf[i] );
	    lc++;
	    if ( lc >= 32 ) {
		printf ( "\n" );
		lc = 0;
	    }
	}
}

int
hextoi ( char *s )
{
        int val = 0;
        int c;

        while ( c = *s++ ) {
            if ( c >= '0' && c <= '9' )
                val = val*16 + (c - '0');
            else if ( c >= 'a' && c <= 'f' )
                val = val*16 + 10 + (c - 'a');
            else if ( c >= 'A' && c <= 'F' )
                val = val*16 + 10 + (c - 'A');
            else
                break;
        }

        return val;
}

int
atoi ( char *s )
{
        int val = 0;

        if ( s[0] == '0' && s[1] == 'x' )
            return hextoi ( &s[2] );

        while ( *s >= '0' && *s <= '9' )
            val = val*10 + (*s++ - '0');

        return val;
}

int
strlen ( char *s )
{
	int rv = 0;

	while ( *s++ )
	    rv++;

	return rv;
}

/* split a string in place.
 * tosses nulls into string, trashing it.
 */
int
split ( char *buf, char **bufp, int max )
{
        int i;
        char *p;

        p = buf;
        for ( i=0; i<max; ) {
            while ( *p && *p == ' ' )
                p++;
            if ( ! *p )
                break;
            bufp[i++] = p;
            while ( *p && *p != ' ' )
                p++;
            if ( ! *p )
                break;
            *p++ = '\0';
        }

        return i;
}

/* A command should look like "r 13 7 7"
 * where the numbers are c h s (cyl, head, sec)
 */
void
run_command ( char *cmd )
{
	char *wp[8];
	int nw;
	int c, h, s;
	int status;
	char *buf;

	// printf ( "\n" );
	// printf ( "Command: %d:: %s\n", strlen(cmd), cmd );
	nw = split ( cmd, wp, 8 );
	// printf ( "nw = %d\n", nw );
	if ( nw != 4 ) {
	    printf ( "Bad command\n" );
	    return;
	}

	c = atoi ( wp[1] );
	h = atoi ( wp[2] );
	s = atoi ( wp[3] );

	// printf ( "C = %d\n", c );
	// printf ( "H = %d\n", h );
	// printf ( "S = %d\n", s );

	buf = hd_read_sector ( c, h, s, &status, 0);
	// status = 0x41;

	printf ( "CHS = %d %d %d  %x\n", c, h, s, status );
	send_buffer ( buf );
}

/* The 'q' to quit also needs a newline after it
 */
void
user_test ( void )
{
	char line[64];

	// fill_buffer ( 0xbc );

	for ( ;; ) {
	    uart_puts ( "Ready%" );
	    uart_gets ( line );
	    if ( line[0] == 'q' )
		break;
	    if ( strlen(line) == 0 )
		continue;

	    run_command ( line );
	    // uart_puts ( "Got: " );
	    // uart_puts ( line );
	}

#ifdef notdef
	for ( ;; ) {
	    uart_stat ();
	    delay_one ();
	}
#endif
}

void
start ( void )
{
	uart_init ();

	delay_one ();

	hd_init ();

	hd_test ();

	printf ( "Starting test\n" );
	user_test ();
	printf ( " -- Done\n" );
}

/* THE END */
