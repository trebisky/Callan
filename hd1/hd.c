/* hd.c
 *
 * A first stab at a "driver" for
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

/*
 * When I finally got RRAM to work, this is what I got:
 *
Status block --
00100200  00 80 00 0B 00 00 00 00 00 00 08 00 00 00 06 00
00100210  00 00 02 00 00 00 00 00
 status: 00000080
 code: 00000000
 unit: 00000000
 cmd: 0000000B
 cyl: 00000000
 sbp: 00000200
Data block --
00100600  00 80 00 0B 00 00 00 00 00 00 08 00 00 00 06 00
00100610  00 00 02 00 00 00 00 00 00 00 00 00 00 00 80 00
00100620  08 00 00 00 06 00 24 00 00 02 00 00 00 00 00 00
00100630  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00100640  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00100650  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00100660  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00100670  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00100680  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00100690  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
001006A0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
001006B0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
001006C0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
001006D0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
001006E0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
001006F0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00100700  FF 30 FF 00 FF 00 4F 00 FF 50 FF 00 FF 00 FF 40
00100710  FF 10 FF 00 FF 00 FF 00 6F 40 FF 00 FF 00 BF 10
00100720  CF 80 FF 00 FF 00 5F 00 BF 00 FF 00 FF 00 7F 40
00100730  FF 10 FF 00 FF 00 FF B0 FF 00 FF 00 FF 00 7F 20
00100740  DF 10 FF 00 FF 00 EF 40 DF F0 FF 00 FF 00 FF 80
00100750  AF 20 FF 00 FF 00 EF D0 FF 00 FF 00 FF 00 BF 80
00100760  FF 80 FF 00 FF 00 BF 40 FF B0 FF 00 FF 00 7F 20
00100770  FF 00 FF 00 FF 00 1F 50 DF 20 FF 00 FF 00 DF 60
00100780  EF 10 FF 00 FF 00 9F 20 AF 44 FF 00 FF 00 AF 70
00100790  6F 90 FF 00 FF 00 EF E0 5F 00 FF 00 FF 00 CF 60
001007A0  FF 50 FF 00 FF 00 2F D0 AF 20 FF 00 FF 00 DF 00
001007B0  FF 20 FF 00 FF 00 3F 50 5F 40 FF 00 FF 00 DF 90
001007C0  6F D0 FF 00 FF 00 FF 30 7F 20 FF 00 FF 00 4F 50
001007D0  FF 00 FF 00 FF 00 7F A0 FF 00 FF 00 FF 00 9F 90
001007E0  6F 80 FF 00 FF 00 BF 40 FF D0 FF 00 FF 00 8F D0
001007F0  4F 80 FF 00 FF 00 EF 60 1F 20 FF 00 FF 80 9F 90
00100800  BF 00 FF 00 FF 00 7F 00 EF 00 FF 00 FF 00 FF 80
00100810  6F 00 FF 00 FF 00 5F 40 FF C0 FF 00 FF 00 6F C0
00100820  FF 80 FF 00 FF 00 BF 80 FF 80 FF 00 FF 00 7F 00
00100830  5F 90 FF 00 FF 00 3F 80 5F 00 FF 00 FF 00 AF 00
00100840  FF 30 FF 00 FF 00 FF 50 FF 00 FF 00 FF 00 FF 10
00100850  5F 10 FF 00 FF 00 7F A0 FF 00 FF 00 FF 00 BF 00
00100860  1F 40 FF 00 FF 00 FF 00 AF C0 FF 00 FF 00 9F 00
00100870  FF 00 FF 00 FF 00 1F 10 FF B0 FF 00 FF 00 CF 00
00100880  FF 00 FF 00 FF 00 3F 30 FF 80 FF 00 FF 00 AF 40
00100890  EF 50 FF 00 FF 00 CF C0 1F 20 FF 00 FF 00 5F B0
001008A0  BF 40 FF 00 FF 00 DF 60 BF 00 FF 00 FF 00 1F 90
001008B0  7F 10 FF 00 FF 00 DF C0 FF 10 FF 00 FF 00 CF A0
001008C0  BF B0 FF 00 FF 00 EF D0 BF 30 FF 00 FF 00 3F 10
001008D0  EF 80 FF 00 FF 00 EF C0 0F A0 FF 00 FF 00 AF 80
001008E0  3F 70 FF 00 FF 00 4F 90 3F 90 FF 00 FF 00 DF 80
001008F0  DF 40 FF 00 FF 00 2F B0 8F A0 FF 00 FF 00 6F 50
00100900  BF 00 FF 00 FF 00 6F 20 DF 00 FF 00 FF 00 CF 00
00100910  6F 10 FF 00 FF 00 FF 60 DF 80 FF 00 FF 00 DF 30
00100920  EF 80 FF 00 FF 00 BF 20 EF 60 FF 00 FF 00 FF 80
00100930  0F 80 FF 00 FF 00 DF A0 FF 50 FF 00 FF 00 7F 80
00100940  FF 40 FF 00 FF 00 3F 00 FF 80 FF 00 FF 00 DF 00
00100950  FF 00 FF 00 FF 00 FF 10 DF 00 FF 00 FF 00 FF 40
00100960  EF 30 FF 00 FF 00 BF B0 DF 80 FF 00 FF 00 5F A0
00100970  FF 10 FF 00 FF 00 FF A0 FF 80 FF 20 FF 00 7F 00
00100980  6F 00 FF 00 FF 00 AF 00 5F 80 FF 20 FF 00 7F 10
00100990  5F 00 FF 00 FF 00 BF 20 FF 80 FF 00 FF 00 DF 10
001009A0  7F 00 FF 00 FF 00 9F 00 1F E8 FF 00 FF 00 6F 00
001009B0  5F 40 FF 00 FF 00 6F E0 BF 50 FF 00 FF 00 FF 80
001009C0  3F 80 FF 00 FF 00 FF A0 3F 60 FF 00 BF 00 9F 00
001009D0  2F 10 FF 00 FF 00 BF 80 CF 80 FF 00 FF 00 8F A0
001009E0  BF 80 FF 00 FF 00 BF C0 5F C0 EF 00 FF 00 4F 80
001009F0  7F 10 FF 00 FF 00 EF 00 DF 00 FF 00 FF 00 2F 00
00100A00  5F 80 FF 00 FF 00 5F 00 BF 20 FF 00 FF 00 AF 00
00100A10  FF C0 FF 00 FF 00 AF 00 4F 10 FF 00 FF 00 BF 50
00100A20  FF 80 FF 00 FF 00 7F 20 EF 00 FF 00 FF 00 EF 00
00100A30  EF 00 FF 00 FF 00 EF 10 FF 20 FF 00 FF 00 7F 00
00100A40  BF 00 FF 00 FF 00 FF 80 DF 80 FF 00 FF 00 FF 00
00100A50  FF 30 FF 00 FF 00 BF 00 3F 60 FF 00 FF 00 9F 20
00100A60  9F 00 FF 00 FF 00 5F 20 1F 00 FF 00 FF 00 FF C0
00100A70  BF 90 FF 00 FF 00 5F 80 FF 00 FF 00 FF 00 7F E0
00100A80  2F 00 FF 00 FF 00 2F B0 BF 90 FF 00 FF 00 7F 10
00100A90  BF 00 FF 00 FF 00 3F 90 3F 00 FF 00 FF 00 EF 00
00100AA0  3F 80 FF 00 FF 00 AF 20 7F 60 FF 00 FF 00 8F 30
00100AB0  4F A0 FF 00 FF 00 3F 40 DF C0 FF 00 FF 00 EF C0
00100AC0  8F 80 FF 00 FF 00 7F 10 BF 40 FF 00 FF 00 AF 20
00100AD0  DF 40 FF 00 FF 00 7F 10 FF 20 FF 00 FF 00 8F 80
00100AE0  6F 80 FF 00 FF 00 FF A0 DF 00 FF 00 FF 00 BF 80
00100AF0  3F 80 FF 00 FF 00 5F 00 8F 30 FF 00 FF 00 1F 80
00100B00  CF 00 FF 00 FF 00 FF 10 BF 80 FF 00 FF 00 1F 00
00100B10  AF 60 FF 00 FF 00 6F 00 7F 10 FF 00 FF 00 9F 00
00100B20  BF 80 FF 00 FF 00 5F 10 EF 00 FF 00 FF 00 7F 90
00100B30  7F 00 FF 00 FF 00 FF 10 FF 50 FF 00 FF 00 FF 00
00100B40  FF 60 FF 00 FF 00 FF 50 CF 40 FF 00 FF 00 FF 80
00100B50  DF 30 FF 00 FF 00 FF 10 EF D0 FF 00 FF 00 6F 70
00100B60  8F 00 FF 00 FF 00 7F 50 FF 00 FF 00 FF 00 FF 00
00100B70  FF 10 FF 00 FF 00 FF 90 DF 10 FF 00 FF 00 DF 00
00100B80  FF 40 FF 00 FF 00 AF 10 FF 80 FF 00 FF 00 7B 00
00100B90  4F 80 FF 00 FF 00 EF 00 1E 00 FF 00 FF 00 5F 10
00100BA0  5F E0 FF 00 7F 00 7F 80 8F 40 FF 00 FF 00 DF E0
00100BB0  FF 40 FF 00 FF 00 BF 20 2F 00 FF 00 FF 00 CF 90
00100BC0  7F 70 FF 00 FF 00 EF 60 FF 00 FF 00 FF 00 FF 10
00100BD0  2F 40 FF 00 FF 00 EF F0 3F 00 FF 00 FF 00 6F 50
00100BE0  7F 00 FF 00 FF 00 7F C0 3F 80 FF 00 FF 00 17 30
00100BF0  3F 50 FF 00 FF 00 3F 40 3F 80 FF 00 FF 00 4F 40
00100C00  EF 00 FF 00 FF 00 7F 60 2F 20 FF 00 FF 00 DF 00
00100C10  FF C0 FF 00 FF 00 3F 80 1F 10 FF 00 FF 00 9F E0
00100C20  EF 10 FF 00 FF 00 FF 00 FF 00 FF 00 FF 00 DF 00
00100C30  FF 40 FF 00 FF 00 EF 10 FF 00 FF 00 FF 00 7F 00
00100C40  1F C0 FF 00 FF 00 BF 50 3F 00 FF 00 FF 00 BF 40
00100C50  FF 60 FF 00 FF 00 DF E0 FF 00 FF 00 FF 00 7F 80
00100C60  9F 70 FF 00 FF 00 FF 80 1F C0 FF 00 FF 00 EF 50
00100C70  4F 01 FF 00 FF 00 5F 20 AF D0 FF 00 FF 00 DF 40
00100C80  FF 70 FF 00 FF 00 3F 10 FF 10 FF 00 FF 00 4F D0
00100C90  7F 40 FF 00 FF 20 AF 00 7F 10 FF 00 FF 00 8F 70
00100CA0  77 00 FF 20 FF 00 7F C0 7F 20 FF 00 FF 00 6F 40
00100CB0  FF 40 FF 00 FF 00 AF 10 5F 10 FF 00 FF 00 FF 00
00100CC0  FF 50 FF 00 FF 00 7F 20 4F 00 FF 00 FF 00 2F E0
00100CD0  9F 10 FF 00 FF 00 CF 50 DF 40 FF 00 FF 00 BF 80
00100CE0  EF 50 FF 00 FF 00 7F 60 FF 30 FF 00 FF 00 7F A0
00100CF0  5F 50 FF 00 FF 00 6F 30 FF 10 FF 00 FF 00 6F A0
00100D00  FF 10 FF 00 FF 00 7F 10 FF 20 FF 00 FF 00 FF 00
00100D10  FF 50 FF 00 FF 00 DF 00 BF C0 FF 00 FF 00 3F 00
00100D20  FF 80 FF 00 FF 00 DF 00 FF 10 FF 00 FF 00 FF 10
00100D30  2F 40 FF 00 FF 00 7F 40 BF 20 FF 00 FF 00 6F 00
00100D40  EF 00 FF 00 FF 00 9F 00 FF 90 FF 00 FF 00 AF A0
00100D50  4F 20 FF 00 FF 00 7F 10 BF 70 FF 00 FF 00 BF 40
00100D60  EF C0 FF 00 FF 00 FF 80 FF 40 FF 00 FF 00 7F 30
00100D70  FF 00 FF 00 FF 00 AF 00 DF A0 FF 00 FF 00 4F 40
00100D80  DF 60 FF 00 FF 00 6F 80 AF 40 FF 00 FF 00 AF B0
00100D90  D7 90 FF 00 FF 00 DF 10 FF D0 FF 00 FF 00 FF 40
00100DA0  7E 90 FF 00 FF 00 7F 90 B7 80 FF 00 7F 00 DF C0
00100DB0  DF 20 FF 00 FF 00 5F 00 EF B0 FF 00 FF 00 FF 50
00100DC0  DF 20 FF 00 FF 00 0F 50 7F 00 FF 00 FF 00 6F 20
00100DD0  AF 00 FF 00 FF 00 5F 14 8F E0 FF 00 FF 00 4F 30
00100DE0  6F 00 FF 00 FF 00 EF 00 4F D0 FF 00 FF 00 6F 00
00100DF0  BF 20 FF 00 FF 00 5F A0 EF B0 FF 00 FF 00 5F 00
 -- Done
*/


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
	hd_cmd->count = sizeof ( struct hd_status );	/* XXX */
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
}

/* 4-15-2022, get serious with a logic analyzer.
 *
 * On a 20 pin chip like an LS245, pin 10 is ground,
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
 * I try my other memory card and indeed, the problem is solved.
 * The other (good) card has only 64K of ram via 4116 chips,
 * but a little memory that works is better than a lot that doesn't.
 * Now I see the 9 pulses with Mrdc going low (hence reading).
 * Then 120 us later I see 12 pulses with Mwtc going low (hence writing).
 * Then 190 us later I see 12 pulses again, also writing.
 *
 * 12 pulses would account for the status block being written, but why twice?
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

	hd_test1 ();

	// scope1 ();
	// scope2 ();
}

void
memset ( char *buf, int n, int val )
{
	while ( n-- )
	    *buf++ = val;
}

/* THE END */
