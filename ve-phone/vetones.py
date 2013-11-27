from struct import pack
from math import sin, pi
import time

def au_file(name, freq, freq1, dur, vol):
    fout = open(name, 'wb')
    # header needs size, encoding=2, sampling_rate=8000, channel=1
    fout.write('.snd' + pack('>5L', 24, 8*dur, 2, 8000, 1))
    factor = 2 * pi * freq/8000
    factor1 = 2 * pi * freq1/8000
    # write data
    for seg in range(8 * dur):
        # sine wave calculations
        sin_seg = sin(seg * factor) + sin(seg * factor1)
        fout.write(pack('b', vol * 64 * sin_seg))
    fout.close()

t = time.strftime("%S", time.localtime())
ti = time.strftime("%M", time.localtime())
tis = float(t)
tis = tis * 100
tim = float(ti)
tim = tim * 100

if __name__ == '__main__':
    au_file(name='timeSound2.au', freq=tim, freq1=tis, dur=100, vol=1.0)

    import os
    os.startfile('timeSound2.au')
