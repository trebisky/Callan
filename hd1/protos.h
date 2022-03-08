/* prototypes
 */

void uart_init ( void );
void uart_putc ( char );
void uart_puts ( char * );

void hd_init ( void );
void hd_test ( void );

void printf ( char *, ... );
void show_reg ( char *, int * );

void dump_buf ( void *, int );

/* THE END */
