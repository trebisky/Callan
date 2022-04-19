/* hd.c
 *
 * A second stab at a "driver" for
 * the Callan CWC
 *
 *  2-27-2022  Tom Trebisky
 *  4-15-2022  Tom Trebisky
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
	// vu_char pad;		/* 0 */
	vu_char status;		/* 1 */
	char _pad1;
	vu_char addr_mid;	/* 3 */
	char _pad2;
	vu_char addr_hi;	/* 5 */
	vu_char reseti;		/* 6 */
	vu_char ctrl;		/* 7 */
	vu_char bad;		/* 7 */
};

#define addr_lo		status

/* This is 18 bytes (9 words) */
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

/* This is 24 bytes (12 words) */
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

/* This is 12 bytes */
struct hd_params {
	vu_short	sector_size;
	vu_char		heads;
	vu_char		spt;
	vu_short	ncyl;
	vu_short	rwc;
	vu_short	pre;
	vu_char		step_norm;
	vu_char		step_restore;
};

#define CMD_STATUS	0
#define CMD_RESET	1
#define CMD_READ	3
#define CMD_RRAM	0x0b	/* read 2k ram diagnostic */
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
// The following for the faulty Card 2
// #define MB_OFFSET		0x10000
// The following for Card 1
#define MB_OFFSET		0x00000
#define LOCAL_MB_BASE		(MB_BASE + MB_OFFSET)

/* What I call "DMA" addresses are addresses as seen by the CWC */
#define DMA_CMD		(MB_OFFSET + 0x100)
#define DMA_STATUS	(MB_OFFSET + 0x200)
#define DMA_DATA	(MB_OFFSET + 0x600)

/* What I call "LOCAL" addresses are addresses as seen by the CPU */
#define LOCAL_CMD	(LOCAL_MB_BASE + DMA_CMD)
#define LOCAL_STATUS	(LOCAL_MB_BASE + DMA_STATUS)
#define LOCAL_DATA	(LOCAL_MB_BASE + DMA_DATA)

/* for command timeouts */
#define LIMIT	500000

/* Nothing here for now, but this always gets called to start the driver.
 */
void
hd_init ( void )
{
}

/* So far, I have always seen this immediately go busy with no
 * need to loop and check.  But it does not hurt.
 */
int
hd_start ( int verbose )
{
	struct hd_regs *hp = HD_BASE;
	unsigned int addr;
	int w;

	if ( hp->status & ST_BUSY ) {
	    printf ( "Controller busy, cannot start %h\n", hp->status );
	    return 0;
	}

	addr = DMA_CMD;
	// if ( verbose )
	//     printf ( "Give controller addr = %h\n", addr );

	hp->addr_lo = addr & 0xff;
	hp->addr_mid = (addr>>8) & 0xff;
	hp->addr_hi = (addr>>16) & 0xff;

	/* Writing this says "go" */
	hp->ctrl = CTL_WL;

	/* Wait for it to go busy */
	w = 0;
	while ( w < LIMIT ) {
	    if ( ! (hp->status & ST_BUSY) ) {
		w++;
		continue;
	    }
	    break;
	}

	if ( w >= LIMIT ) {
	    printf ( "Command never started\n" );
	    return 0;
	}

	if ( verbose )
	    printf ( "Went busy after %d\n", w );

	return 1;
}

void
hd_wait ( void )
{
	struct hd_regs *hp = HD_BASE;
	int w;
	int s;

	/* We see 0x80 when polling, i.e. BUSY */

	/* This now works as it should */
	w = 0;
	while ( w < LIMIT ) {
	    s = hp->status;
	    // printf ( " status: %h\n", s );
	    if ( s & ST_BUSY ) {
		w++;
		continue;
	    }
	    break;
	}

	/* I once saw: 0x1934F = 103247
	 * when using the defective RAM board.
	 */
	printf ( "hd_wait finished after: %d\n", w );
}

