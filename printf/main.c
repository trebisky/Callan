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

/* Test multibus RAM.
 * We have 512K at 0x110000 to 0x18ffff
 */

/* Every segment shows this:

Fail at 0018FC00 -- DEADBEEF --> 00010001
Fail at 0018FC00 -- 21524110 --> 00010001
Fail at 0018FC04 -- DEADBEEF --> 00010001
Fail at 0018FC04 -- 21524110 --> 00010001
Fail at 0018FC08 -- DEADBEEF --> 00010001
Fail at 0018FC08 -- 21524110 --> 00010001
Fail at 0018FC0C -- DEADBEEF --> 00010001
Fail at 0018FC0C -- 21524110 --> 00010001
Fail at 0018FC10 -- DEADBEEF --> 00010001
Fail at 0018FC10 -- 21524110 --> 00010001
Fail at 0018FC14 -- DEADBEEF --> 00010001
Fail at 0018FC14 -- 21524110 --> 00010001
Fail at 0018FC18 -- DEADBEEF --> 00010001
Fail at 0018FC18 -- 21524110 --> 00010001
Fail at 0018FC1C -- DEADBEEF --> 00010001
Fail at 0018FC1C -- 21524110 --> 00010001
 */

#define MB_BASE		0x110000
#define MB_SIZE 	512*1024

/* Avoid last 4K */
#define SEG_SIZE 	15*4096

static void
ram_seg ( int base )
{
	unsigned int *rp;
	unsigned int *end;
	unsigned int x;

	rp = (unsigned int *) base;
	end = rp + SEG_SIZE / sizeof(unsigned int);

	while ( rp < end ) {
	    x = 0xDEADBEEF;
	    *rp = x;
	    if ( *rp != x )
		printf ( "Fail at %h -- %h --> %h\n", rp, x, *rp );
	    x = ~x;
	    *rp = x;
	    if ( *rp != x )
		printf ( "Fail at %h -- %h --> %h\n", rp, x, *rp );
	    rp++;
	}
}

static void
ram_seg2 ( int base )
{
	unsigned int *rp;
	unsigned int *end;
	int ok = 1;
	int first = 1;
	unsigned int x;

	rp = (unsigned int *) base;
	end = rp + SEG_SIZE / sizeof(unsigned int);

	while ( rp < end ) {
	    *rp = (unsigned int) rp;
	    rp++;
	}

	rp = (unsigned int *) base;
	while ( rp < end ) {
	    if ( *rp != (unsigned int) rp ) {
		x = (unsigned int) rp;
		if ( first )
		    printf ( "Fail at %h -- %h --> %h\n", rp, x, *rp );
		first = 0;
		ok = 0;
	    }
	    rp++;
	}

	if ( ok )
	    printf ( "Seg %h OK\n", base );
}

void
ram_test ( void )
{
	int base;
	int end;

	base = MB_BASE;
	end = base + MB_SIZE;

	while ( base < end ) {
	    printf ( "Testing: %h\n", base );
	    ram_seg ( base );
	    ram_seg2 ( base );
	    base += 0x10000;
	}
}

void
start ( void )
{
	uart_init ();
	// bss_clear ( 0xabcdabcd, 0xdeadbeef );

	uart_puts ( "Hello world\n" );

	printf ( "Hello again\n" );

	for ( ;; ) {
	    ram_test ();
	    printf ( " -- Done\n" );
	}
}

/* THE END */
