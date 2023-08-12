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

case "$OSTYPE" in
  linux*)
    MAKECMD=make
    echo
    echo ===> $OSTYPE setup
    echo
    ;;
  *bsd*)
  darwin*)
    MAKECMD=gmake
    echo
    echo ===> $OSTYPE setup
    echo
    ;;
  *)
    MAKECMD=make
    echo
    echo ===> Generic setup
    echo
    ;;
esac
cd $BASEDIR/$PJPDIR
if [[ $CLEAN == 1 ]] ; then
  make distclean
fi
if [[ $VIDEO == 1 ]] ; then
  ./configure --prefix=$SCKDIR
else
  ./configure --prefix=$SCKDIR --disable-video --disable-ffmpeg --disable-v4l2
fi
echo $?
## These flasg might not be required anymore
CFLAGS="-fPIC -O2" CXXFLAGS="-fPIC" make dep
CFLAGS="-fPIC -O2" CXXFLAGS="-fPIC" make
if [[ $? -eq 0 ]] ; then
  make dep
  if [[ $? -eq 0 ]] ; then
    make
    echo
    echo READY TO INSTALL
    if [[ $? -eq 0 ]] ; then
      mkdir -p $SCKDIR
      sudo make install
      cd $BASEDIR
      if [[ $CLEAN == 1 ]] ; then
        rm -rf $SCKDIR
      fi
      python -m venv $SCKDIR 
      source $SCKDIR/bin/activate
      cd $BASEDIR/$PJPDIR/pjsip-apps/src/python
      $SCKDIR/bin/python3 setup.py install
    fi
  fi
fi
