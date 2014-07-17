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
import platform as pf
import sys
import pjsua as pj
import threading
from syslog import syslog as logger
from syslog import LOG_INFO as log_info
from syslog import LOG_ERR as log_err
import veconfig as vc

LOG_LEVEL = 3

# Detect platform flavor currently supported are pc, cubieboard2 and cubietruck
FLAVORS = ['pc','cubieboard2','cubietruck']
_platform = vc.get_flavor()
#if LOG_LEVEL < 2:
logger(log_info, _platform)
if _platform in FLAVORS:
    if _platform == 'pc':
        import asgetch as vi
    elif _platform == 'cubieboard2':
        import vewiring as vi
    elif _platform == 'cubietruck':
        import vegpio as vi
else:
    logger(log_err, u'SCK Unsupported platform')
    sys.exit(1)


# Logging callback
def log_cb(level, str, len):
    logger(log_info,"PJSUA " + str),


def main_loop():
    logger(log_info, "SCK Ready!")
    while True:

        try:
            # Read only one character from standard input
            ri = vi.read_input()
            #choice = ri()
            # Special options are handled by *,-,+ and / characters
            if ri() == "*":
                # * enable local audio
                logger(log_info,
                        "SCK Toggle Local MIC")
                # TODO
            elif ri() == "+":
                # Test only option, do not use it for real services!
                logger(log_info,
                        "SCK Dialing TEST")
                make_call("sip:1106@sip.sdf.org")
            elif ri() == "-":
                # TODO reserved
                logger(log_info,
                        "SCK - Action Reserved")
            elif ru() == "/":
                # Exit manually
                logger(log_info,
                        "SCK Exit on user request!")
                return
            else:
                for extension in speedial:
                    # TODO FIX this, so ugly hack
                    if extension == "ext1":
                        make_call('sip:' + speedial['ext1'] + 
                                '@' + sipcfg['srv'])
                        logger(log_info, 
                                "SCK Dialing " + extension)
                    elif extension == "ext2":
                        make_call('sip:' + speedial['ext2'] + 
                                '@' + sipcfg['srv'])
                        logger(log_info, 
                                "SCK Dialing " + extension)
                    elif extension == "ext3":
                        make_call('sip:' + speedial['ext3'] + 
                                '@' + sipcfg['srv'])
                        logger(log_info, 
                                "SCK Dialing " + extension)
                    elif extension == "ext4":
                        make_call('sip:' + speedial['ext4'] + 
                                '@' + sipcfg['srv'])
                        logger(log_info, 
                                "SCK Dialing " + extension)
                    elif extension == "ext5":
                        make_call('sip:' + speedial['ext5'] + 
                                '@' + sipcfg['srv'])
                        logger(log_info, 
                                "SCK Dialing " + extension)

        except ValueError:
            logger(log_info,
                    "SCK Exception, this is weird!")

	    continue


""" Make a call to specified SIP URI """
def make_call(uri):
    try:
        logger(log_info, "SCK ("+uri+")")
        call = acc.make_call(uri, VeCallCallback())
        return call
    except pj.Error, e:
        logger(log_err, "SCK " + str(e))
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
                logger(log_err,
                        'SCK registration status ' +
                        str(self.account.info().reg_status) + ' ' +
                        self.account.info().reg_reason
                )

            self.sem.release()


    def on_incoming_call(self, call):
	#TODO A lot of stuff, call handling mainly and logging also
        logger(log_info, "SCK Incoming call from " + 
                call.info().remote_uri
        )
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


    def on_state(self):
        global current_call
        logger(log_info,
                "SCK Call is " + self.call.info().state_text
        )
        logger(log_info, " Last code = " +
                str(self.call.info().last_code)
        )
        logger(log_info,
                " (" + self.call.info().last_reason + ")"
        )

        global call_state
        call_state = self.call.info().state

        #global tone
        if call_state == pj.CallState.EARLY:
            #tone = VeTone().ring_start()
            pass
        elif call_state == pj.CallState.DISCONNECTED:
            #VeTone().ring_stop(tone)
            current_call = None

    # Notification when call's media state changed
    def on_media_state(self):
        global lib
        if self.call.info().media_state == pj.MediaState.ACTIVE:
            # Connect the call to sound device
            call_slot = self.call.info().conf_slot
            lib.conf_connect(call_slot, 0)
            lib.conf_connect(0, call_slot)


try:
    # Get PBX/SIP username/extension, PBX server and password
    sipcfg = vc.get_sipcfg()
    # Get Speed Dial Extensions
    speedial = vc.get_speedial()
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
    # Set sound device TODO in vc.py
    #lib.set_snd_dev(0,0)
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
                pj.AccountConfig(sipcfg['srv'], sipcfg['ext'],
                    sipcfg['passwd'])
        )
    # Set the account call back
    acc_cb = VeAccountCallback(acc)
    acc.set_callback(acc_cb)
    acc_cb.wait()
    # Global variables
    ve_local_audio = False
    ve_call = None
    # main loop
    main_loop()
    # We're done, shutdown the library
    lib.destroy()
    lib = None
    sys.exit(0)

except pj.Error, e:
    logger(log_err, "SCK Exception: " + str(e))
    lib.destroy()
    lib = None
    sys.exit(1)
