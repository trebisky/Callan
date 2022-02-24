/* On reset the 680x0 fetches first a long word
 * to be used as the initial stack pointer, then
 * a long word to be used as the initial PC.
 * This worked fine 11-7-2006 and ran my LED
 * blink test, first time !!
 */
	.text

/* A Callan extension rom sits at address 0x400000
 * and must begin with 2e7c 0001
 * This is what we see in the current Callan ROM.
 * The assembler is smart enough to convert
 * this to a moveal
	moveal #0x1fffe,%sp
 */
	movel #0x1fffe,%sp

/* this would be right for the boot rom, but not
 * for an extension rom.
	.long	0x00080000
	.long	0x00FC0008
 */

/* Lets run C code
 */
	bsr	start

loop:	bra	loop
