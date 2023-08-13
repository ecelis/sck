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
import pjsua2 as pj
import sck.getchar as get_char
# import sck.wiringcb as wiringcb
import sck.gpio_api as gpio_api
from sck.endpoint import Endpoint
from sck.logging import Logging
from sck.settings import AppConfig, Config


class Application():
    """Main SCK Application class"""

    def __init__(self, config, read_input) -> None:
        self.loging = Logging()
        log = self.get_logger()
        log.info('App init')
        # Instantiate endpoint
        self.ep = Endpoint()
        self.ep.libCreate()

        # Default config
        self.app_config = AppConfig()
        self.app_config.epConfig.uaConfig.threadCnt = 1
        self.app_config.epConfig.uaConfig.mainThreadOnly = False
        # self.appConfig.epConfig.logConfig.writer = self.logger
        # self.appConfig.epConfig.logConfig.filename = "sck.log"
        # self.appConfig.epConfig.logConfig.fileFlags = pj.PJ_O_APPEND
        # self.appConfig.epConfig.logConfig.level = 5
        # self.appConfig.epConfig.logConfig.consoleLevel = 5

        # Initialize library
        _ua = "sck-" + self.ep.libVersion().full
        self.app_config.epConfig.uaConfig.userAgent = _ua
        self.ep.libInit(self.app_config.epConfig)

    def get_logger(self):
        """Grab the logger instance"""
        return self.loging.get_logger()

    def start(self):
        """Launch SCK"""
        print('Ignition!')


def main() -> int:
    """Start SCK"""
    print("eloh")
    config = Config()
    hardware = config.get_flavor()
    if_read = None
    if hardware in config.flavor:
        if hardware == 'pc':
            if_input = get_char
        # elif machine == 'cb2':
        #     if_input = wiringcb
        elif hardware == 'ct':
            if_input = gpio_api
        if_read = if_input.ReadInput()
        app = Application(config, if_read)
        app.start()
    else:
        # TODO log err
        print('OMG!')
        return -1
    print('Bye!')
    return 0


if __name__ == '__main__':
    sys.exit(main())
