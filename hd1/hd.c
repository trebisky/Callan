/* hd.c
 *
 * A first stab at a "driver" for
 * the Callan CWC
 *
 *  2-27-2022  Tom Trebisky
 *
 */

#include "protos.h"

void memset ( char *, int, int );

typedef volatile unsigned char vu_char;
typedef volatile unsigned short vu_short;
typedef volatile unsigned long vu_long;

/* Note that the entries in the following
 * are byte swapped as compared to the manual.
 * That is because the m68k is big endian and
 * the CWC manual was written from a little endian view.
 */
struct hd_regs {
	char _pad0;
	vu_char status;		/* 1 */
	char _pad1;
	vu_char addr_mid;	/* 3 */
	char _pad2;
	vu_char addr_hi;	/* 5 */
	vu_char reseti;		/* 6 */
	vu_char ctrl;		/* 7 */
};

#define addr_lo		status

struct hd_cmd {
	vu_char		unit;
	vu_char		cmd;
	vu_char		head;
	vu_char		sector;
	vu_short	cyl;
	vu_long		count;
	vu_long		addr;
	vu_long		status;
};

struct hd_status {
	vu_char		code;
	vu_char		stat;
	vu_char		unit;
	vu_char		cmd;
	vu_char		head;
	vu_char		sector;
	vu_short	cyl;
	vu_long		count;
	vu_long		addr;
	vu_long		sbp;
	vu_long		ecc;
};

#define CMD_STATUS	0
#define CMD_RESET	1
#define CMD_READ	3
#define CMD_SEEK	0x10
#define CMD_PARAMS	0x11
#define CMD_RESTORE	0x12

#define HD_BASE		((struct hd_regs *) 0x1f00a0)

#define ST_BUSY		0x80
#define ST_INT		0x40

/* We will probably never enable interrupts.
 * We should set the WL bit.
 * Any write to ctrl will make the controller "go"
 * so we whould have written the 3 address bytes already.
 */
#define CTL_IE		0x80
#define CTL_WL		0x40

/* ---------------------------------------------------- */

/* Our multibus RAM setup is unusual.
 * Our RAM board is jumpered to start 64K up (at 0x10000)
 * also it has some defects, so we can only use
 *  the first part (0 - 0xefff)
 * For this project this is no big deal
 */
#define MB_BASE			0x100000
#define MB_OFFSET		0x10000
#define LOCAL_MB_BASE		(MB_BASE + MB_OFFSET)

/* What I call "DMA" addresses are addresses as seen by the CWC */
#define DMA_CMD		(MB_OFFSET + 0x100)
#define DMA_STATUS	(MB_OFFSET + 0x200)
#define DMA_DATA	(MB_OFFSET + 0x600)

/* What I call "LOCAL" addresses are addresses as seen by the CPU */
#define LOCAL_CMD	(LOCAL_MB_BASE + DMA_CMD)
#define LOCAL_STATUS	(LOCAL_MB_BASE + DMA_STATUS)
#define LOCAL_DATA	(LOCAL_MB_BASE + DMA_DATA)

void
hd_start ( void )
{
	struct hd_regs *hp = HD_BASE;
	unsigned int addr;

	addr = DMA_CMD;
	printf ( "Give controller addr = %h\n", addr );

	hp->addr_lo = addr & 0xff;
	hp->addr_mid = (addr>>8) & 0xff;
	hp->addr_hi = (addr>>16) & 0xff;
	hp->ctrl = CTL_WL;
}

#define LIMIT	500000

void
hd_wait ( void )
{
	struct hd_regs *hp = HD_BASE;
	int w = 0;

#ifdef notdef
/* We see 0x80 when polling, i.e. BUSY */
	int s;

	while ( w < LIMIT ) {
	    s = hp->status;
	    printf ( " status: %h\n", s );
	    if ( s & ST_BUSY ) {
		w++;
		continue;
	    }
	    break;
	}
#endif

	while ( w < LIMIT ) {
	    if ( hp->status & ST_BUSY ) {
		w++;
		continue;
	    }
	    break;
	}

	// My printf has some kind of bug for %d
	printf ( "hd_wait finished after: %d\n", w );

	/* I see: 0x1934F = 103247 */
	printf ( "hd_wait finished after: %h\n", w );
}
void
hd_show_status ( struct hd_status *hs )
{
	printf ( "Status block --\n" );
	dump_buf ( (void *) hs, sizeof(*hs) );

	printf ( " status: %h\n", hs->stat );
	printf ( " code: %h\n", hs->code );
	printf ( " unit: %h\n", hs->unit );
	printf ( " cmd: %h\n", hs->cmd );
	printf ( " cyl: %h\n", hs->cyl );
	printf ( " sbp: %h\n", hs->sbp );
}

void
hd_check_status ( void )
{
	struct hd_cmd *hd_cmd;
	struct hd_status *hd_status;
	struct hd_data *hd_data;

	hd_cmd = (struct hd_cmd *) LOCAL_CMD;
	hd_status = (struct hd_status *) LOCAL_STATUS;
	hd_data = (struct hd_data *) LOCAL_DATA;

	memset ( (char *) hd_cmd, sizeof(*hd_cmd), 0 );
	memset ( (char *) hd_status, sizeof(*hd_status), 0xab );
	memset ( (char *) hd_data, sizeof(*hd_status), 0xab );

	printf ( "Blocks clear\n" );
	// hd_show_status ( LOCAL_STATUS );

	hd_cmd->cmd = CMD_STATUS;
	hd_cmd->unit = 0;
	hd_cmd->status = (vu_long) DMA_STATUS;
	hd_cmd->count = sizeof ( struct hd_status );
	hd_cmd->addr = (vu_long) DMA_DATA;

	printf ( "Starting cmd\n" );
	hd_start ();
	printf ( "Waiting for cmd\n" );
	hd_wait ();
	printf ( "Waiting done\n" );

	hd_show_status ( (struct hd_status *) LOCAL_STATUS );
	hd_show_status ( (struct hd_status *) LOCAL_DATA );
}

void
hd_init ( void )
{
}

void
hd_test ( void )
{
	struct hd_regs *hp = HD_BASE;
	int s;

	s = hp->status;
	printf ( "Controller status: %h\n", s );

	hd_check_status ();
}

void
memset ( char *buf, int n, int val )
{
	while ( n-- )
	    *buf++ = val;
}

/* THE END */
