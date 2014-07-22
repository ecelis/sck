#!/usr/bin/env bash

# Update OS packages
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
# Symlink the project shared folder in $HOME
if [ ! -L sck ]; then
  ln -s /vagrant/ sck
fi
# Build and install pjsip dependencies
cd sck
./build.sh

