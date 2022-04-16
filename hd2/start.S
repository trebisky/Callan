/* We don't need to do much as this is loaded
 * as S-records by the boot ROM, then started
 * at the first load address.
 * We just continue using the same stack that
 * the boot ROM was using.
 *
 * Tom Trebisky  2-27-2022
 */
	.text

/* Clear the bss area
 */
	movel	#__bss_start__, %sp@-
	movel	#__bss_end__, %sp@-
	jsr	bss_clear
	addql	#8, %sp


/* Lets run C code
 */
	bsr	start

/* we let this return to the bootrom should
 * it ever return.
 */
/*
loop:	bra	loop
*/
