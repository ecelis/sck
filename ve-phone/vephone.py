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

import os, sys, threading, vess
import platform as pf
import pjsua as pj
from syslog import syslog as logger
from syslog import LOG_INFO as log_info
from syslog import LOG_ERR as log_err
import veconfig as vc
import vegpio as gpio
#import vetone

LOG_LEVEL = 3
# Global variables
ve_call = None
ve_tone = None
ve_siren = None
ve_amp = None
# pjsip library global variables
lib = None
current_call = None
call_state = None

# Logging callback
def log_cb(level, str, len):
    logger(log_info,"PJSUA " + str),

def main_loop():
    global ve_call
    ports = gpio.get_ports()
    logger(log_info, "SCK Ready!")

    while True:
        try:
            actions = gpio.read_ports(ports)
            for action, parameter in actions.iteritems():
                if parameter[1] is not None:
                    getattr(ve_action, action)(int(parameter[1]))

        except ValueError:
            logger(log_error, 'SCK Exception, this is weird!')

	    continue


"""Make a call to specified SIP URI"""
def make_call(uri):
    try:
        logger(log_info, "SCK ("+uri+")")
        call = acc.make_call(uri, VeCallCallback())
        return call
    except pj.Error, e:
        logger(log_err, "SCK " + str(e))
        return None


class VeAction():
    def button_1(self, state):
        global ve_call
        if state == 0 and ve_call is None:
            ve_call = make_call('sip:' + speedial['ext1'] +
                '@' + sipcfg['srv'])

    def button_2(self, state):
        global ve_call
        if state == 0 and ve_call is None:
            ve_call = make_call('sip:' + speedial['ext2'] +
                '@' + sipcfg['srv'])

    def button_3(self, state):
        global ve_call
        if state == 0 and ve_call is None:
            ve_call = make_call('sip:' + speedial['ext3'] +
                '@' + sipcfg['srv'])

    def button_4(self, state):
        global ve_call
        if state == 0 and ve_call is None:
            ve_call = make_call('sip:' + speedial['ext4'] +
                '@' + sipcfg['srv'])

    def button_5(self, state):
        global ve_call
        if state == 0 and ve_call is None:
            ve_call = make_call('sip:' + speedial['ext5'] +
                '@' + sipcfg['srv'])

    def siren(self, state):
        #TODO I guess I won't need this
        global ve_siren
        global ve_amp
        #if state == 0 and (ve_amp == 0 or ve_amp is None) and ve_siren is None:
        if state == 1 and ve_siren is None:
            # Toggle audio amplifier on
            #ve_amp = vess.amplifier_on()
            # Create a siren player
            VeSound().siren()
            # Connect the siren player to speakers
            lib.conf_connect(ve_siren, 0)
            #elif state == 1 and ve_amp == 1 and ve_siren is not None:
        elif state == 0 and ve_siren is not None:
            # Toggle audio amplifier off
            #ve_amp = vess.amplifier_off()
            # Disconnect siren from speakers
            lib.conf_disconnect(ve_siren, 0)
            # Dispose of the siren
            ve_siren = None

    """ Toggle local audio On and Off """
    def local_audio(self, state):
        global ve_call
        global ve_amp
        global lib
        if state == 1 and (ve_amp == 0 or ve_amp is None):
            # Toggle audio amplifier on
            ve_amp = vess.amplifier_on()
            # Connect system's audio capture to playback 
            # (allows for the local microphone connected to speakers)
            lib.conf_connect(0, 0)
        elif state == 0 and ve_amp == 1:
            # Disconnects audio capture from playback
            lib.conf_disconnect(0, 0)
            # Toggle audio amplifier off
            ve_amp = vess.amplifier_off()

class VeSound:
    def ring_start(self):
        global lib
        global tone
        tone = lib.create_player(os.path.dirname(
            os.path.realpath(__file__)) + "/sounds/tone.wav", True)

    def incoming(self):
        global lib
        global tone
        tone = lib.create_player(os.path.dirname(
            os.path.realpath(__file__)) + "/sounds/three-tones.wav", False)

    def siren(self):
        global lib
        global ve_siren
        siren = lib.create_player(os.path.dirname(
            os.path.realpath(__file__)) + '/sounds/siren.wav', False)
        ve_siren = lib.player_get_slot(siren)



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
                        self.account.info().reg_reason)
            self.sem.release()

    def on_incoming_call(self, call):
	#TODO A lot of stuff, call handling mainly and logging also
        logger(log_info, "SCK Incoming call from " + call.info().remote_uri)
        global current_call
        global ve_call
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
        global call_state
        global ve_call
        logger(log_info, "SCK Call is " + self.call.info().state_text)
        logger(log_info, "SCK Last code = " + str(self.call.info().last_code))
        logger(log_info, "SCK (" + self.call.info().last_reason + ")")
        call_state = self.call.info().state
        #global tone
        if call_state == pj.CallState.EARLY:
            #tone = VeTone().ring_start()
            pass
        elif call_state == pj.CallState.DISCONNECTED:
            #VeTone().ring_stop(tone)
            current_call = None
            ve_call = None

    # Notification when call's media state changed
    def on_media_state(self):
        global lib
	    #global tone
        if self.call.info().media_state == pj.MediaState.ACTIVE:
            # Connect the call to sound device
            #VeTone().ring_stop(tone)
            call_slot = self.call.info().conf_slot
            lib.conf_connect(call_slot, 0)
            lib.conf_connect(0, call_slot)


try:
    # Initialize ValkEye Sound System
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
    #TODO Make a test with TCP
    transport = lib.create_transport(pj.TransportType.UDP)
    # Start pjsua library
    lib.start()
    if sipcfg == None:
        # Create local/user-less account (No PBX registration)
        acc = lib.create_account_for_transport(transport)
    else:
        # Register on PBX
        acc = lib.create_account(
                pj.AccountConfig(sipcfg['srv'], sipcfg['ext'],
                    sipcfg['pwd'])
        )
    # Set the account call back
    acc_cb = VeAccountCallback(acc)
    acc.set_callback(acc_cb)
    acc_cb.wait()
    # Set of actions mapped to keys in GPIO ports dictionary
    ve_action = VeAction()
    # main loop
    main_loop()
    # We're done, shutdown the library and do clean up
    lib.destroy()
    lib = None
    sys.exit(0)

except pj.Error, e:
    logger(log_err, "SCK Exception: " + str(e))
    lib.destroy()
    lib = None
    sys.exit(1)
