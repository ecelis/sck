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

import sys
import tty
import termios
# import msvcrt


class _GetchUnix:
    def __call__(self):
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch


class ReadInput:
    """Gets a single character from standard input.  Does not echo to the
    screen."""
    def __init__(self):
        try:
            self.impl = None  # _GetchWindows()
        except ImportError:
            self.impl = _GetchUnix()

    def __call__(self):
        return self.impl()


# class _GetchWindows:
#     """
#     Maybe I'll never use the windows class, but still useful to get it
#     in here, just in case, taken from:
#     http://code.activestate.com/recipes/134892-getch-like-unbuffered-character-reading-from-stdin/
#     The site states the license as PSF
#     """

#     def __call__(self):
#         return msvcrt.getch()
