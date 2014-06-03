# ValkEye SIP Phone, veconfig.py
# Ernesto Celis <developer@celisdelafuente.net>
# Nov. 2013
#
# This program is distributed in the hope that it will be useful,
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
from syslog import LOG_INFO as log_info
from syslog import LOG_ERR as log_err

def get_sipcfg():
    sipcfg = None
    logger(log_info, "SCK Trying to register in PBX")
    try:
        ext = config.get("sip", "ext")
        srv = config.get("sip", "srv")
        pwd = config.get("sip", "passwd")
        sipcfg = dict([('ext', ext), ('srv', srv), ('pwd', pwd)])
        logger(log_info,
                "SCK SIP Account Credentials, " + ext + "@" + srv)
        return sipcfg

    except:
        logger(log_err,
                "SCK Error while reading SIP Auth Credentials")


def get_speedial():
    speedial = None
    try:
        ext1 = config.get("speedial", "ext1")
        ext2 = config.get("speedial", "ext2")
        ext3 = config.get("speedial", "ext3")
        ext4 = config.get("speedial", "ext4")
        ext5 = config.get("speedial", "ext5")
        speedial = dict([('ext1', ext1), ('ext2', ext2),
            ('ext3', ext3), ('ext4', ext4), ('ext5', ext5)])
        return speedial

    except:
        logger(log_err, "SCK Can't Load Speed Dial Extensions")


def get_audiocfg():
    audiocfg = None
    try:
        master = config.get("audio", "master")
        pcm = config.get("audio", "pcm")
        capture = config.get("audio", "capture")
        cap_idx = config.get("audio", "cap_idx")
        input_src = config.get("audio", "input_src")
        in_idx = config.get("audio", "in_idx")
        mic = config.get("audio", "mic")
        mic_boost = config.get("audio", "mic_boost")
        audiocfg = dict([('master',master), ('pcm',pcm),
            ('capture',capture), ('cap_idx',cap_idx),
            ('input_src',input_src), ('in_idx',in_idx),
            ('mic',mic), ('mic_boost',mic_boost)])
        return audiocfg

    except:
        logger(log_err,"SCK Config Audio Error.")


try:
    config = ConfigParser.RawConfigParser()
    config.read([os.path.expanduser('~/settings/config.ini'),
	os.path.expanduser('~/.veconfig.ini'),
        os.path.dirname(os.path.realpath(__file__)) + '/config.ini',
        os.path.expanduser('~/sauron-com-kit/ve-phone/config.ini'),
        'config.ini']
    )

except:
    logger(log_err, "SCK General Config Exception,")

