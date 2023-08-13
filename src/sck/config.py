"""
Config is part of Simple Communications Kit (SCK).

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

import configparser
import os


class Config():
    """SCK Config class"""
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super(Config, cls).__new__(cls)
            cls._instance.set_config(configparser.ConfigParser())
            cls._instance.config.read(
                [
                    os.path.expanduser('~/.config/sck/config.ini'),
                    os.path.dirname(os.path.realpath(__file__)) + '/config.ini',
                    os.path.expanduser('/etc/sck/config.ini')
                    ]
                    )

        return cls._instance

    def __init__(self):
        self.config = self._instance.get_config()

    def set_config(self, config):
        """Set the config parser"""
        self.config = config

    def get_config(self):
        """Set the config parser"""
        return self.config

    def get_flavor(self):
        """Return hardware architecture configuration"""
        return self.config.get('default', 'flavor')

    def get_sipcfg(self):
        """Return SIP settings"""
        return dict(self.config.items('sip'))

    def get_speedial(self):
        """Return speed dial extensions"""
        return dict(self.config.items('speedial'))
