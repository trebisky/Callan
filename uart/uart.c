/* uart.c
 *
 * Driver for the NEC 7201 uart on the Callan
 *
 *  2-28-2022  Tom Trebisky
 *
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
uart_init ( void )
{
	/* nothing here, we rely on the boot roms */
}

void
uart_putc ( int c )
{
	struct uart *up = UART0_BASE;

	while ( ! (up->csr & CSR_TBE) )
	    ;
	up->data = c;
}

/* Portable code below here */

void
uart_puts ( char *s )
{
        while ( *s ) {
            if (*s == '\n')
                uart_putc('\r');
            uart_putc(*s++);
        }
}

#ifdef notdef
/* I am keeping this here just for the record -- */
/* You can do this, but you have to shift data 8 bits
 * into or out of the top 8 bits.
 * The scheme below avoids all these shifts and
 * is a better choice I think.
 */
struct uart {
	vu_short data;
	vu_short csr;
};

void
putc ( int c )
{
	struct uart *up = UART0_BASE;

	while ( ! ((up->csr>>8) & CSR_TBE) )
	    ;
	up->data = c << 8;
}
#endif

/* THE END */
