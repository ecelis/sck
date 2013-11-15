# ValkEye SIP Phone
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

# Logging callback
def log_cb(level, str, len):
    print str,


def show():
    while True:
        print "1 AMBULANCE"
	print "2 FIRE DEPARTMENT"
	print "3 POLICE"
	print "4 C. PROTECTION"
	print "5 WOMEN SERVICES"
	print ""
        print "Press <ENTER> to quit"
            
	try:
	    print "Type your choice: \r\n"
            getch = _Getch()
            choice = getch()
	    if choice == "1":
		print "Dial AMBULANCE"
	        call = acc.make_call(sys.argv[1], VeCallCallback())
            elif choice == "2":
		print "Dial FIRE"
	    elif choice == "3":
		pass
	    elif choice == "4":
		pass
	    elif choice == "5":
		pass
	    elif choice == "*":
		pass
	    elif choice == "Q":
		sys.exit(0)
	    else:
		pass 

	except ValueError:
            print "Invalid input. Try again."
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

# Maybe I'll never use the windows class, but still useful to get it
# in here, just in case, taken from:
# http://code.activestate.com/recipes/134892-getch-like-unbuffered-character-reading-from-stdin/
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
		self.sem.release()


# Callback to receive events from Call
class VeCallCallback(pj.CallCallback):
    def __init__(self, call=None):
        pj.CallCallback.__init__(self, call)

    # Notification when call sate has changed
    def on_state(self):
        print "Call is ", self.call.info().state_text,
        print "last code =", self.call.info().last_code,
        print "(" + self.call.info().last_reason + ")"

    # Notification when call's media state changed
    def on_media_state(self):
        global lib
        if self.call.info().media_state == pj.MediaState.ACTIVE:
            # Connect the call to sound device
            call_slot = self.call.info().conf_slot
            lib.conf_connect(call_slot, 0)
            lib.conf_connect(0, call_slot)
            print "Hello world, I can talk!"


try:
    # Create library instance
    lib = pj.Lib()
    config = cp.RawConfigParser()
#    cnx = mysql.connect(user='valkeye', password='0
    # Init library with default config
    lib.init(log_cfg = pj.LogConfig(level=3, callback=log_cb))
    # Create UDP transport which listens to any available port
    transport = lib.create_transport(pj.TransportType.UDP)
    # Start the library
    lib.start()
    # Create local/user-less account
    # userless 
    #acc = lib.create_account_for_transport(transport)
    acc = lib.create_account(pj.AccountConfig("192.168.1.120","2002", "3y3Ext1"))
    acc_cb = VeAccountCallback(acc)
    acc.set_callback(acc_cb)
    acc_cb.wait()
    # Show main menu
    show()
    # We're done, shutdown the library
    lib.destroy()
    lib = None

except pj.Error, e:
    print "Exception: " + str(e)
    lib.destroy()
    lib = None
    sys.exit(1)


