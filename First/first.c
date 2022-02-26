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

typedef volatile unsigned short vu_short;
typedef volatile unsigned char vu_char;

struct uart {
	vu_char data;
	char _pad0;
	vu_char csr;
	char _pad1;
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
putca ( int c )
{
	struct uart *up = UART0_BASE;

	while ( ! (up->csr & CSR_TBE) )
	    ;
	up->data = c;
}

void
putcb ( int c )
{
	struct uart *up = UART1_BASE;

	while ( ! (up->csr & CSR_TBE) )
	    ;
	up->data = c;
}

void
start ( void )
{
	for ( ;; )
	    putca ( 'A' );
	    /*
	    putcb ( 'B' );
	    putca ( 'A' );
	    putcb ( 'B' );
	    */
}

/* THE END */
