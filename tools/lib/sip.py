#!/usr/bin/env python3
"""
@file sip.py
@brief Python Implementation of SIP module
@author Ran Bao (ran.bao@wirelessguard.co.nz)
@date March, 2017

Python implementation of Serial Interface Protocol module
"""


def hexchar_to_int(ch):
    val = 0

    if ch >= '0' and ch <= '9':
        val = ord(ch) - ord('0')
    elif ch >= 'A' and ch <= 'F':
        val = ord(ch) - ord('A')
        val += 10
    elif ch >= 'a' and ch <= 'f':
        val = ord(ch) - ord('a')
        val += 10

    return val


class SerialInterfaceProtocol(object):
    """Serial Interface Protocol encoder and decoder"""
    SIP_CMD_SIZE = 16
    SIP_PAYLOAD_LENGTH = 20
    SIP_TOTAL_LENGTH = 5 + SIP_PAYLOAD_LENGTH * 2

    NONE = 0
    SFLAG = 1
    COMMAND = 2
    PAYLOAD_LENGTH_H = 3
    PAYLOAD_LENGTH_L = 4
    PAYLOAD_H = 5
    PAYLOAD_L = 6
    EFLAG = 7

    SIP_PAYLOAD_0_S = 0
    SIP_PAYLOAD_1_S = 1
    SIP_PAYLOAD_2_S = 2
    SIP_PAYLOAD_3_S = 3
    SIP_PAYLOAD_4_S = 4
    SIP_PAYLOAD_5_S = 5
    SIP_PAYLOAD_6_S = 6
    SIP_PAYLOAD_7_S = 7
    SIP_PAYLOAD_8_S = 8
    SIP_PAYLOAD_9_S = 9
    SIP_PAYLOAD_10_S = 10
    SIP_PAYLOAD_11_S = 11
    SIP_PAYLOAD_12_S = 12
    SIP_PAYLOAD_13_S = 13
    SIP_PAYLOAD_14_S = 14
    SIP_PAYLOAD_15_S = 15
    SIP_PAYLOAD_16_S = 16
    SIP_PAYLOAD_17_S = 17
    SIP_PAYLOAD_18_S = 18
    SIP_PAYLOAD_19_S = 19
    SIP_CMD_S = 20
    SIP_PAYLOAD_LENGTH_S = 21

    def __init__(self):
        self.buffer = ['0' for _ in range(self.SIP_TOTAL_LENGTH)]
        self.cbTable = [None for _ in range(self.SIP_CMD_SIZE)]

        self.buffer[0] = '<'
        self.buffer[-1] = '>'

        self.state = self.NONE
        self.payloadIndex = 0

    def registerCallbackObject(self, command, obj):
        """Bind function to a specific command"""
        self.cbTable[command] = obj

    def deregisterCallbackObject(self, command):
        self.cbTable[command] = None

    def set(self, symbol, value):
        """Set value in a specific field in buffer"""
        if symbol == self.SIP_CMD_S:
            octet = "{:01x}".format(value);
            self.buffer[1] = octet[0]
        elif symbol == self.SIP_PAYLOAD_LENGTH_S:
            octet = "{:02x}".format(value)
            self.buffer[2] = octet[0]
            self.buffer[3] = octet[1]
        elif symbol in range(self.SIP_PAYLOAD_0_S, self.SIP_PAYLOAD_19_S + 1):
            octet = "{:02x}".format(value)
            self.buffer[symbol * 2 + 4] = octet[0]
            self.buffer[symbol * 2 + 5] = octet[1]

    def get(self, symbol):
        """Get a value in a specific field in buffer"""
        if symbol == self.SIP_CMD_S:
            return hexchar_to_int(self.buffer[1])
        elif symbol == self.SIP_PAYLOAD_LENGTH_S:
            high = hexchar_to_int(self.buffer[2])
            low = hexchar_to_int(self.buffer[3])
            return (high << 4) | low
        elif symbol in range(self.SIP_PAYLOAD_0_S, self.SIP_PAYLOAD_19_S + 1):
            high = hexchar_to_int(self.buffer[symbol * 2 + 4])
            low = hexchar_to_int(self.buffer[symbol * 2 + 5])
            return (high << 4) | low

    def poll(self, ch):
        """The state machine that decode SIP message"""
        if ch == '<':
            self.state = self.SFLAG
            self.payloadIndex = 0

        if self.state == self.SFLAG:
            self.state = self.COMMAND

        elif self.state == self.COMMAND:
            self.buffer[1] = ch
            self.state = self.PAYLOAD_LENGTH_H

        elif self.state == self.PAYLOAD_LENGTH_H:
            self.buffer[2] = ch
            self.state = self.PAYLOAD_LENGTH_L

        elif self.state == self.PAYLOAD_LENGTH_L:
            self.buffer[3] = ch

            # discard invalid packet
            if self.get(self.SIP_PAYLOAD_LENGTH_S) >= self.SIP_PAYLOAD_LENGTH:
                self.state = self.NONE
            else:
                self.state = self.PAYLOAD_H

        elif self.state == self.PAYLOAD_H:
            self.buffer[self.payloadIndex * 2 + 4] = ch
            self.state = self.PAYLOAD_L

        elif self.state == self.PAYLOAD_L:
            self.buffer[self.payloadIndex * 2 + 5] = ch
            self.payloadIndex += 1

            # append ch to payload until reach the given length
            if self.payloadIndex < self.get(self.SIP_PAYLOAD_LENGTH_S):
                self.state = self.PAYLOAD_H
            else:
                self.state = self.EFLAG

        elif self.state == self.EFLAG:
            if ch == '>':
                # execute code
                command = self.get(self.SIP_CMD_S)
                if self.cbTable[command] != None:
                    self.cbTable[command]()

            self.state = self.NONE

        elif self.state == self.NONE:
            pass
        else:
            pass


if __name__ == "__main__":
    pass
