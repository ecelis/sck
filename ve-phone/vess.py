#!/usr/bin/env python
# ValkEye sound System, vss.py 
# Ernesto Celis <developer@celisdelafuente.net>
# Dic. 2013
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
import pyalsa.alsacard as acard
import pyalsa.alsamixer as amixer
import sys
import syslog
import veconfig

class VSS:
    """ValkEye Sound System factory
    
    It returns an instance of VSS based on Python version since 
    Alsa python module changed the API between 2.4 and 2.7"""

    def __init__(self):
	if sys.version_info[0] == 2:
	    if sys.version_info[1] > 4:
		# TODO Check if it works with 2.5 and 2.6
		self.impl = _VSS27()
	    elif sys.version_info[1] == 4:
		self.impl = _VSS24()
	    else:
		print "OMFG! we don't support your Python version it seems, please drop us a line"
	        sys.exit(1)


class _Vss:
    """Base implementation of ValkEye SIP Phone Sound system"""

    def __init__(self):
	self.audiocfg = veconfig.get_audiocfg()
        # Get the mixer
	self.mixer = amixer.Mixer()
	self.mixer.attach()
	self.mixer.load()
	self._get_mixer_elements()
	self.mute_all()


    def _get_mixer_elements(self):
	# Get the mixer elements
	if self.mixer.count > 0:
	    elements = self.mixer.list()
	    for i in elements:
		if i[0] == self.audiocfg['master']:
		    self._set_master_control(i)
		elif i[0] == self.audiocfg['pcm']:
		    self._set_pcm_control(i)
		elif i[0] == self.audiocfg['capture']:
		    self._set_capture_control(i)
		elif i[0] == self.audiocfg['cap_idx']:
		    self._set_cap_idx_control(i)
		elif i[0] == self.audiocfg['input_src']:
		    self._set_input_src_control(i)
		elif i[0] == self.audiocfg['in_idx']:
		    self._set_in_idx_control(i)
		elif i[0] == self.audiocfg['mic']:
		    self._set_mic_control(i)
		elif i[0] == self.audiocfg['mic_boost']:
		    self._set_mic_boost_control(i)
		

    def _set_master_control(self, element):
	self.master = amixer.Element(self.mixer, element[0], element[1])

    def _set_pcm_control(self, element):
	self.pcm = amixer.Element(self.mixer, element[0], element[1])

    def _set_capture_control(self, element):
	self.capture = amixer.Element(self.mixer, element[0], element[1])

    def _set_cap_idx_control(self, element):
	self.cap_idx = amixer.Element(self.mixer, element[0], element[1])

    def _set_input_src_control(self, element):
	self.input_src = amixer.Element(self.mixer, element[0], element[1])
    
    def _set_in_idx_control(self, element):
	self.in_idx = amixer.Element(self.mixer, element[0], element[1])

    def _set_mic_control(self, element):
	self.mic = amixer.Element(self.mixer, element[0], element[1])

    def _set_mic_boost_control(self, element):
	self.mic_boost = amixer.Element(self.mixer, element[0], element[1])


    def mute_all(self):
	pass

    def mute_mic(self, state=True):
	pass

    def mute_phone(self, state=True):
	pass

    def mute_speaker(self, state=True):
	pass

    def open_mic(self):
	pass

    def stereo_on(self, state=False):
	pass

    def micGain_down(self):
	pass

    def micGain_up(self):
	pass

    def phoneVol_up(self):
	pass

    def phoneVol_down(self):
        pass

    def speakerVol_up(self):
	pass

    def speakerVol_down(self):
	pass


class _VSS24(_Vss):
    """Python 2.4 implementation"""
    def __init__(self):
	_Vss.__init__(self)

    def mute_all(self):
	if self.master:
	    self.mic.setVolumeAll(0)
            self.master.setVolumeAll(0)
	    self.pcm.setVolumeAll(0)

    def mute_mic(self, state=True):
	pass

    def mute_phone(self, state=True):
	# Rear left (speaker) channel off
	# Rear right (phone) channel on
	sw = self.pcm.getSwitch()
	print sw
	swt = self.pcm.getSwitchTuple()
	print sw


    def mute_speaker(self, state=True):
	pass

    def open_mic(self):
	pass

    def stereo_on(self, state=False):
	pass

    def micGain_down(self):
	pass

    def micGain_up(self):
	pass

    def phoneVol_up(self):
	pass

    def phoneVol_down(self):
        pass

    def speakerVol_up(self):
	pass

    def speakerVol_down(self):
	pass

   
class _VSS27(_Vss):
    """Python 2.7 implementation"""
    def __init__(self):
	_Vss.__init__(self)

    def mute_all(self):
	if self.master:
            self.master.set_volume_all(0)

    def mute_mic(self, state=True):
	pass

    def mute_phone(self, state=True):
	pass

    def mute_speaker(self, state=True):
	pass

    def open_mic(self):
	pass

    def stereo_on(self, state=False):
	pass

    def micGain_down(self):
	pass

    def micGain_up(self):
	pass

    def phoneVol_up(self):
	pass

    def phoneVol_down(self):
        pass

    def speakerVol_up(self):
	pass

    def speakerVol_down(self):
	pass


