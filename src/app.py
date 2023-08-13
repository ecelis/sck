"""
Simple Communications Kit (SCK).

Copyright 2013 - 2023 Ernesto Celis

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
"""

import sys
from sck.config import Config
import sck.getchar as get_char
import sck.wiringcb as wiringcb
import sck.gpio_api as gpio_api


def main() -> int:
    """Start SCK"""
    config = Config()
    machine = config.get_flavor()
    if_read = None
    if machine in config.flavor:
        if machine == 'pc':
            if_input = get_char
        elif machine == 'cb2':
            if_input = wiringcb
        elif machine == 'ct':
            if_input = gpio_api
        if_read = if_input.ReadInput()
    else:
        # TODO log err
        return -1
    return 0


if __name__ == '__main__':
    sys.exit(main())
