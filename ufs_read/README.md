ufs_read

This is a tool to read/extract the contents of the Callan disk image.

The Callan was an edition 7 unix system running on a Motorola mc68010
processor.  This means that it is big endian.
It also means that the disk contains two "old style"
unix file systems.

This program negotiates the unix file system, copying the file
structure to a directory on a linux system.
