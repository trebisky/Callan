My first code for the Callan

This began as Heurikon/blink1, which was some code to run
on the Heurikon HK68/ME board that I developed back in 2013.

This code was loaded into the Callan using a serial cable talking
to the boot rom.  I didn't have S-record code running yet,
so I just poked it in 16 bytes at a time using the bootrom "E"
command, which was fine for an initial test.
