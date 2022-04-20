/* ram.c
 *
 * 4-20-2022
 */

#include "protos.h"

#ifdef notdef
/* This is correct for "Card 2" which has the MB8264 chips
 * and various problems
 */
#define MB_BASE		0x110000
#define MB_SIZE 	512*1024

/* This is correct for "Card 1" which has 4116 chips
 */
#define MB_BASE		0x100000
#define MB_SIZE 	64*1024
#endif

/* This is correct for the 2M Wicat card
 */
#define MB_BASE		0x100000
#define MB_SIZE 	15*64*1024
// #define MB_SIZE 	16*64*1024

#define SEG_SIZE	0x10000		/* 64K */

/* Avoid last 4K
 * We did this for our screwed up 512K card.
 */
#define SEG_HACK 	15*4096

/* --------------------------------------------------------- */
/* --------------------------------------------------------- */


/* Test multibus RAM.
 *
 * Card 1 has 64K at 0x100000 to 0x10ffff
 * It has no errors.
 *
 * Card 2 has 512K at 0x110000 to 0x18ffff
 * It has many errors, as follows ---
 *
 * Note - the 0x0018xxxx address is correct,
 * this card is "bumped up" by 64k to leave room
 * for card 1 ahead of it.
 *
 * Every segment shows this:

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

static void
ram_seg ( int base )
{
	unsigned int *rp;
	unsigned int *end;
	unsigned int x;

	rp = (unsigned int *) base;
	// end = rp + SEG_HACK / sizeof(unsigned int);
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
	// end = rp + SEG_HACK / sizeof(unsigned int);
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

	// if ( ok )
	//     printf ( "Seg %h OK\n", base );
}

void
ram_loop1 ( void )
{
	int base;
	int end;

	printf ( "Phase 1\n" );

	base = MB_BASE;
	end = base + MB_SIZE;

	/* We loop over 64K sections.
	 * Card 1 has only one such section.
	 */
	while ( base < end ) {
	    // printf ( "Testing: %h\n", base );
	    ram_seg ( base );
	    ram_seg2 ( base );
	    base += SEG_SIZE;
	}
}

void
ram_loop2 ( void )
{
	int base;
	int end;
	int *p;
	int *p_end;
	int x;

	printf ( "Phase 2\n" );

	base = MB_BASE;
	end = base + MB_SIZE;
	p_end = ( int * ) end;

	p = ( int * ) base;
	while ( p < p_end ) {
	    *p = (int) p;
	    p++;
	}

	p = ( int * ) base;
	while ( p < p_end ) {
	    x = *p;
	    if ( x != (int) p )
		printf ( "Pass 2 fail at %h -- %h\n", p, x );
	    p++;
	}
}

void
ram_test ( void )
{
	int pass = 0;

	for ( ;; ) {
	    pass++;
	    printf ( "---- Pass %d -----\n", pass );
	    ram_loop1 ();
	    ram_loop2 ();
	}
}

/* THE END */
