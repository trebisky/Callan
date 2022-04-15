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

/* for command timeouts */
#define LIMIT	500000

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
	if ( verbose )
	    printf ( "Give controller addr = %h\n", addr );

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

#ifdef notdef
/* Case 1 - this causes weird crashes */

/* We see 0x80 when polling, i.e. BUSY */
	int s;

	w = 0;
	while ( w < LIMIT ) {
	    s = hp->status;
	    printf ( " status: %h\n", s );
	    if ( s & ST_BUSY ) {
		w++;
		continue;
	    }
	    break;
	}

	/* I see: 0x1934F = 103247 */
	printf ( "hd_wait finished after: %h\n", w );
#endif

#ifdef notdef
/* Case 2 - this works */
	uart_putc ( 'X' );
	uart_putc ( 'X' );
	delay_one ();

	w = 0;
	while ( w < LIMIT ) {
	    delay_one ();
	    if ( hp->status & ST_BUSY ) {
		w++;
		uart_putc ( '.' );
	    } else
		break;
	}
#endif

/* Case 3 - this works */
/* this is finished after 1 check */

	w = 0;
	while ( w < LIMIT ) {
	    if ( hp->status & ST_BUSY ) {
		w++;
		// uart_putc ( '.' );
		delay_ms ( 1 );
	    } else
		break;
	}

	printf ( "hd_wait finished after: %d\n", w );
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
hd_test_cmd ( int cmd )
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
	hd_cmd->count = sizeof ( struct hd_status );
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
	hd_show_status ( (struct hd_status *) LOCAL_DATA );
}

void
hd_reset_controller ( void )
{
	/* Reset controller */
	hd_test_cmd ( CMD_RESET );
}

void
hd_check_status ( void )
{
	/* Get status of selected drive */
	hd_test_cmd ( CMD_STATUS );
}

void
hd_read_ram ( void )
{
	hd_test_cmd ( CMD_RRAM );
}

void
hd_params ( void )
{
}

void
hd_init ( void )
{
}

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

	printf ( " --- --- Check drive 0 status\n" );
	hd_check_status ();
#endif

	printf ( " --- --- Read 2K ram\n" );
	hd_read_ram ();

#ifdef notyet
	printf ( " --- --- Drive params\n" );
	hd_params ();
#endif
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

/* "Scope loops" for use with logic analyzer
 */

/* The idea with this first loop was to just get used to using
 * the OLS logic analyzer.
 * I have to capture at 10 Mhz to see all the pulses.
 * (actually 5 Mhz seems to work, but 10 is better)
 * I have one probe on pin 19 of U67 (the 25LS2521).
 * This chip is the comparator that recognizes when an address
 * on the multibus matches the board.
 * The pulses I see are active low, 250 ns in size,
 * spaced 8 us apart.
 * I can use a "low level" trigger to make the capture start
 * with the first pulse.
 * Looking at the generated code there is a lot of extra
 * rubbish generated by the compiler, but this works just fine
 * for my purposes.
 */
void
scope1 ( void )
{
	struct hd_regs *hp = HD_BASE;
	int s;

	for ( ;; ) {
	    s = hp->status;
	    s = hp->status;
	    s = hp->status;
	    s = hp->status;
	    delay_ms ( 1 );
	}
}

