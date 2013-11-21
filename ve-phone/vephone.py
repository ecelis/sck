# ValkEye SIP Phone, vephone.py
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

import sys
import pjsua as pj
import threading
import ConfigParser as cp
#import mysql.connector as mysql
#from mysql.connector import errorcode
import syslog

# Logging callback
def log_cb(level, str, len):
    print str,


def main_loop():
    while True:
	syslog.syslog(syslog.LOG_INFO, "Ready!")
	#TODO Nobody is going to see the menu, so get rid of it
        #print "1 AMBULANCE"
	#print "2 FIRE DEPARTMENT"
	#print "3 POLICE"
	#print "4 C. PROTECTION"
	#print "5 WOMEN SERVICES"
	#print ""
        #print "Press <ENTER> to quit"
        # TODO Put the addressbook in DB       
	try:
            getch = _Getch()
            choice = getch()
	    if choice == "1":
		syslog.syslog(syslog.LOG_INFO, "Dial AMBULANCE")
		# TODO catch exceptions and send those to syslog
		call = acc.make_call("sip:1001@172.20.10.35", VeCallCallback())
            elif choice == "2":
		syslog.syslog(syslog.LOG_INFO,"Dial FIRE DEPT")
		call = acc.make_call("1001@172.20.10.35", VeCallCallback())
	    elif choice == "3":
		syslog.syslog(syslog.LOG_INFO,"Dial POLICE")
		call = acc.make_call("sip:1001@172.20.10.35", VeCallCallback())
	    elif choice == "4":
		syslog.syslog(syslog.LOG_INFO,"Dial C. PROTECTION")
		call = acc.make_call("sip:1001@172.20.10.35", VeCallCallback())
	    elif choice == "5":
		syslog.syslog(syslog.LOG_INFO,"Dial WOMEN")
		call = acc.make_call("sip:1001@172.20.10.35", VeCallCallback())
	    elif choice == "*":
		syslog.syslog(syslog.LOG_INFO,"Dial LOCAL MIC")
                call = acc.make_call("sip:1001@172.20.10.35", VeCallCallback())
	    elif choice == "T":
		# Test only option, do not use it for real services!
		syslog.syslog(syslog.LOG_INFO,"Dial TEST")
		call = acc.make_call("sip:1001@192.168.1.71", VeCallCallback())
	    elif choice == "Q":
		syslog.syslog(syslog.LOG_NOTICE,"Exit on user request!")
		sys.exit(0)
	    else:
		syslog.syslog(syslog.LOG_NOTICE,"Invalid input, this is weird!")
		pass 

	except ValueError:
            syslog.syslog(syslog.LOG_NOTICE,"Invalid input, this is weird!")
	    continue

	    #return choice


class _Getch:
    """Gets a single character from standard input.  Does not echo to the
screen."""
    def __init__(self):
        try:
            self.impl = _GetchWindows()
        except ImportError:
            self.impl = _GetchUnix()

    def __call__(self): return self.impl()


class _GetchUnix:
    def __init__(self):
        import tty, sys

    def __call__(self):
        import sys, tty, termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

""" Maybe I'll never use the windows class, but still useful to get it
in here, just in case, taken from:
http://code.activestate.com/recipes/134892-getch-like-unbuffered-character-reading-from-stdin/ """
class _GetchWindows:
    def __init__(self):
        import msvcrt

    def __call__(self):
        import msvcrt
        return msvcrt.getch()


# Callback for handling registration
class VeAccountCallback(pj.AccountCallback):
    sem = None

    def __init__(self, account):
        pj.AccountCallback.__init__(self, account)

    def wait(self):
	self.sem = threading.Semaphore(0);
	self.sem.acquire()

    def on_reg_state(self):
	if self.sem:
            if self.account.info().reg_status >= 200:
		syslog.syslog(syslog.LOG_INFO,"SIP Client registered succesfully on PBX")
		self.sem.release()

    def on_incoming_call(self, call):
	#TODO A lot of stuff, call handling mainly and logging also
	global current_call
        current_call = call
	call_cb = VeCallCallback(current_call)
	current_call.set_callback(call_cb)
	#current_call.answer(180)

	if current_call:
	    call.answer(200)
	    return

        #if not current_call:
	#    current_call.answer(200)

        

# Callback to receive events from Call
class VeCallCallback(pj.CallCallback):
    def __init__(self, call=None):
        pj.CallCallback.__init__(self, call)

    # Notification when call sate has changed
    def on_state(self):
	global current_call
        syslog.syslog(syslog.LOG_INFO,"Call is " + self.call.info().state_text)
        syslog.syslog(syslog.LOG_INFO, "last code =" + str(self.call.info().last_code))
        syslog.syslog(syslog.LOG_INFO, "(" + self.call.info().last_reason + ")")

	if self.call.info().state == pj.CallState.DISCONNECTED:
            current_call = None
	    #TODO syslog.syslog(syslog.LOG_INFO, "Current call is " + current_call

    # Notification when call's media state changed
    def on_media_state(self):
        global lib
        if self.call.info().media_state == pj.MediaState.ACTIVE:
            # Connect the call to sound device
            call_slot = self.call.info().conf_slot
            lib.conf_connect(call_slot, 0)
            lib.conf_connect(0, call_slot)
            syslog.syslog(syslog.LOG_INFO,"Active call media state")


try:
    # Create library instance
    lib = pj.Lib()
    config = cp.RawConfigParser()
    # TODO Connect to DB
    #cnx = mysql.connect(user='valkeye', password='eyevalk', host='localhost',
	#	    database='valkeye_db')
    #cursor = cnx.cursor()
    #query = ("SELECT ext FROM adress_book")
    #cursor.execute(query)
    # Init library with default config
    lib.init(log_cfg = pj.LogConfig(level=3, callback=log_cb))
    # Create UDP transport which listens to any available port
    transport = lib.create_transport(pj.TransportType.UDP)
    # Start the library
    lib.start()
    # Create local/user-less account
    # userless 
    #acc = lib.create_account_for_transport(transport)
    acc = lib.create_account(pj.AccountConfig("192.168.1.71","2002", "3y3Ext1"))
    acc_cb = VeAccountCallback(acc)
    acc.set_callback(acc_cb)
    acc_cb.wait()
    # main loop
    main_loop()
    # We're done, shutdown the library
    lib.destroy()
    lib = None

except pj.Error, e:
    syslog.syslog(syslog.LOG_ERR,"Exception: " + str(e))
    lib.destroy()
    lib = None
    sys.exit(1)


