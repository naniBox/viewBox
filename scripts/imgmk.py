#!/usr/bin/env python2.5

import PIL.Image as Image
import sys
import os
import struct

def avg(p):
	r,g,b = p
	return 255-((r+g+b)/3)

def main():
	im = Image.open(sys.argv[1])
	print "//",sys.argv[1],im.format, im.size, im.mode

	xr, yr = im.size

	packed = []
	byte = 0
	for cy in range(yr):
		for cx in range(xr):
			p = im.getpixel((cx,cy))
			if cx%2==0:
				byte = (avg(p)>>4)<<4
			else:
				byte = byte | (avg(p)>>4)
				packed.append(byte)

			#print "%d, %d, %s, %d, %02x"%(cx,cy,str(p),avg(p),byte)

	print "#define LOGO_SIZE %d"%len(packed)
	print "static const unsigned char logo[] = {"
	for idx in range(len(packed)):
		b = packed[idx]
		print "0x%02x,"%b,
		if (idx+1)%16==0:
			print
	print "};"

if __name__ == '__main__':
	main()