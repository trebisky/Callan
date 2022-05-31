Edition 7

This is the entire contents of the Callan hard drive.
It had two partitions with files, the root, and the
"usr" partition.  The drive itself had 20M capacity.

This is a worthy example for study if you want to see what
a working edition 7 unix system (circa 1985) looked like.

This one ran on a Motorola mc68010 processor.

 * There are about 300 files on the root partition.
 * There are about 1300 files on the usr partition.

1. root.tar.gz - root partition (5M)
2. usr.tar.gz - usr partition (7M)

The following files give information that is not preserved
in the above collections (primarily because it is awkward
to do so and because I became lazy when writing ufs_read.c).
In the case of the /dev files, there is no other option.

1. Readme.links - files that had hard links (I just made separate copies)
2. Readme.own - original file ownership
3. Readme.special - special files (contained in the /dev directory)
