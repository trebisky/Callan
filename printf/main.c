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

#ifdef notdef
void
delay_x ( void )
{
	volatile int count;
	
	count = 50000;
	while ( count-- )
	    ;
}
#endif

int bss_rubbish[4];

void
bss_clear ( unsigned int *b1, unsigned int *b2 )
{
	while ( b1 < b2 )
	    *b1++ = 0;
}

void
start ( void )
{
	uart_init ();
	// bss_clear ( 0xabcdabcd, 0xdeadbeef );

	uart_puts ( "Hello world\n" );

	printf ( "Hello again\n" );
}

/* THE END */
