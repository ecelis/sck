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
import syslog
import MySQLdb as db


def get_address_book():
    try:
        dbname = config.get("database", "name")
        dbuser = config.get("database", "user")
        dbpasswd = config.get("database", "passwd")
        dbhost = config.get("database", "host")
        # TODO Make it DB agnostic
        query = ("SELECT id,extension,cid FROM address_book")
        conn = db.connect(user=dbuser, passwd=dbpasswd, host=dbhost,
            db=dbname)
        cursor = conn.cursor()
        cursor.execute(query)
        data = cursor.fetchall()
        address_book = dict((id,(str(extension),cid)) for id,extension,cid in data)
        return address_book
    except:
	syslog.syslog(syslog.LOG_ERR, "Config Address Book Error," )
        pass

def get_sipcfg():
    sipcfg = None
    try:
        ext = config.get("sip", "ext")
        srv = config.get("sip", "srv")
        pwd = config.get("sip", "passwd")
        _sipcfg = dict([('ext', ext), ('srv', srv), ('pwd', pwd)])
        if not _sipcfg == None:
            sipcfg = dict((k,v) for k,v in _sipcfg.iteritems())

        return sipcfg
    except:
	syslog.syslog(syslog.LOG_ERR,"Config SIP Account Error.")
	pass

def get_audio_settings():
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
    except:
	syslog.syslog(syslog.LOG_ERR,"Config Audio Error.")

try:
    config = ConfigParser.RawConfigParser()
    config.read([os.path.expanduser('~/settings/config.ini'),
        os.path.expanduser('~/sauron-com-kit/ve-phone/config.ini'),
	'config.ini']
    )
except:
    syslog.syslog(syslog.LOG_ERR, "Config Error,")
    pass


