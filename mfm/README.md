mfm

This is my mfm_dump project

I used the David Gesswein MFM emulator to read my Callan Unistar hard drive.
It has an odd format, so I couldn't immediately read it to a disk image
as I would have liked to.  But I could certainly read and save the MFM
transitions file.

Now my job is to learn enough to concoct a custom format for the drive.
In order to learn enough to do that, I decided to study his code and
write this "track dumper" that reads data for a single track from
the transitions file and dumps it in a format that I can understand.
