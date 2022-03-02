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

/* Note that the entries in the following
 * are byte swapped as compared to the manual.
 * That is because the m68k is big endian and
 * the CWC manual was written from a little endian view.
 */
struct hdc {
	char _pad0;
	vu_char addr_lo;	/* 1 */
	char _pad1;
	vu_char addr_mid;	/* 3 */
	char _pad2;
	vu_char addr_hi;	/* 5 */
	vu_char reseti;		/* 6 */
	vu_char ctrl;		/* 7 */
};

#define status	addr_lo

#define HD_BASE		((struct hdc *) 0x1f00a0)

#define ST_BUSY		0x80
#define ST_INT		0x40

/* We will probably never enable interrupts.
 * We should set the WL bit.
 * Any write to ctrl will make the controller "go"
 * so we whould have written the 3 address bytes already.
 */
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
