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