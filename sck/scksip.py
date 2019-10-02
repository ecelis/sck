# Copyright 2013 - 2019 Ernesto Celis

# This file is part of SOS Communications Kit (SCK).

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

import sys
import pjsua as pj
import threading
from syslog import syslog as logger
from syslog import LOG_INFO
from syslog import LOG_ERR
import config

# Global stuff
FLAVORS = ['pc', 'cb2', 'ct']
DIGITS = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
PINOUT = ['ext1', 'ext2', 'ext3', 'ext4', 'ext5', 'siren', 'spk']
LOG_LEVEL = 3
call_in_progress = None

# Detect hardware, supported flavors are PC, Cubieboard 2 and Cubietruck
_hardware = config.get_flavor()
logger(LOG_INFO, u'SCK Running with ' + _hardware + ' settings')
if _hardware in FLAVORS:
    if _hardware == 'pc':
        import getchar as in_interface
    elif _hardware == 'cb2':
        import wiringcb as in_interface
    elif _hardware == 'ct':
        import gpio_api as in_interface
    read_input = in_interface.read_input()
else:
    logger(LOG_ERR, u'SCK does not support the hardware')
    sys.exit(1)


# Logging callback
def log_cb(level, str, len):
    logger(LOG_INFO, "PJSUA " + str)


def main_loop():
    logger(LOG_INFO, "SCK Ready!")
    while True:
        try:
            # Read input operation
            in_value = read_input()
            # Special options are handled by *,-,+ and / characters
            if in_value == "*":
                # TODO * enable local audio
                logger(LOG_INFO, "SCK Toggle Local MIC")
            elif in_value == "+":
                # TODO Test only option, do not use it for real services!
                logger(LOG_INFO, "SCK Dialing TEST")
                #make_call("sip:1106@sip.sdf.org")
            elif in_value == "-":
                # TODO reserved
                logger(LOG_INFO, "SCK - Action Reserved")
            elif in_value == "/":
                # Exit manually
                logger(LOG_INFO, "SCK Exit on user request!")
                return
            elif in_value in DIGITS:
                # Only the PC version takes the whole range of digits as input
                if _hardware == 'pc':
                    for speed, ext in speedial.iteritems():
                        getattr(ve_speedial, 'ext'+in_value)(ext)
                else:
                    logger(LOG_INFO,
                           u'SCK trying to dial with digits in an unsupported hardware')
            elif in_value in PINOUT:
                if _hardware == 'cb2':
                    for speed, ext in speedial.iteritems():
                        getattr(ve_speedial, in_value)(ext)
                else:
                    logger(LOG_INFO,
                           u'SCK trying to dial with GPIO in an unsupported hardware')
            else:
                logger(LOG_ERR, 'SCK Invalid input ' + in_value)
        except ValueError:
            logger(LOG_INFO, "SCK Exception, this is weird!")
        continue


""" Make a call to specified SIP URI """
def make_call(uri):
    try:
        logger(LOG_INFO, "SCK ("+uri+")")
        call = acc.make_call(uri, CallCallback())
        return call
    except pj.Error, message:
        logger(LOG_ERR, "SCK " + str(message))
        return None


""" Functions triggered by speedial extensions """
class SpeedDial():
    def ext0(self, ext):
        global call_in_progress
        if call_in_progress is None:
            logger(LOG_INFO, 'SCK Dialing TODO')
            call_in_progress = make_call('sip:' + ext + '@' + sipcfg['srv'])

    def ext1(self, ext):
        global call_in_progress
        if call_in_progress is None:
            logger(LOG_INFO, 'SCK Dialing TODO')
            call_in_progress = make_call('sip:' + ext + '@' + sipcfg['srv'])

    def ext2(self, ext):
        global call_in_progress
        if call_in_progress is None:
            logger(LOG_INFO, 'SCK Dialing TODO')
            call_in_progress = make_call('sip:' + ext + '@' + sipcfg['srv'])

    def ext3(self, ext):
        global call_in_progress
        if call_in_progress is None:
            logger(LOG_INFO, 'SCK Dialing TODO')
            call_in_progress = make_call('sip:' + ext + '@' + sipcfg['srv'])

    def ext4(self, ext):
        global call_in_progress
        if call_in_progress is None:
            logger(LOG_INFO, 'SCK Dialing TODO')
            call_in_progress = make_call('sip:' + ext + '@' + sipcfg['srv'])

    def ext5(self, ext):
        global call_in_progress
        if call_in_progress is None:
            logger(LOG_INFO, 'SCK Dialing TODO')
            call_in_progress = make_call('sip:' + ext + '@' + sipcfg['srv'])

    def ext6(self, ext):
        global call_in_progress
        if call_in_progress is None:
            logger(LOG_INFO, 'SCK Dialing TODO')
            call_in_progress = make_call('sip:' + ext + '@' + sipcfg['srv'])

    def ext7(self, ext):
        global call_in_progress
        if call_in_progress is None:
            logger(LOG_INFO, 'SCK Dialing TODO')
            call_in_progress = make_call('sip:' + ext + '@' + sipcfg['srv'])

    def ext8(self, ext):
        global call_in_progress
        if call_in_progress is None:
            logger(LOG_INFO, 'SCK Dialing TODO')
            call_in_progress = make_call('sip:' + ext + '@' + sipcfg['srv'])

    def ext9(self, ext):
        global call_in_progress
        if call_in_progress is None:
            logger(LOG_INFO, 'SCK Dialing TODO')
            call_in_progress = make_call('sip:' + ext + '@' + sipcfg['srv'])


