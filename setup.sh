#!/usr/bin/env bash

# Copyright 2013 - 2023 Ernesto Celis
#
# This file is part of Simple Communications Kit (SCK).
#
# SCK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# SCK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with SCK.  If not, see <https://www.gnu.org/licenses/>.

PJS_VERSION=${PJS_VERSION:-2.13.1}
BASEDIR=$(pwd)
PJPDIR=third_party/pjproject-$PJS_VERSION
VIDEO=${VIDEO:-1}
CLEAN=${CLEAN:-1}
SCKDIR=$HOME/sck

if [[ $OSTYPE == "linux-gnu" ]] ; then
 echo "Linux build"
  MAKECMD=make
else
  MAKECMD=gmake
fi

cd $BASEDIR/$PJPDIR

if [[ $CLEAN == 1 ]] ; then
  $MAKECMD distclean
fi

if [[ $VIDEO == 1 ]] ; then
  ./configure --prefix=$SCKDIR
else
  ./configure --prefix=$SCKDIR --disable-video --disable-ffmpeg --disable-v4l2
fi

if [[ $? -eq 0 ]] ; then
  CFLAGS="-fPIC -O2" CXXFLAGS="-fPIC" $MAKECMD dep
  if [[ $? -eq 0 ]] ; then
    CFLAGS="-fPIC -O2" CXXFLAGS="-fPIC" $MAKECMD
    echo
    echo READY TO INSTALL
    if [[ $? -eq 0 ]] ; then
      if [[ $CLEAN == 1 ]] ; then
        rm -rf $SCKDIR
      fi
      python3 -m venv $SCKDIR 
      source $SCKDIR/bin/activate
      $MAKECMD install
      cd $BASEDIR/$PJPDIR/pjsip-apps/src/swig/python
      CFLAGS="-fPIC -O2" CXXFLAGS="-fPIC" $MAKECMD
      $MAKECMD install
      $SCKDIR/bin/python3 setup.py install
    fi
  fi
fi
