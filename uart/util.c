/* util.c
 */
#include "protos.h"

/* multi line dump, byte by byte.
 *  count gives size of buffer
 * from Kyu, adapted to our limited printf
 */
void
dump_buf ( void *addr, int len )
{
        unsigned char *p;
        int i;

        p = (unsigned char *) addr;

        for ( i=0; i<len; i++ ) {
            if ( (i % 16) == 0 )
                printf ( "%h  ", (long) addr );
                // printf ( "%08x  ", (long) addr );

            // printf ( "%02x ", *p++ );
            printf ( "%x ", *p++ );

            if ( i > 0 && ((i+1) % 16) == 0 ) {
                printf ( "\n" );
                addr += 16;
            }
        }

        if ( (len % 16) != 0 )
            printf ( "\n" );
}

/* THE END */
