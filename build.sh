#!/usr/bin/env bash

#Copyright 2013 - 2019 Ernesto Celis

#This file is part of SOS Communications Kit (SCK).

#SCK is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.

#SCK is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with SCK.  If not, see <https://www.gnu.org/licenses/>.

PJS_VERSION=${PJS_VERSION:-2.4.5}
CWD=$(pwd)
PJPDIR=third_party/pjproject-$PJS_VERSION
VIDEO=${VIDEO:-1}
CLEAN=${CLEAN:-1}


cd $CWD/$PJPDIR
if [[ $CLEAN == 1 ]] ; then
  make distclean
  find $CWD/$PJDIR -type f -name '.*.depend*' -exec rm {} \;
#rm -f pjmedia/include/pjmedia/config_auto.h
#rm -f pjmedia/include/pjmedia-codec/config_auto.h
#rm -f pjmedia/build/os-auto.mak
#rm -f pjlib/include/pj/config_site.h 
#rm -f pjlib/include/pj/compat/os_auto.h
#rm -f pjlib/include/pj/compat/m_auto.h
#rm -f pjlib/build/os-auto.mak
#rm -f pjlib-util/build/os-auto.mak
#rm -f build/os-auto.mak
#rm -f build/cc-auto.mak
#rm -f build.mak
fi
if [[ $VIDEO == 1 ]] ; then
  ./configure
else
  ./configure --disable-video --disable-ffmpeg --disable-v4l2
fi
CFLAGS="-fPIC -O2" CXXFLAGS="-fPIC" make dep
CFLAGS="-fPIC -O2" CXXFLAGS="-fPIC" make
sudo make install
cd $CWD
if [[ $CLEAN == 1 ]] ; then
  rm -rf ENV
  python3 -m venv ENV
#virtualenv ENV
fi
#. ENV/bin/activate
cd $CWD/$PJPDIR/pjsip-apps/src/python
sudo $CWD/ENV/bin/python3 setup.py install
