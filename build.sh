#!/usr/bin/env bash
make distclean
rm -f pjmedia/include/pjmedia/config_auto.h
rm -f pjmedia/include/pjmedia-codec/config_auto.h
rm -f pjmedia/build/os-auto.mak
rm -f pjlib/include/pj/config_site.h 
rm -f pjlib/include/pj/compat/os_auto.h
rm -f pjlib/include/pj/compat/m_auto.h
rm -f pjlib/build/os-auto.mak
rm -f pjlib-util/build/os-auto.mak
rm -f build/os-auto.mak
rm -f build/cc-auto.mak
rm -f build.mak
./configure --disable-video --disable-ffmpeg --disable-v4l2
CFLAGS="-fPIC" CXXFLAGS="-fPIC" make dep
CFLAGS="-fPIC" CXXFLAGS="-fPIC" make 
cd pjsip-apps/src/python
python setup.py build