/* We end up needing to hack on this too */
int
scope_start ( int verbose )
{
	struct hd_regs *hp = HD_BASE;
	unsigned int addr;
	int w;

	if ( hp->status & ST_BUSY ) {
	    printf ( "Controller busy, cannot start %h\n", hp->status );
	    return 0;
	}

	addr = DMA_CMD;
	if ( verbose )
	    printf ( "Give controller addr = %h\n", addr );

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


/* checking on 1 ms intervals again began causing processor
 * hangs, so we copy hd_wait() here and fiddle with it.
 */
void
scope_wait ( void )
{
	// struct hd_regs *hp = HD_BASE;
	// int w;

	delay_ms ( 500 );
}

/* This is hd_test_cmd() copied here so I can butcher
 * it for use by scope2 ()
 */
void
scope_test_cmd ( int cmd )
{
	struct hd_cmd *hd_cmd;
	struct hd_status *hd_status;
	struct hd_data *hd_data;
	int s;
	int count;

	hd_cmd = (struct hd_cmd *) LOCAL_CMD;
	hd_status = (struct hd_status *) LOCAL_STATUS;
	hd_data = (struct hd_data *) LOCAL_DATA;

	memset ( (char *) hd_cmd, sizeof(*hd_cmd), 0 );
	memset ( (char *) hd_status, sizeof(*hd_status), 0xab );
	memset ( (char *) hd_data, sizeof(*hd_status), 0xab );

	// printf ( "Blocks clear\n" );
	// hd_show_status ( LOCAL_STATUS );

	hd_cmd->cmd = cmd;
	hd_cmd->unit = 0;
	hd_cmd->status = (vu_long) DMA_STATUS;
	hd_cmd->count = sizeof ( struct hd_status );
	hd_cmd->addr = (vu_long) DMA_DATA;

	/* loop here starting the command over and over */
	count = 0;
	for ( ;; ) {
	    printf ( "Cmd %d\n", ++count );
	    // printf ( "Starting cmd\n" );
	    s = scope_start ( 0 );
#ifdef notdef
	    if ( ! s ) {
		printf ( "Command aborted\n" );
		return;
	    }
#endif

	    // printf ( "Waiting for cmd\n" );
	    scope_wait ();
	    // printf ( "Waiting done\n" );
	}

	// hd_show_status ( (struct hd_status *) LOCAL_STATUS );
	// hd_show_status ( (struct hd_status *) LOCAL_DATA );
}

/* On a 20 pin chip like an LS245, pin 10 is ground,
 *  and pin 20 is Vcc.
 * I connect probe 0 to Multibus BCLK
 * I connect probe 1 to BREQ
 * I connect probe 2 to BPRN

 * On Bclk, I see a square wave with a 2.5 ms period (400 Hz),
 *  this is entirely bogus and due to me sampling too slowly.
 * I crank up the analyzer sample rate to 50 Mhz and I see
 *  a 7.7 Mhz clock (130 ns period).
 *  maybe this is 8 Mhz from a really sloppy oscillator.
 * One book I have says it typically runs at 10 Mhz.
 *
 * I set up probe 1 (Breq) to trigger if it goes low.
 * And it does as soon as my test case runs.
 * My test case does the ugly crash thing,
 * but I see Breq go low 9 times along with Bprn going low in
 * response each time.
 * The Breq pulse is 1000 ns low, as is the response.
 * The response is delayed by 20 ns.
 *
 * The individual pulses are 3 us apart.
 * Why only 9?  And why does the CPU crash?
 * The command block is 18 bytes.  Maybe the CWC is getting it
 * 9 "words" at a time.
 *
 * I am still getting odd troubles.  I run the capture at 10 Mhz and
 *  see the following:
 * 1 - I trigger on Breq going low
 * 2 - I see the 9 pules as described above.
 *     that burst lasts about 24 us
 * 3 - At about 145 us I see Breq going low and staying low
 *     it does get the virtually immediate Bprn to give the CWC the bus.
 * 4 - But it holds the bus forever (or as long as my capture, which is
 *     for 614 us at the 10 Mhz sample rate.
 *
 * Conclusion - the CWC is getting and never releasing the multibus.
 * What probably goes on from the CPU side is that it tries to get the
 * bus in order to read status, and finds itself waiting indefinitely.
 * Because of this the interrupt driven refresh never happens, DRAM
 * contents goes to rubbish and we are out of luck, or something of
 * sort.  So why is the CWC "hung" and not releasing the bus?
 * Maybe it is time to try a different memory card.
 *
 */
void
scope2 ( void )
{
	struct hd_regs *hp = HD_BASE;
	int s;

	/* Check on initial board status */
	s = hp->status;
	if ( s & ST_BUSY )
	    printf ( "Controller busy (hung) on test startup %h\n", s );

	scope_test_cmd ( CMD_RRAM );
}

void
hd_test ( void )
{
	// hd_test2 ();
	// hd_test3 ();
	// hd_test4 ();
	// hd_test5 ();

	// hd_test1 ();

	// scope1 ();
	scope2 ();
}

void
memset ( char *buf, int n, int val )
{
	while ( n-- )
	    *buf++ = val;
}

/* THE END */
