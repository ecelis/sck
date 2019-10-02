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
import ConfigParser
import os
from syslog import syslog as logger
from syslog import LOG_ERR


def get_flavor():
    try:
        return config.get('default', 'flavor')
    except:
        logger(LOG_ERR, 'SCK Failed to detect flavor')


def get_sipcfg():
    try:
        return dict(config.items('sip'))

    except:
        logger(LOG_ERR, "SCK Error while reading SIP Auth Credentials")


def get_speedial():
    try:
        return dict(config.items('speedial'))

    except:
        logger(LOG_ERR, "SCK Can't Load Speed Dial Extensions")


try:
    config = ConfigParser.RawConfigParser()
    #config.readfp(open('config.ini'))
    config.read([os.path.expanduser('~/.config/sck/config.ini'),
                 os.path.dirname(os.path.realpath(__file__)) + '/config.ini',
                 os.path.expanduser('/etc/sck/config.ini')]
                )

except:
    logger(LOG_ERR, "SCK General Config Exception,")
