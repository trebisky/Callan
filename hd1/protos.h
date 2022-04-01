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

void delay_x ( void );
void delay_one ( void );
void delay_ms ( int );

/* THE END */
