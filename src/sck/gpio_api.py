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
from syslog import syslog as logger
from syslog import LOG_ERR
from sck.config import Config

INPUT = 0
OUTPUT = 1
#TODO PWM_OUTPUT = 2
PULLUP = 3
PULLDOWN = 4
PULLOFF = 5
#TODO CHECK = 6
#TODO GPIO_CLOCK = 7
LOW = 0
HIGH = 1


def get_ports():
    try:
        config = Config().get_config()
        config.readfp(open('exports.ini'))
        #TODO It could be a RawConfigParser to get the whole dictionary from cfg
        return dict([
            ('siren', config.get('features', 'sirena')),
            ('local_audio', config.get('features', 'llave')),
            ('button_1', config.get('features', 'boton1')),
            ('button_2', config.get('features', 'boton2')),
            ('button_3', config.get('features', 'boton3')),
            ('button_4', config.get('features', 'boton4')),
            ('button_5', config.get('features', 'boton5'))
            ])
    except:
        logger(LOG_ERR, 'SCK GPIO Expansion Ports Config Error')
        return None


def _read_port_value(gpio_value):
    with open(gpio_value, 'r', 0) as value:
        for line in value:
            return line.rstrip('\n')


def read_input(ports):
    port = {}
    try:
        for key, gpio in ports.iteritems():
            port[key] = key, _read_port_value(gpio + '/value')

        return port
    except:
        logger(LOG_ERR, 'SCK GPIO Read Error')
        return None
