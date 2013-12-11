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
import veconfig

class VSS:
    def __init__(self):
	if sys.version_info[0] == 2:
	    if sys.version_info[1] > 4:
		# TODO Check if it works with 2.5 and 2.6
		self.impl = _VSS27()
	    elif sys.version_info[1] == 4:
		self.impl = _VSS24()
	    else:
	        
	        sys.exit(1)

class _Vss:
    def __init__(self):
	self.audiocfg = veconfig.get_audiocfg()
       # Get the mixer
	self.mixer = amixer.Mixer()
	self.mixer.attach()
	self.mixer.load()
	#print self.mixer.list()
	self._get_mixer_elements()
	self.mute_all()

    def _get_mixer_elements(self):
	# Get the mixer elements
	if self.mixer.count > 0:
	    elements = self.mixer.list()
	    for i in elements:
		if i[0] == self.audiocfg['master']:
		    self._set_master(i)
		#elif i[0] == audiocfg['pcm']:
		#    self._set_pcm(i)
		#elif i[0] == audiocfg['capture']:
		#    self._set_capture(i)
		#elif i[0] == audiocfg['cap_idx']:
		#    self._set_cap_idx(i)
		#elif i[0] == audiocfg['input_src']:
		#    self._set_input_src(i)
		#elif i[0] == audiocfg['in_idx']:
		#    self._set_in_idx(i)
		#elif i[0] == audiocfg['mic']:
		#    self._set_mic(i)
		#elif i[0] == audiocfg['mic_boost']:
		#    self._set_mic_boost(i)
		pass

    def _set_master(self, element):
	self.master = amixer.Element(self.mixer, element[0], element[1])

    def _set_pcm(self, element):
	self.pcm = amixer.Element(self.mixer, element[0], element[1])

    def _set_capture(self, element):
	self.capture = amixer.Element(self.mixer, element[0], element[1])

    def _set_cap_idx(self, element):
	self.capture = amixer.Element(self.mixer, element[0], element[1])

    def _set_input_src(self, element):
	self.capture = amixer.Element(self.mixer, element[0], element[1])

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

    def migGain_down(self):
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
    def __init__(self):
	_Vss.__init__(self)

    def mute_all(self):
	if self.master:
            self.master.setVolumeAll(0)

   

class _VSS27(_Vss):
    def __init__(self):
	_Vss.__init__(self)

    def mute_all(self):
	if self.master:
            self.master.set_volume_all(0)


