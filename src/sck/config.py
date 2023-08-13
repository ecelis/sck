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
        self.flavor = ['pc', 'cb2', 'ct']
        self.digt = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
        self.pinout = ['ext1', 'ext2', 'ext3', 'ext4', 'ext5', 'siren', 'spk']
        self.log_level = 3

    def set_config(self, config: configparser.ConfigParser):
        """Set the config parser"""
        self.config = config

    def set_log_level(self, level: int):
        """Set the log level, 1, 2 or 3 less to more verbose"""
        self.log_level = level

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