void
hd_show_data ( char *buf, int count )
{
	printf ( "Data block --\n" );
	dump_buf ( buf, count );
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

/* Before rearranging boards (when the CWC would never be able
 * to get the bus), I had wait counts over 100,000.
 * Now with the WDC the highest priority master, I get
 * a wait count of 1 !!!
 */

void
hd_test_cmd ( int cmd, int count )
{
	struct hd_cmd *hd_cmd;
	struct hd_status *hd_status;
	struct hd_data *hd_data;
	int s;

	hd_cmd = (struct hd_cmd *) LOCAL_CMD;
	hd_status = (struct hd_status *) LOCAL_STATUS;
	hd_data = (struct hd_data *) LOCAL_DATA;

	memset ( (char *) hd_cmd, sizeof(*hd_cmd), 0 );
	memset ( (char *) hd_status, sizeof(*hd_status), 0xab );
	memset ( (char *) hd_data, sizeof(*hd_status), 0xab );

	printf ( "Blocks clear\n" );
	// hd_show_status ( LOCAL_STATUS );

	hd_cmd->cmd = cmd;
	hd_cmd->unit = 0;
	hd_cmd->status = (vu_long) DMA_STATUS;
	hd_cmd->count = count;
	hd_cmd->addr = (vu_long) DMA_DATA;

	printf ( "Starting cmd\n" );
	s = hd_start ( 1 );
	if ( ! s ) {
	    printf ( "Command aborted\n" );
	    return;
	}

	printf ( "Waiting for cmd\n" );
	hd_wait ();
	printf ( "Waiting done\n" );

	hd_show_status ( (struct hd_status *) LOCAL_STATUS );
	hd_show_data ( (char *) LOCAL_DATA, count );
}

void
hd_reset_controller ( void )
{
	/* Reset controller */
	hd_test_cmd ( CMD_RESET, 16 );
}

void
hd_check_status ( void )
{
	/* Get status of selected drive */
	hd_test_cmd ( CMD_STATUS, 16 );
}

void
hd_read_ram ( void )
{
	hd_test_cmd ( CMD_RRAM, 2048 );
}

#define SECTOR_SIZE	512

/* This is hd_test_cmd() copied and modified.
 * The first time I tried it, it complained that a disk paramter block
 * had not been loaded!
 */
void
hd_read_sector ( int c, int h, int s )
{
	struct hd_cmd *hd_cmd;
	struct hd_status *hd_status;
	struct hd_data *hd_data;
	int stat;

	hd_cmd = (struct hd_cmd *) LOCAL_CMD;
	hd_status = (struct hd_status *) LOCAL_STATUS;
	hd_data = (struct hd_data *) LOCAL_DATA;

	memset ( (char *) hd_cmd, sizeof(*hd_cmd), 0 );
	memset ( (char *) hd_status, sizeof(*hd_status), 0xab );
	memset ( (char *) hd_data, SECTOR_SIZE, 0xab );

	// printf ( "Blocks clear\n" );
	// hd_show_status ( LOCAL_STATUS );

	hd_cmd->cmd = CMD_READ;
	hd_cmd->unit = 0;
	hd_cmd->head = h;
	hd_cmd->sector = s;
	hd_cmd->cyl = c;
	hd_cmd->status = (vu_long) DMA_STATUS;
	hd_cmd->count = SECTOR_SIZE;
	hd_cmd->addr = (vu_long) DMA_DATA;

	// printf ( "Starting cmd\n" );
	stat = hd_start ( 1 );
	if ( ! stat ) {
	    printf ( "Command aborted\n" );
	    return;
	}

	// printf ( "Waiting for cmd\n" );
	hd_wait ();
	// printf ( "Waiting done\n" );

	hd_show_status ( (struct hd_status *) LOCAL_STATUS );
	hd_show_data ( (char *) LOCAL_DATA, SECTOR_SIZE );
}

/* I have a Rodime 204 drive
 * This has 320 cylinders and 8 heads (4 platters)
 * It can be formatted to have 17 sectors of 512 bytes
 * Values below are pulled from the Aux ROM disassembly.
 * Note in particular the big value for "ncyl"
 */

/* And this is hd_read_sector() copied and modified.
 */
void
hd_params ( void )
{
	struct hd_cmd *hd_cmd;
	struct hd_status *hd_status;
	struct hd_params *hd_params;
	int stat;

	hd_cmd = (struct hd_cmd *) LOCAL_CMD;
	hd_status = (struct hd_status *) LOCAL_STATUS;
	hd_params = (struct hd_params *) LOCAL_DATA;

	memset ( (char *) hd_cmd, sizeof(*hd_cmd), 0 );
	memset ( (char *) hd_status, sizeof(*hd_status), 0xab );
	memset ( (char *) hd_params, sizeof(struct hd_params), 0 );

	/* Now load stuff for our Rodime RO 204 */
	/* We never intend to write, so ignore rwc and pre */
	hd_params->sector_size = 512;
	hd_params->heads = 8;
	hd_params->spt = 17;
	hd_params->ncyl = 0x3ff;
	// hd_params->rwc = 0xf0;
	// hd_params->pre = 0xf0;
	hd_params->step_norm = 0x9e;
	hd_params->step_restore = 0xc;

	hd_cmd->cmd = CMD_PARAMS;
	hd_cmd->unit = 0;
	hd_cmd->status = (vu_long) DMA_STATUS;
	hd_cmd->count = sizeof(struct hd_params);
	hd_cmd->addr = (vu_long) DMA_DATA;

	stat = hd_start ( 1 );
	if ( ! stat ) {
	    printf ( "Command aborted\n" );
	    return;
	}

	hd_wait ();

	hd_show_status ( (struct hd_status *) LOCAL_STATUS );
}

/* =========================================================================== */
/* =========================================================================== */
/* =========================================================================== */

/* mid always reads as zero.
 */
void
hd_test2 ( void )
{
	struct hd_regs *hp = HD_BASE;
	int x;

	x = hp->addr_mid;
	printf ( "Mid: %x\n", x );

	hp->addr_mid = 0x55;
	x = hp->addr_mid;
	printf ( "Mid: %x\n", x );

	hp->addr_mid = ~0x55;
	x = hp->addr_mid;
	printf ( "Mid: %x\n", x );
}

#ifdef notdef
/* pad also always reads as zero.
 * note that no error happens if you access it
 */
void
hd_test3 ( void )
{
	struct hd_regs *hp = HD_BASE;
	int x;

	x = hp->pad;
	printf ( "Pad: %x\n", x );

	hp->pad = 0x55;
	x = hp->pad;
	printf ( "Pad: %x\n", x );

	hp->pad = ~0x55;
	x = hp->pad;
	printf ( "Pad: %x\n", x );
}
#endif

/* this however gives:
 * Bus Error, addr: 001F00A8 at 010A2A
 */
void
hd_test4 ( void )
{
	struct hd_regs *hp = HD_BASE;
	int x;

	x = hp->bad;
	printf ( "Bad: %x\n", x );

	hp->bad = 0x55;
	x = hp->bad;
	printf ( "Bad: %x\n", x );

	hp->bad = ~0x55;
	x = hp->bad;
	printf ( "Bad: %x\n", x );
}

/* This returns 0 forever */
void
hd_test5 ( void )
{
	struct hd_regs *hp = HD_BASE;
	int x;
	int n;

	n = 0;
	for ( ;; ) {
	    // delay_x ();
	    delay_one ();
	    n++;
	    x = hp->status;
	    printf ( "Status: %x %d\n", x, n );
	}
}

/* I run this and get:
 *  Exception: Tr
 * While waiting for the command
 */
void
hd_test1 ( void )
{
	struct hd_regs *hp = HD_BASE;
	int s;

	s = hp->status;
	printf ( "Controller status: %h\n", s );

#ifdef notdef
	printf ( " --- --- Reset controller\n" );
	hd_reset_controller ();
#endif
}

/* =========================================================================== */
/* =========================================================================== */

void
hd_test_rram ( void )
{
	printf ( " --- --- Read 2K ram\n" );
	hd_read_ram ();
}

void
hd_test_status ( void )
{
	printf ( " --- --- Check drive 0 status\n" );
	hd_check_status ();
}


/* status gives the following:
Starting HD test
 --- --- Check drive 0 status
Went busy after 0
hd_wait finished after: 0
Status block --
00100200  00 41 00 00 00 00 00 00 00 00 00 10 00 00 06 00
00100210  00 00 02 00 00 00 00 00
 status: 00000041
 code: 00000000
 unit: 00000000
 cmd: 00000000
 cyl: 00000000
 sbp: 00000200
Data block --
00100600  AB AB AB AB AB AB AB AB AB AB AB AB AB AB AB AB

 * So, no data (which makes sense).
 * status byte is 0x41 which is "drive not ready or not connected".
 * Which also makes sense.
 * "sbp" is status block pointer.
 *   this makes sense (we put the status block at 0x200)
 */

void
hd_test_read ( void )
{
	int cyl, head, sector;

	printf ( " --- --- Read sector\n" );
	hd_read_sector ( 0, 0, 0 );
}

void
hd_test_params ( void )
{
	int cyl, head, sector;

	printf ( " --- --- Load params\n" );
	hd_params ();
}


void
hd_test ( void )
{
	// hd_test2 ();
	// hd_test3 ();
	// hd_test4 ();
	// hd_test5 ();

	// hd_test_rram ();
	// hd_test_status ();

	hd_test_params ();
	hd_test_read ();
}

void
memset ( char *buf, int n, int val )
{
	while ( n-- )
	    *buf++ = val;
}

/* THE END */
