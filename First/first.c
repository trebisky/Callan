/* start.c
 *
 * The first bit of C code that gets run
 * 10-30-2006
 */

typedef volatile unsigned short vu_short;

struct uart {
	vu_short data;
	vu_short csr;
};

#define CSR_TBE		0x04
#define CSR_RCA		0x01

#define UART0_BASE	((struct uart *) 0x600000)
#define UART1_BASE	((struct uart *) 0x600004)

void
delay_x ( void )
{
	volatile int count;
	
	count = 50000;
	while ( count-- )
	    ;
}

void
putc ( int c )
{
	struct uart *up = UART0_BASE;

	while ( ! (up->csr & CSR_TBE) )
	    ;
	up->data = c;
}

void
start ( void )
{
	for ( ;; )
	    putc ( 'X' );
}

/* THE END */
