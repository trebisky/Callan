/* hd.c
 *
 * A first stab at a "driver" for
 * the Callan CWC
 *
 *  2-27-2022  Tom Trebisky
 *
 */

typedef volatile unsigned short vu_short;
typedef volatile unsigned char vu_char;

struct hdc {
	vu_char addr_lo;	/* 0 */
	char _pad0;
	vu_char addr_mid;	/* 2 */
	char _pad1;
	vu_char addr_hi;	/* 4 */
	char _pad2;
	vu_char ctrl;		/* 6 */
	vu_char reseti;		/* 7 */
};

#define status	addr_lo

#define HD_BASE		((struct hdc *) 0x1f00a0)

#define ST_BUSY		0x80
#define ST_INT		0x40

#define CTL_IE		0x80
#define CTL_WL		0x40

void
hd_init ( void )
{
	struct hdc *hp = HD_BASE;
	int x;

	x = hp->status;
}

/* THE END */
