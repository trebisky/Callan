Callan "Unistar" multibus machine

This is a multibus based computer based on a
Pacific Microsystems PM68K cpu board.

This project began with analyzing the bootroms and
learning how to use them to download and run code.

The project ended with reading the contents of the original
edition 7 unix from the hard drive, which is available below.
This is the first thing in the list, though it was the
culmination of the project.

1. Edition7 - the unix system files from the hard drive
1. Roms - read and disassemble boot roms
1. toS - C program to generate S records
1. Srecord - a tool to download S records
1. First - a first tiny test to download and run
1. libgcc - vital routines from libgcc in assembler
1. printf - set up a C development framework
1. ram - ram diagnostic
1. hd1 - first attempts at driver for the hard drive controller
1. cwc-firmware - analysis of firmware in the hard drive controller
1. hd2 - driver for the hard drive controller after I get good RAM card
1. uart - improved serial IO for interaction with python script
1. hd3 - hopefully the final hd driver and contents extraction
1. mfm - the mfm_dump tool to dump tracks from a Gesswein transitions file
1. ufs_read - tool to read/extract files the the callan disk image
