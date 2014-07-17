#!/usr/bin/env bash
CWD=$(pwd)
PJPDIR=third_party/pjproject-2.1.0
FMPDIR=third_party/ffmpeg-1.2.6
SUDO=/usr/bin/sudo
#ENABLE_VIDEO=$1

usage () {
#TODO
  echo "Usage: $0 TODO"
  exit 1
}

#[[ $# -eq 0 ]] && usage
echo -e "Enable video support? [y/N]"
read ENABLE_VIDEO

if [[ $ENABLE_VIDEO == 'y' ]] ; then
  cd $CWD/$FMPDIR
  make distclean
  ./configure --enable-shared --disable-static --enable-memalign-hack
  make -j2
  make install
fi

cd $CWD/$PJPDIR
make distclean
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
if [[ $ENABLE_VIDEO == 'y' ]] ; then
  ./configure
else
  ./configure --disable-video --disable-ffmpeg --disable-v4l2
fi
CFLAGS="-fPIC" CXXFLAGS="-fPIC" make dep
CFLAGS="-fPIC" CXXFLAGS="-fPIC" make
if [ -x $SUDO ]; then
  sudo make install
  cd $CWD/$PJPDIR/pjsip-apps/src/python
  sudo python setup.py install
else
  su -c "make install" root
  cd $CWD/$PJPDIR/pjsip-apps/src/python
  su -c "python setup.py install" root
fi

