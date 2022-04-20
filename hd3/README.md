hd3

This is hopefully the finished project.

This began as a copy of the "uart" project, but adds real code
to talk to the hard drive controller and drive.

The Callan.py script uses a serial protocol to talk to my code
on the Callan and read sectors from the drive.

The python script has the loops to traverse the drive and
most importantly can write to a "log" file on my linux
system that I will then use to regenerate a disk image.
