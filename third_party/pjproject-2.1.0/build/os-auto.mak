# build/os-auto.mak.  Generated from os-auto.mak.in by configure.

export OS_CFLAGS   := $(CC_DEF)PJ_AUTOCONF=1 -O2 -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1

export OS_CXXFLAGS := $(CC_DEF)PJ_AUTOCONF=1 -O2 

export OS_LDFLAGS  :=  -lm -lnsl -lrt -lpthread  -lasound   -lv4l2 -lcrypto -lssl

export OS_SOURCES  := 


