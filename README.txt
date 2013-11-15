Sauron Communications Kit
=========================

Built on top of pjsip library, Sauron Communications Kit aim is to provide
VoIP capabilities to citizen's emergency towers. Mainly it is a SIP
client writen in python for a very specific purpose. It runs right in the
command line with no need for any graphic desktop environment.

Since it is designed to run on (GNU/Linux) embeded systems, it uses the
syslog capabilities provided by the operating system.


Features
--------

* Autoanswer
* Fixed set of extension numbers to dial
* One key press dial
* logging to system's syslog
* Command line only


CentOS Install
--------------

    cd sauron-com-kit
    ./build.sh
    su
    python setup.py install

Run
---

   cd sauron-com-kit/ve-phone
   python vephone.py


Enjoy!
Ernesto Celis

