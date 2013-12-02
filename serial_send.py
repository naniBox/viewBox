#!/usr/bin/env python2.6

import sys
import os
import serial
import time

def run():
	try:
		ser = serial.Serial( port=sys.argv[1], baudrate=int(sys.argv[2]), bytesize=serial.EIGHTBITS, stopbits=serial.STOPBITS_TWO, timeout=0 )
		count = 0
		while True:
			s = "Writing some data out : %d\n"%count
			print s,
			ser.write(s)
			count += 1
			time.sleep( 0.001 )

	except KeyboardInterrupt:
		print 'Exiting...'

if __name__ == '__main__':
	run()
