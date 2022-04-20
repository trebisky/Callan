/* main.c
 *
 * Ram diagnostic 4-20-2022
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

	printf ( "Start RAM testing\n" );

	ram_test ();

	printf ( " -- Done\n" );
}

/* THE END */
