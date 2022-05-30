ufs_read

This is a tool to read/extract files from an edition 7 unix filesystem.

It copies the files to a parallel structure on the local disk
(in my case on my linux system).

My purpose in writing this was to extract the contents of the Callan disk image.
Some aspects (file ownership, suid settings, special files) are rcorded in
comments that I grep from the output and save alongside the extracted file
contents.

The Callan was an edition 7 unix system running on a Motorola mc68010
processor.  This means that it is big endian.
It also means that the disk contains two "old style"
unix file systems.

This program negotiates the unix file system, copying the file
structure to a directory on a linux system.

