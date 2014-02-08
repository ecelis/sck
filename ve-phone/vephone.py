#!/usr/bin/env python
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

import os
import sys
import pjsua as pj
import threading
import syslog
import asgetch as gc
import veconfig
#import vess
#import vetone

LOG_LEVEL = 3
# Logging callback
def log_cb(level, str, len):
    syslog.syslog(syslog.LOG_INFO,"PJSUA " + str),


def main_loop():
    while True:
        syslog.syslog(syslog.LOG_INFO, "SCK Ready!")
        try:
            # Read only one character from standard input
            getch = gc._Getch()
            choice = getch()
            # Special options are handled by *,-,+ and / characters
            if choice == "*":
                # * enable local audio
                syslog.syslog(syslog.LOG_INFO,"SCK Toggle Local MIC")
                # TODO
            elif choice == "+":
                # Test only option, do not use it for real services!
                syslog.syslog(syslog.LOG_INFO,"SCK Dialing TEST")
                make_call("sip:1106@sip.sdf.org")
            elif choice == "-":
                # TODO reserved
                syslog.syslog(syslog.LOG_INFO,"SCK - Action Reserved")
            elif choice == "/":
                # Exit manually
                syslog.syslog(syslog.LOG_NOTICE,"SCK Exit on user request!")
                return
            elif choice == "1":
                syslog.syslog(syslog.LOG_INFO,"SCK Dialing 1")
                make_call("sip:1001@192.168.1.11")
            elif choice == "2":
                syslog.syslog(syslog.LOG_INFO,"SCK Dialing 2")
                make_call("sip:1001@192.168.1.11")
            elif choice == "3":
                syslog.syslog(syslog.LOG_INFO,"SCK Dialing 3")
                make_call("sip:1001@192.168.1.11")
            elif choice == "4":
                syslog.syslog(syslog.LOG_INFO,"SCK Dialing 4")
                make_call("sip:1001@192.168.1.11")
            elif choice == "5":
                syslog.syslog(syslog.LOG_INFO,"SCK Dialing 5")
                make_call("sip:1001@192.168.1.11")
            """else:
                for contact in address_book:
                    # address_book = {'1': ('1001', 'AMBULANCE'), ...}
                    if choice == str(contact):
                        uri = "sip:"+address_book[contact][0]+"@"+sipcfg['srv']
                        syslog.syslog(syslog.LOG_INFO, "SCK Dial "
                            + str(contact) + " " + address_book[contact][1])
                        # Call contact
                        make_call(uri)
            """

        except ValueError:
            syslog.syslog(syslog.LOG_NOTICE,"SCK Exception, this is weird!")

	    continue


def make_call(uri):
    try:
        syslog.syslog(syslog.LOG_INFO, "SCK ("+uri+")")
        call = acc.make_call(uri, VeCallCallback())
        return call
    except pj.Error, e:
        syslog.syslog(syslog.LOG_ERR, "SCK " + str(e))
        return None


""" Callback for handling registration on PBX """
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
                syslog.syslog(syslog.LOG_ERR, 'SCK registration status ' +
                        str(self.account.info().reg_status) + ' ' +
                        self.account.info().reg_reason)
		self.sem.release()

    def on_incoming_call(self, call):
	#TODO A lot of stuff, call handling mainly and logging also
        syslog.syslog(syslog.LOG_INFO, "SCK Incoming call from "
	    + call.info().remote_uri)
	global current_call
        #TODO global tone
	#tone = VeTone().ring_start()
        current_call = call
	call_cb = VeCallCallback(current_call)
	current_call.set_callback(call_cb)
	current_call.answer(200)

""" Class to receive events from Call """
class VeCallCallback(pj.CallCallback):

    def __init__(self, call=None):
        pj.CallCallback.__init__(self, call)

    """ Notification when call sate has changed """
    def on_state(self):
	global current_call
        syslog.syslog(syslog.LOG_INFO,"SCK Call is " + self.call.info().state_text)
        syslog.syslog(syslog.LOG_INFO, "    Last code = " + str(self.call.info().last_code))
        syslog.syslog(syslog.LOG_INFO, "    (" + self.call.info().last_reason + ")")

        global call_state
	call_state = self.call.info().state

        #global tone
	if call_state == pj.CallState.EARLY:
#	    tone = VeTone().ring_start()
            pass
	elif call_state == pj.CallState.DISCONNECTED:
	    #VeTone().ring_stop(tone)
            current_call = None

    # Notification when call's media state changed
    def on_media_state(self):
        global lib
#	global tone
        if self.call.info().media_state == pj.MediaState.ACTIVE:
            # Connect the call to sound device
#            VeTone().ring_stop(tone)
            call_slot = self.call.info().conf_slot
            lib.conf_connect(call_slot, 0)
            lib.conf_connect(0, call_slot)

class VeTone:
    def ring_start(self):
	global tone
	tone = lib.create_player(
	    os.path.dirname(os.path.realpath(__file__)) + "/sounds/tone.wav",
	    True
	)
	lib.conf_connect(tone, 0)
	return tone

    def ring_stop(self, tone):
	lib.conf_disconnect(tone, 0)
        lib.player_destroy(tone)

    def incoming(self):
	global tone
	tone = lib.create_player(
	    os.path.dirname(os.path.realpath(__file__)) + "/sounds/three-tones.wav",
	    False
	)
	lib.conf_connect(tone, 0)
	return tone


try:
    # Initialize ValkEye Sound System
    #audio = vess.VSS()
    # Get PBX/SIP username/extension, PBX server and password
    sipcfg = veconfig.get_sipcfg()
    # Get address book
    address_book = veconfig.get_address_book()
    # Media Config
    media = pj.MediaConfig()
    media.ec_options = 0 # pjsua default 0
    media.ec_tail_len = 256 # pjsua default 256
    media.no_vad = False # disable Voice Activity Detector
    media.enable_ice = True # Enable (ICE) Interactive Connectivity Establishment
    # Create pjsua library instance
    lib = pj.Lib()
    # Init pjsua with default config
    lib.init(log_cfg = pj.LogConfig(level=LOG_LEVEL, callback=log_cb))
   # Create UDP transport which listens to any available port
    transport = lib.create_transport(pj.TransportType.UDP)
    # Start the library
    lib.start()
    if sipcfg == None:
        # Create local/user-less account
        acc = lib.create_account_for_transport(transport)
    else:
        # Register on PBX
        acc = lib.create_account(
	    pj.AccountConfig(sipcfg['srv'], 
	    sipcfg['ext'], 
	    sipcfg['pwd']))
    # Set the account call back
    acc_cb = VeAccountCallback(acc)
    acc.set_callback(acc_cb)
    acc_cb.wait()
    # main loop
    main_loop()
    # We're done, shutdown the library
    lib.destroy()
    lib = None
    sys.exit(0)

except pj.Error, e:
    syslog.syslog(syslog.LOG_ERR,"SCK Exception: " + str(e))
    lib.destroy()
    lib = None
    sys.exit(1)
