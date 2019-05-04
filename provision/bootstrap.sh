#!/bin/bash

SCK_BASE=/vagrant

case $(uname -m) in
  x86_64)
    ARCH=x64  # or AMD64 or Intel64 or whatever
  ;;
  i*86)
    ARCH=x86  # or IA32 or Intel32 or whatever
  ;;
  *)
    # leave ARCH as-is
  ;;
esac

if [ -f /etc/os-release ]; then
  # freedesktop.org and systemd
  . /etc/os-release
  OS=$NAME
  VER=$VERSION_ID
elif type lsb_release >/dev/null 2>&1; then
  # linuxbase.org
  OS=$(lsb_release -si)
  VER=$(lsb_release -sr)
elif [ -f /etc/lsb-release ]; then
  # For some versions of Debian/Ubuntu without lsb_release command
  . /etc/lsb-release
  OS=$DISTRIB_ID
  VER=$DISTRIB_RELEASE
elif [ -f /etc/debian_version ]; then
  # Older Debian/Ubuntu/etc.
  OS=Debian
  VER=$(cat /etc/debian_version)
elif [ -f /etc/SuSe-release ]; then
  # Older SuSE/etc.
  echo Unsuported
elif [ -f /etc/redhat-release ]; then
  # Older Red Hat, CentOS, etc.
  echo Unsuported
else
  # Fall back to uname, e.g. "Linux <version>", also works for BSD, etc.
  OS=$(uname -s)
  VER=$(uname -r)
fi

echo "Build ${OS} ${VER} ${ARCH}"

case "${OS}" in
  Centos*)
    yum -x 'kernel*' -y --skip-broken update
    yum -y groupinstall Base
    yum -y groupinstall "Development Tools"
    # Install required dependencies, it assumes a REHEL6 base and Development Tools
    yum -y install python-devel \
      python-setuptools \
      alsa-lib \
      alsa-lib-devel \
      portaudio \
      portaudio-devel \
      libsamplerate \
      libsamplerate-devel \
      openssl \
      openssl-devel \
      libmad \
      libmad-devel \
      taglib taglib-devel \
      lame \
      lame-devel \
      libvorbis \
      libvorbis-devel \
      libogg libogg-devel \
      speex-devel \
      ladspa \
      ladspa-devel \
      libshout \
      libshout-devel \
      jack-audio-connection-kit \
      jack-audio-connection-kit-devel \
      libao libao-devel
  ;;
  Debian*)
    sudo apt-get -yqq update && sudo apt-get -yqq dist-upgrade
    sudo apt-get -yqq install python3-venv build-essential
    sudo apt-get -yqq install \
      python-pjproject

    ;;
  *)
    echo $OS
    echo 'Oops'
  ;;
esac

#cd $SCK_BASE
#python3 -m venv venv
## Symlink the project shared folder in $HOME
#if [ ! -L sck ]; then
  #ln -s $SCK_BASE $HOME/sck
#fi
## Build and install pjsip dependencies
#cd $SCK_BASE
#./build.sh
