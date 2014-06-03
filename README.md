Sauron-OS Communications Kit
============================

Built on top of pjsua library, SCK aims to provide VoIP capabilities to
Citizenship's Towers System. It is a SIP client writen in python for a
very specific purpose. Runs right in the command line without need for
any graphic desktop environment. Designed to run on Unix like (mainly
GNU/Linux) embedded systems, SCK uses the operating system log
capabilities.

Copyright 2013 Ernesto Celis, this software is released under the terms
of the GNU Public License version 3

SCK relies on third party softwre libraries which may be released under
diferent license terms, read the COPY file to get more info.

SCK is sponsored by [Valk Technologies](http://valktechnologies.com/)

Features
--------

* Auto-answer
* Fixed set of extension numbers to dial
* One key press speed dial
* Logs to system's log
* Command line only
* Plain text config file


Install
-------

The only thing that needs to be installed is the pjsua python module, as it is not available as a binary package in none of the two GNU/Linux distributions I'm working on, I've included a copy of pjproject version 1.2.0

**TODO** Dependencies

### CentOS/Fedora

    cd sauron-com-kit
    su
    ./build.sh


### Ubuntu

    cd sauron-com-kit
    sudo ./build.sh

### Manual

No video support

    cd sauron-com-kit/third_party/pjproject-2.1.0
    ./configure --disable-video --disable-ffmpeg --disable-v4l2
    CFLAGS="-fPIC" CXXFLAGS="-fPIC" make dep
    CFLAGS="-fPIC" CXXFLAGS="-fPIC" make
    cd pjsip-apps/src/python
    python setup.py install

With video support

    cd sauron-com-kit/third_party/ffmpeg-1.2.6
    ./configure --enable-shared --disable-static --enable-memalign-hack
    make -j2
    make install
    cd sauron-com-kit/third_party/pjproject-2.1.0
    ./configure --disable-video --disable-ffmpeg --disable-v4l2
    CFLAGS="-fPIC" CXXFLAGS="-fPIC" make dep
    CFLAGS="-fPIC" CXXFLAGS="-fPIC" make
    cd pjsip-apps/src/python
    python setup.py install


Configure
---------

Copy the file `sauron-com-kit/ve-phone/config.ini.orig` either to
`~/config.ini` or `~/settings/config.ini` or
`sauron-com-kit/ve-phone/config.ini`

Edit the file with your PBX user name and password, speed dial extension
numbers and audio settings for your sound card if needed


Run
---

   cd sauron-com-kit/ve-phone python vephone.py


Enjoy!

Ernesto Celis

P.S. Thank you Teluu Ltd. for the great
[pjsip/pjsua](http://www.pjsip.org/) libraries!
