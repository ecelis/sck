"""
This file is part of Simple Communications Kit (SCK).

Copyright 2013 - 2023 Ernesto Celis

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
"""
import pjsua2 as pj


class Endpoint(pj.Endpoint):
    """Python object inherited from pj.Endpoint"""
    _instance = None

    def __init__(self):
        pj.Endpoint.__init__(self)
        Endpoint._instance = self

    def validate_uri(self, uri):
        """Check for a valid URI"""
        return Endpoint._instance.utilVerifyUri(uri) == pj.PJ_SUCCESS

    def validate_sip_uri(self, uri):
        """ Check valid SIP URI"""
        return Endpoint._instance.utilVerifySipUri(uri) == pj.PJ_SUCCESS
