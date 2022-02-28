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

#ifdef notdef
/* You can do this, but you have to shift data 8 bits
 * into or out of the top 8 bits.
 * The scheme below avoids all these shifts and
 * is a better choice I think.
 */
struct uart {
	vu_short data;
	vu_short csr;
};
#endif

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

#ifdef notdef
void
putc ( int c )
{
	struct uart *up = UART0_BASE;

	while ( ! ((up->csr>>8) & CSR_TBE) )
	    ;
	up->data = c << 8;
}
#endif

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
	    putc ( 'A' );
}

/* THE END */
