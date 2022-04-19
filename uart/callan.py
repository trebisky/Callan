#!/usr/bin/python3

# callan.py
#
# Python code to talk to my stub code running on the Callan.
# the purpose is to read all the blocks from my Rodime hard drive.
#
# Tom Trebisky 4-19-2022

import os
import sys
import serial
import time

device = "/dev/ttyUSB0"
#device = "/dev/ttyUSB1"
baud = 9600
# It seems that a 1 second timeout is the minimum
read_timeout = 0.1

def flush () :
    n = 0
    while True :
        n += 1
        #c = ser.read().decode('ascii')
        c = ser.read()
        if len(c) == 0 :
            break
    return n

# Here is what I see (talking to the callan monitor)
#
# Got:  1 13
# Got:  1 10
# Got:  1 62 >
#
# So, the return I send gets echoed as a \r\n
# then I see the single character prompt

def probe_ORIG () :
    cmd = "\r"
    ser.write ( cmd.encode('ascii') )

    #print ( "\nprobe", end='' )
    print ( "\nprobe" )
    while True :
        c = ser.read().decode('ascii')
        if len(c) == 0 :
            break
        h = ord(c[0])
        n = len(c)
        print ( "Got: ", n, h, c )
    print ( "\n - done" )

# after learning from the above, I wrote this
def probe () :
    cmd = "\r"
    ser.write ( cmd.encode('ascii') )
    resp = ""

    while True :
        c = ser.read().decode('ascii')
        if len(c) == 0 :
            break
        resp += c

    #print ( "Got: ", resp )

    if len(resp) == 0 :
        last = '+'
    else :
        last = resp[-1]
    #print ( "Last: ", last )

    return last

def log_sector ( info ) :
    log_file = open("Callan.log", "a") 
    log_file.write ( info )
    log_file.close()

def one_sector ( c, h, s ) :

    #cmd = "r 3 4 5\r"
    cmd = "r "
    cmd += str(c)
    cmd += " "
    cmd += str(h)
    cmd += " "
    cmd += str(s)
    cmd += "\r"

    ser.write ( cmd.encode('ascii') )
    line = ""
    log = ""

    while True :
        c = ser.read().decode('ascii')
        if len(c) == 0 :
            break
        if c == '\n' :
            log += line
            log += '\n'
            print ( line )
            line = ""
        else :
            line += c

    # The last line is usually the prompt,
    # which there is no point in printing.
    if line[-1] != '%' :
        print ( line )
        log += line
        log += '\n'

    log_sector ( log )

def run_stuff () :
    cyl = 99
    head = 2
    #for s in range(17) :
    for s in range(4) :
        one_sector ( cyl, head, s )

ser = serial.Serial ( device, baud, timeout=read_timeout )
print ( "Using port " + ser.name )
print ( " baud rate: {}".format(baud) )

flush ()
print ( "Flush done" )

status = False
while True :
    print ( "Probing ..." )
    x = probe ()
    if x == '>' :
        print ( "Talking to Callan Monitor" )
        break
    if x == '%' :
        status = True
        break
    time.sleep ( 0.5 )

if status == False :
    print ( "Abandoning ship" )
    ser.close ()
    exit ()

print ( "Run the command" )
run_stuff ()

ser.close()

# THE END
