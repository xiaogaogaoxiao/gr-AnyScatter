#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: AnyScatterGRC
# GNU Radio version: 3.8.0.0

from gnuradio import gr
from gnuradio import uhd
import sys
import signal
import time
import AnyScatter
import zmq
import threading
import struct

context = zmq.Context()

class AnyScatterThread(gr.top_block):

	def __init__(self):
		gr.top_block.__init__(self, "AnyScatter")

		self.tag_rate = tag_rate = 62.5e3
		self.symbol_rate = symbol_rate = 1e6
		self.rx_rate = rx_rate = 50e6
		self.rx_gain = rx_gain = 25
		self.num_antennas = num_antennas = 4
		self.center_freq = center_freq = 2450e6

		self.uhd_usrp_source = uhd.usrp_source(
			",".join(("addr0=192.168.40.2,addr1=192.168.41.2", "")),
			uhd.stream_args(
				cpu_format="fc32",
				args='',
				channels=list(range(0,4)),
			),
		)
		self.uhd_usrp_source.set_time_source('external', 1)
		self.uhd_usrp_source.set_clock_source('external', 1)
		self.uhd_usrp_source.set_center_freq(center_freq, 0)
		self.uhd_usrp_source.set_gain(rx_gain, 0)
		self.uhd_usrp_source.set_antenna('RX2', 0)
		self.uhd_usrp_source.set_bandwidth(rx_rate, 0)
		self.uhd_usrp_source.set_center_freq(center_freq, 1)
		self.uhd_usrp_source.set_gain(rx_gain, 1)
		self.uhd_usrp_source.set_antenna('RX2', 1)
		self.uhd_usrp_source.set_bandwidth(rx_rate, 1)
		self.uhd_usrp_source.set_center_freq(center_freq, 2)
		self.uhd_usrp_source.set_gain(rx_gain, 2)
		self.uhd_usrp_source.set_antenna('RX2', 2)
		self.uhd_usrp_source.set_bandwidth(rx_rate, 2)
		self.uhd_usrp_source.set_center_freq(center_freq, 3)
		self.uhd_usrp_source.set_gain(rx_gain, 3)
		self.uhd_usrp_source.set_antenna('RX2', 3)
		self.uhd_usrp_source.set_bandwidth(rx_rate, 3)
		self.uhd_usrp_source.set_samp_rate(rx_rate)
		self.uhd_usrp_source.set_time_unknown_pps(uhd.time_spec())
		self.AnyScatter_demodulator = AnyScatter.demodulator(num_antennas, symbol_rate, tag_rate)
		self.AnyScatter_decimator = AnyScatter.decimator(num_antennas, rx_rate, symbol_rate)

		self.connect((self.AnyScatter_decimator, 0), (self.AnyScatter_demodulator, 0))
		self.connect((self.uhd_usrp_source, 0), (self.AnyScatter_decimator, 0))
		self.connect((self.uhd_usrp_source, 1), (self.AnyScatter_decimator, 1))
		self.connect((self.uhd_usrp_source, 2), (self.AnyScatter_decimator, 2))
		self.connect((self.uhd_usrp_source, 3), (self.AnyScatter_decimator, 3))

		self.socket = context.socket(zmq.SUB)
		self.socket.setsockopt(zmq.SUBSCRIBE, b'')
		self.socket.connect("ipc:///tmp/AnyScatterIPC")
		self.stopZMQ = False
		threading.Thread(target=self.recvZMQ, daemon=True).start()

	def __del__(self):
		self.stopZMQ = True
		self.socket.close()

	def recvZMQ(self):
		while not self.stopZMQ:
			report = self.socket.recv()
			res = ''.join([f'{x:02X} ' for x in report[:4]])
			idx = struct.unpack('<H', bytearray(report[4:6]))[0]
			num_antennas = struct.unpack('<H', bytearray(report[6:8]))[0]
			res += f' | {idx:d} '
			print(res)

if __name__ == '__main__':
	if gr.enable_realtime_scheduling() != gr.RT_OK:
		print("Error: failed to enable real-time scheduling.")
	tb = AnyScatterThread()

	def sig_handler(sig=None, frame=None):
		tb.stop()
		tb.wait()
		sys.exit(0)

	signal.signal(signal.SIGINT, sig_handler)
	signal.signal(signal.SIGTERM, sig_handler)

	tb.start()
	try:
		input('Press Enter to quit: ')
	except EOFError:
		pass
	tb.stop()
	tb.wait()