""" Callback for handling registration on PBX """
class AccountCallback(pj.AccountCallback):

    semaphore = None

    def __init__(self, account):
        pj.AccountCallback.__init__(self, account)

    def wait(self):
        self.semaphore = threading.Semaphore(0)
        self.semaphore.acquire()

    def on_reg_state(self):
        if self.semaphore:
            if self.account.info().reg_status >= 200:
                logger(LOG_ERR, 'SCK registration status ' +
                       str(self.account.info().reg_status) +
                       ' ' + self.account.info().reg_reason)
            self.semaphore.release()

    def on_incoming_call(self, call):
        # TODO A lot of stuff, call handling mainly and logging also
        logger(LOG_INFO, "SCK Incoming call from " + call.info().remote_uri)
        global current_call
        # TODO global tone
        # tone = VeTone().ring_start()
        current_call = call
        call_cb = CallCallback(current_call)
        current_call.set_callback(call_cb)
        current_call.answer(200)


""" Class to receive events from Call """
class CallCallback(pj.CallCallback):

    def __init__(self, call = None):
        pj.CallCallback.__init__(self, call)

    def on_state(self):
        global current_call
        logger(LOG_INFO, "SCK Call is " + self.call.info().state_text)
        logger(LOG_INFO, "SCK Last code = " + str(self.call.info().last_code))
        logger(LOG_INFO, "SCK (" + self.call.info().last_reason + ")")

        global call_state
        call_state = self.call.info().state

        # global tone
        if call_state == pj.CallState.EARLY:
            # tone = VeTone().ring_start()
            pass
        elif call_state == pj.CallState.DISCONNECTED:
            # VeTone().ring_stop(tone)
            current_call = None

    # Notification when call's media state change
    def on_media_state(self):
        global lib
        if self.call.info().media_state == pj.MediaState.ACTIVE:
            # Connect the call to sound device
            call_slot = self.call.info().conf_slot
            lib.conf_connect(call_slot, 0)
            lib.conf_connect(0, call_slot)


try:
    sipcfg = config.get_sipcfg()        # Get PBX/SIP auth credentials
    speedial = config.get_speedial()    # Get Speed Dial Extensions
    media = pj.MediaConfig()            # Media Config
    media.ec_options = 0                # pjsua default 0
    media.ec_tail_len = 256             # pjsua default 256
    media.no_vad = False                # disable Voice Activity Detector
    media.enable_ice = True             # Enable (ICE) Interactive Connectivity Establishment
    lib = pj.Lib()                      # Create pjsua library instance
    # Init pjsua with default config
    lib.init(log_cfg = pj.LogConfig(level = LOG_LEVEL, callback = log_cb))
    # Set sound device TODO in vc.py
    # lib.set_snd_dev(0,0)
    # Create UDP transport which listens to any available port
    transport = lib.create_transport(pj.TransportType.UDP)
    # Start the library
    lib.start()
    if sipcfg is None:
        # Create local/user-less account
        acc = lib.create_account_for_transport(transport)
    else:
        # Register on PBX
        acc = lib.create_account(pj.AccountConfig(sipcfg['srv'], sipcfg['ext'],
                                                  sipcfg['passwd']))
    # Set the account call back
    acc_cb = AccountCallback(acc)
    acc.set_callback(acc_cb)
    acc_cb.wait()
    # Global variables
    ve_speedial = SpeedDial()
    # main loop
    main_loop()
    # We're done, shutdown the library
    lib.destroy()
    lib = None
    sys.exit(0)

except pj.Error, e:
    logger(LOG_ERR, "SCK Exception: " + str(e))
    lib.destroy()
    lib = None
    sys.exit(1)
