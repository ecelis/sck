# config.py is part of SCK
# Copyright 2013, 2014, 2015, 2016
# Ernesto Celis <developer@celisdelafuente.net>
#
# SCK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
#

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
    config.readfp(open('config.ini'))
    config.read([os.path.expanduser('/etc/sck/config.ini'),
                 os.path.expanduser('~/sck/config.ini'),
                 os.path.dirname(os.path.realpath(__file__)) + '/config.ini']
                )

except:
    logger(LOG_ERR, "SCK General Config Exception,")
