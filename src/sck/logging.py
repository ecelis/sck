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

import logging
from sck.settings import Config


class Logging():
    """SCK Logger"""

    def __init__(self) -> None:
        self.logging = logging
        config = Config()
        self.logging.basicConfig(level=config.log_level)

    def get_logger(self):
        """Return logger instance"""
        return self.logging.getLogger(__name__)
