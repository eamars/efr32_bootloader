#!/usr/bin/env python2

"""
@file ioserial.py
@brief Serial library for other test scripts
@author David Barclay
        Ran Bao (ran.bao@wirelessguard.co.nz)
@date March, 2017

For Mac OS and Linux
"""

from __future__ import print_function

import sys
import os
import serial
import time

## Finds serial device files from /dev
# @return   list of devices available in /dev
def findSerial():
    devices = []
    ls = os.listdir("/dev")
    for dev in ls:
        if (dev.startswith("ttyACM") or dev.startswith("tty.usbmodem")):
            devices.append("/dev/" + dev)
    return devices

## IOSerial - Main module class
# Acts as an interface to a serial file desriptor in /dev
class IOSerial(object):

    ## Class initializer
    # @param    port    the device file descriptor in /dev
    def __init__(self, port):
        self.serial = serial.Serial(baudrate=115200, bytesize=8, parity='N', stopbits=1)
        self.setPort(port)

    ## Sets the serial port to the specified file descriptor name
    # @param    port    the device file descriptor in /dev
    def setPort(self, port):
        self.serial.port = port

    ## Returns the file descriptor name of the current serial port
    #   @return the current serial port file name
    def getPort(self):
        return self.serial.port

    ## Attempts to open the serial port file. Does not return anything useful
    def attach(self):
        try:
            self.serial.open()
            print("Attached serial: " + self.getPort(), file=sys.stderr)
        except:
            return None
        self.serial.flushInput()

    ## Attempts to open the serial port. Blocks until opened, or timeout.
    # This function will not return until it has attached or
    #  the timeout is reached. By default this function has no timeout.
    # @param    timeout the number of seconds to wait before timing out
    # @return   bool - wether the attach was successful
    def blockingAttach(self, timeout=0):
        t = time.time()
        while (not self.isAttached()):
            self.attach()
            if ((timeout != 0) and (time.time() > (t + timeout))):
                break
            elif (not self.isAttached()):
                print('.', end='', file=sys.stderr)
                time.sleep(1)
        return self.isAttached()

    ## Determines if the port is currently open
    # @return   bool - True if the port is currently open
    def isAttached(self):
        return (self.serial.isOpen())

    ## Closes the serial port
    def detach(self):
        self.serial.close()
        print("Detached serial: " + self.getPort(), file=sys.stderr)

    ## Attempts to read a character from the serial port. If timeout is set 
    #   to none, returns instantly.
    # @param    timeout - number of seconds to wait for output
    # @return   string - a character, or '' if none available.
    def readchar(self, timeout=None):
        """Read one character from serial port"""
        self.serial.timeout = timeout
        try:
            ch = self.serial.read(1)
            self.serial.timeout = None
            return ch
        except serial.serialutil.SerialException:
            self.detach()
        return ''

    ## Attempts to read a line from the serial port. If timeout is set 
    #   to none, returns instantly.
    # @param    timeout - number of seconds to wait for output
    # @return   string - a string, or '' if none available.
    def readline(self, timeout=None):
        self.serial.timeout = timeout
        try:
            line = self.serial.readline()
            self.serial.timeout = None
            return line
        except serial.serialutil.SerialException:
            self.detach()
        return ''

    ## Writes a string to the serial device. If timeout=None, does not wait.
    # @param    the string to write
    # @param    timeout - The number of seconds to wait for the device. 
    def write(self, string, timeout=None):
        self.serial.timeout = timeout
        try:
            self.serial.timeout = None
            self.serial.write(string)
        except serial.serialutil.SerialException:
            self.detach()
        return None

    ## Writes a string to the serial device, with '\r\n' appended to the end.
    #   If timeout=None, does not wait.
    # @param    string
    # @param    timeout - The number of seconds to wait for the device. 
    def writeline(self, string, timeout=None):
        self.write(string + "\r\n", timeout=timeout)
        return None

