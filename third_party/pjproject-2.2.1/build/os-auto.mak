# build/os-auto.mak.  Generated from os-auto.mak.in by configure.

export OS_CFLAGS   := $(CC_DEF)PJ_AUTOCONF=1  -I/home/ecelis/local/adt-bundle-linux-x86_64-20140321/android-ndk-r9d/platforms/android-19/arch-arm/usr/include -I/home/ecelis/local/adt-bundle-linux-x86_64-20140321/android-ndk-r9d/sources/cxx-stl/gnu-libstdc++/4.8/include -I/home/ecelis/local/adt-bundle-linux-x86_64-20140321/android-ndk-r9d/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi/include -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1

export OS_CXXFLAGS := $(CC_DEF)PJ_AUTOCONF=1  -I/home/ecelis/local/adt-bundle-linux-x86_64-20140321/android-ndk-r9d/platforms/android-19/arch-arm/usr/include -I/home/ecelis/local/adt-bundle-linux-x86_64-20140321/android-ndk-r9d/sources/cxx-stl/gnu-libstdc++/4.8/include -I/home/ecelis/local/adt-bundle-linux-x86_64-20140321/android-ndk-r9d/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi/include  -shared --sysroot=/home/ecelis/local/adt-bundle-linux-x86_64-20140321/android-ndk-r9d/platforms/android-19/arch-arm -fexceptions -frtti

export OS_LDFLAGS  :=  -nostdlib -L/home/ecelis/local/adt-bundle-linux-x86_64-20140321/android-ndk-r9d/platforms/android-19/arch-arm/usr/lib/ -L/home/ecelis/local/adt-bundle-linux-x86_64-20140321/android-ndk-r9d/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi -lm /home/ecelis/local/adt-bundle-linux-x86_64-20140321/android-ndk-r9d/platforms/android-19/arch-arm/usr/lib/crtbegin_so.o -lgnustl_static  -lc -lgcc -lOpenSLES -llog

export OS_SOURCES  := 


