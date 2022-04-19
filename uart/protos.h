/* prototypes
 */

void uart_init ( void );
int uart_getc ( void );
void uart_putc ( int );
void uart_puts ( char * );
void uart_stat ( void );

void printf ( char *, ... );
void show_reg ( char *, int * );

void dump_buf ( void *, int );

void delay_x ( void );
void delay_one ( void );
void delay_ms ( int );

/* THE END */
