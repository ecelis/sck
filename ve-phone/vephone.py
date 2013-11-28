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
import vetone

LOG_LEVEL = 3
# Logging callback
def log_cb(level, str, len):
    syslog.syslog(syslog.LOG_INFO,str),


def main_loop():
    while True:
	syslog.syslog(syslog.LOG_INFO, "Ready!")
	try:
            getch = gc._Getch()
            choice = getch()
	    # Search the address book first, it only handles 0 to 1
            for c in address_book:
		# address_book = {'1': ('1001', 'AMBULANCE'), ...}
		if choice == c:
		    uri = "sip:"+address_book[c][0]+"@"+sipcfg['srv']
		    syslog.syslog(syslog.LOG_INFO, "Dial " + 
			str(c) + " " +
		        address_book[c][1])
		    make_call(uri)
            # Special options are handled by *,-,+ and / characters
	    if choice == "*":
	        # * enable local audio
		syslog.syslog(syslog.LOG_INFO," Enable Local MIC")
		# TODO
	    elif choice == "+":
		# Test only option, do not use it for real services!
		syslog.syslog(syslog.LOG_INFO,"Dial TEST")
		make_call("sip:1106@sip.sdf.org")
	    elif choice == "-":
		# TODO
		pass
	    elif choice == "/":
		# Exit manually
		syslog.syslog(syslog.LOG_NOTICE,"Exit on user request!")
		return
	    else:
		# anything else shouldn't be valid
		syslog.syslog(syslog.LOG_NOTICE,"Invalid input, this is weird!")

	except ValueError:
            syslog.syslog(syslog.LOG_NOTICE,"Invalid input, this is weird!")
	    continue


def make_call(uri):
    try:
        syslog.syslog(syslog.LOG_INFO, "("+uri+")")
        call = acc.make_call(uri, VeCallCallback())
	return call
    except pj.Error, e:
	syslog.syslog(syslog.LOG_ERR, str(e))
	return None


""" Callback for handling registration """
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
        syslog.syslog(syslog.LOG_INFO, "Incoming call from "
	    + call.info().remote_uri)
	global current_call

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
        syslog.syslog(syslog.LOG_INFO,"Call is " + self.call.info().state_text)
        syslog.syslog(syslog.LOG_INFO, "last code = " + str(self.call.info().last_code))
        syslog.syslog(syslog.LOG_INFO, "(" + self.call.info().last_reason + ")")

        global call_state
	call_state = self.call.info().state

        global tone
	if call_state == pj.CallState.EARLY:
	    tone = VeTone().ring_start()
	elif call_state == pj.CallState.INCOMING:
	    # TODO let it ring at least once before answer the call
	    #VeTone().ring_start()
	    pass
        elif call_state == pj.CallState.CONFIRMED:
	    VeTone().ring_stop(tone)
	elif call_state == pj.CallState.DISCONNECTED:
	    VeTone().ring_stop(tone)
            current_call = None

    # Notification when call's media state changed
    def on_media_state(self):
        global lib
        if self.call.info().media_state == pj.MediaState.ACTIVE:
            # Connect the call to sound device
            call_slot = self.call.info().conf_slot
            lib.conf_connect(call_slot, 0)
            lib.conf_connect(0, call_slot)

class VeTone:
    """ Start ring tone """
    def ring_start(self):
	global tone
	# Create the tone from sound file and loop forever
	tone = lib.create_player(
	    os.path.dirname(os.path.realpath(__file__)) + "/sounds/tone.wav",
	    True
	)
	# Connect ring tone to sound device
	lib.conf_connect(tone, 0)
	return tone

    """ Stop ring tone """
    def ring_stop(self, tone):
	if tone:
	    # disconnect tone from sound device
	    lib.conf_disconnect(tone,0)
	    # destroy tone
            lib.player_destroy(tone)


try:
    # Get PBX/SIP username/extension, PBX server and password
    sipcfg = veconfig.get_sipcfg()
    # Get address book
    address_book = veconfig.get_address_book()
    # Create audio tones instance
    #tone = vetone.Tone()
    # Create library instance
    lib = pj.Lib()
    # Init library with default config
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
    syslog.syslog(syslog.LOG_ERR,"Exception: " + str(e))
    lib.destroy()
    lib = None
    sys.exit(1)
