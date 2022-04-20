This is a continuation of my work with the Callan Hard Drive controller

The game here is to write some stub code to run on the Callan that
acts as though it is reading sectors.

Then I work up a serial protocol and a python script that uses
that protocol to read sectors and record their contents.

This is based on the hd2 project, but without real code
to talk to the disk controller, it is all about the
protocol.  After this, we will move on to hd3.
