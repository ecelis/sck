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
import pjsua2 as pj


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


class SipTransportConfig:
    """Transport setting"""

    def __init__(self, type, enabled):
        # pj.PersistentObject.__init__(self)
        self.type = type
        self.enabled = enabled
        self.config = pj.TransportConfig()

    def readObject(self, node):
        child_node = node.readContainer("SipTransport")
        self.type = child_node.readInt("type")
        self.enabled = child_node.readBool("enabled")
        self.config.readObject(child_node)

    def writeObject(self, node):
        child_node = node.writeNewContainer("SipTransport")
        child_node.writeInt("type", self.type)
        child_node.writeBool("enabled", self.enabled)
        self.config.writeObject(child_node)

class AccConfig:
    """Account setting with buddy list"""

    def __init__(self):
        self.enabled = True
        self.config = pj.AccountConfig()
        self.buddyConfigs = []

    def readObject(self, node):
        acc_node = node.readContainer("Account")
        self.enabled = acc_node.readBool("enabled")
        self.config.readObject(acc_node)
        buddy_node = acc_node.readArray("buddies")
        while buddy_node.hasUnread():
            buddy_cfg = pj.BuddyConfig()
            buddy_cfg.readObject(buddy_node)
            self.buddyConfigs.append(buddy_cfg)

    def writeObject(self, node):
        acc_node = node.writeNewContainer("Account")
        acc_node.writeBool("enabled", self.enabled)
        self.config.writeObject(acc_node)
        buddy_node = acc_node.writeNewArray("buddies")
        for buddy in self.buddyConfigs:
            buddy_node.writeObject(buddy)


class AppConfig:
    """Master settings"""

    def __init__(self):
        self.epConfig = pj.EpConfig()	# pj.EpConfig()
        self.udp = SipTransportConfig(pj.PJSIP_TRANSPORT_UDP, True)
        self.tcp = SipTransportConfig(pj.PJSIP_TRANSPORT_TCP, True)
        self.tls = SipTransportConfig(pj.PJSIP_TRANSPORT_TLS, False)
        self.accounts = []		# Array of AccConfig

    def loadFile(self, file):
        json = pj.JsonDocument()
        json.loadFile(file)
        root = json.getRootContainer()
        self.epConfig = pj.EpConfig()
        self.epConfig.readObject(root)

        tp_node = root.readArray("transports")
        self.udp.readObject(tp_node)
        self.tcp.readObject(tp_node)
        if tp_node.hasUnread():
            self.tls.readObject(tp_node)

        acc_node = root.readArray("accounts")
        while acc_node.hasUnread():
            acfg = AccConfig()
            acfg.readObject(acc_node)
            self.accounts.append(acfg)
                    
    def saveFile(self, file):
        json = pj.JsonDocument()

        # Write endpoint config
        json.writeObject(self.epConfig)

        # Write transport config
        tp_node = json.writeNewArray("transports")
        self.udp.writeObject(tp_node)
        self.tcp.writeObject(tp_node)
        self.tls.writeObject(tp_node)

        # Write account configs
        node = json.writeNewArray("accounts")
        for acc in self.accounts:
            acc.writeObject(node)
                
        json.saveFile(file)
