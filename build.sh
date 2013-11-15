#!/usr/bin/env bash
make distclean
./configure --disable-video --disable-ffmpeg --disable-v4l2
CFLAGS="-fPIC" CXXFLAGS="-fPIC" make dep
CFLAGS="-fPIC" CXXFLAGS="-fPIC" make 
cd pjsip-apps/src/python
python setup.py build

