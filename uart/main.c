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

void
user_test ( void )
{
}

void
start ( void )
{
	uart_init ();

	printf ( "Starting test\n" );
	user_test ();
	printf ( " -- Done\n" );
}

/* THE END */
