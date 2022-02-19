Callan boot ROMSs

The CPU board has 4 roms in sockets.

Sockets 1 and 3 are the basic boot ROM, which is held
in a pair of 2732 devices (for a total of 8K)

Sockets 2 and 4 hold what I call an "extension ROM"
in a pair of 2764 devices (for a total of 16K)

The boot ROMs are provided by Pacific Microsystems and
probably came with the CPU board.  The extension ROMs
were added by Callan and know about the "CWC" to boot.
The "CWC" is the "Callan Winchester Controller", which
was a separate multibus board.  Probably there is code
also for the CFC (Callan Floppy Controller), which was
yet another board.

My game is to disassemble and annotate these ROMS.
The files you want to look at are:


callan13.txt

callan24.txt
