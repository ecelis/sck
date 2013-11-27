# ValkEye SIP Phone, vetones.py
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
import pygame

FREQ = 44100 # Same as audio CD
BITSIZE = -16 # Unsigned 16bit
CHANNELS = 2 # 1 = mono, 2 = stereo
BUFFER = 1024 # audio buffer size in No. of samples/sec
FRAMERATE = 30 # How often check if playback has finished

class Tone:
soundir = os.path.dirname(os.path.realpath(__file__)) + "/sounds"
one_tone = soundir + "/tone.wav"
three_tones = soundir + "/three-tones.wav"

def __init__(self):
   pygame.mixer.init(FREQ, BITSIZE, CHANNELS, BUFFER)

def one_ring():
    sound = pygame.mixer.Sound(one_tone)
    clock = pygame.time.Clock()
    sound.play()
    while pygame.mixer.get_busy():
	clock.tick(FRAMERATE)

one_ring()

