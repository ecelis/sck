# Copyright 2013 - 2019 Ernesto Celis

# This file is part of SOS Communications Kit (SCK).

# SCK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# SCK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with SCK.  If not, see <https://www.gnu.org/licenses/>.

import os
import sys
import syslog
import wiringpi2 as wp

# Define what value is HIGH and LOW for the board pinout
HIGH = 1
LOW = 0
# Defines if 0 or 1 received in the pin triggers actions
TRIG = 0
# Time in ms the delay function will use
DELAY = 50
# TODO The dictionary should come from configuration file to allow
# easy configuration of pinout
PINOUT = {
    'ext1':23,
    'ext2':19,
    'ext3':13,
    'ext4':15,
    'ext5':22,
    'siren':14,
    'spk':48
}
# The library should be initialized before use
wp.wiringPiSetup()

""" Read input from pins """
def read_input():
    for ext, pin in PINOUT:
        if wp.digitalRead(pin) == TRIG:
            return ext
        else:
            return None


""" Changes the state of one pin HIGH = 1 LOW = 0 returns the state """
def change_state(pin_number, state):
    wp.digitalWrite(pin_number, state)
    return state


def delay():
    wp.delay(DELAY)
