/* -*- c++ -*- */
/*
 * Copyright 2020 Taekyung Kim (tkkim92@korea.ac.kr).
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "demodulator_impl.h"

namespace gr {
	namespace AnyScatter {

		demodulator::sptr demodulator::make(int num_antennas, float symbol_rate, float tag_rate)
		{
			return gnuradio::get_initial_sptr
				(new demodulator_impl(num_antennas, symbol_rate, tag_rate));
		}


		demodulator_impl::demodulator_impl(int num_antennas, float symbol_rate, float tag_rate)
			: gr::sync_block("demodulator",
					gr::io_signature::make(1, 1, sizeof(gr_complex) * (num_antennas * (num_antennas + 1) / 2)),
					gr::io_signature::make(0, 0, 0)),
			d_symbol_rate(symbol_rate),
			d_tag_rate(tag_rate),
			d_sps(std::round(symbol_rate / tag_rate)),
			d_num_antennas(num_antennas),
			d_num_pairs(num_antennas * (num_antennas - 1) / 2),
			d_vlen(num_antennas * (num_antennas + 1) / 2)
		{
			d_context = new zmq::context_t(1);
			d_socket = new zmq::socket_t(*d_context, ZMQ_PUB);
			d_socket->bind("ipc:///tmp/AnyScatterIPC");

			d_sample_buf = std::vector<std::vector<gr_complex>>(d_vlen,
					std::vector<gr_complex>(d_sps, gr_complex(0.0f, 0.0f)));
			d_sample_sum = std::vector<gr_complex>(d_vlen, gr_complex(0.0f, 0.0f));
			d_sample_idx = 0;
			d_flush_cnt = 0;

			d_channel0 = std::vector<gr_complex>(d_vlen, gr_complex(0.0, 0.0f));
			d_channel1 = std::vector<gr_complex>(d_vlen, gr_complex(0.0, 0.0f));

			d_gate_prev = std::vector<std::vector<gr_complex>>(d_vlen,
					std::vector<gr_complex>(3, gr_complex(0.0f, 0.0f)));
			d_gate_curr = std::vector<std::vector<gr_complex>>(d_vlen,
					std::vector<gr_complex>(3, gr_complex(0.0f, 0.0f)));
			d_gate_cnt = std::vector<int>(d_vlen, 0);
			d_gate_delta = std::round(d_sps / 8.0f);

			d_run_cnt0 = std::vector<int>(d_vlen, 0);
			d_run_cnt1 = std::vector<int>(d_vlen, 0);

			d_rx_bits = std::vector<std::bitset<40>>(d_vlen, std::bitset<40>());
		}

		demodulator_impl::~demodulator_impl()
		{
			d_socket->close();
			delete d_socket;
			delete d_context;
		}

		void demodulator_impl::timing_sync(const int idx, const gr_complex sample)
		{
			++d_gate_cnt[idx];

			if(d_gate_cnt[idx] == d_sps - d_gate_delta) {

				d_gate_prev[idx][0] = d_gate_curr[idx][0];
				d_gate_curr[idx][0] = sample;

			} else if(d_gate_cnt[idx] == d_sps) {

				d_gate_prev[idx][1] = d_gate_curr[idx][1];
				d_gate_curr[idx][1] = sample;

				demodulate(idx, sample);
				decoding(idx);

			} else if(d_gate_cnt[idx] == d_sps + d_gate_delta) {

				d_gate_prev[idx][2] = d_gate_curr[idx][2];
				d_gate_curr[idx][2] = sample;

				float dist[3] = {0.0f, 0.0f, 0.0f};
				for(int i = 0; i < 3; ++i) {
					if(idx < d_num_pairs) {	// conj sample
						dist[i] = std::abs(std::arg(d_gate_curr[idx][i] * std::conj(d_gate_prev[idx][i])));
					} else {				// magsq sample
						dist[i] = std::abs(d_gate_curr[idx][i] - d_gate_prev[idx][i]);
					}
				}

				if(dist[1] < dist[0] && dist[2] < dist[0]) {
					d_gate_cnt[idx] = d_gate_delta + 1;
				} else if(dist[0] < dist[1] && dist[2] < dist[1]) {
					d_gate_cnt[idx] = d_gate_delta;
				} else {
					d_gate_cnt[idx] = d_gate_delta - 1;
				}
			}
		}

		void demodulator_impl::demodulate(const int idx, const gr_complex sample)
		{
			float dist0, dist1;

			if(idx < d_num_pairs) {	// conj sample
				dist0 = std::abs(std::arg(d_channel0[idx] * std::conj(sample)));
				dist1 = std::abs(std::arg(d_channel1[idx] * std::conj(sample)));
			} else {				// magsq sample
				dist0 = std::abs(d_channel0[idx] - sample);
				dist1 = std::abs(d_channel1[idx] - sample);
			}

			d_rx_bits[idx] <<= 1;
			if(dist0 < dist1) {
				if(d_run_cnt0[idx] > 0) d_gate_cnt[idx] = 0;
				++d_run_cnt0[idx]; d_run_cnt1[idx] = 0;
				d_channel0[idx] = sample;
			} else {
				if(d_run_cnt1[idx] > 0) d_gate_cnt[idx] = 0;
				d_run_cnt0[idx] = 0; ++d_run_cnt1[idx];
				d_channel1[idx] = sample;
				d_rx_bits[idx].set(0);
			}

			if(d_run_cnt0[idx] >= 4 || d_run_cnt1[idx] >= 4) {
				d_run_cnt0[idx] = 0; d_run_cnt1[idx] = 0;
				d_channel0[idx] = d_gate_curr[idx][1];
				d_channel1[idx] = d_gate_prev[idx][1];
			}

		}

		void demodulator_impl::decoding(const int idx)
		{
			// Frame Format (40 coded bits)
			// Preamble		4 bits		(0b1010 -> 4B/5B encoding -> 0b10101)
			// Data			20 bits
			// CRC-8-CCITT	8 bits		(for both preamble and data)
			// Simplified 4B/5B encoding
			// append an flipped bit for less or equal than 5 consecuitive bits
			// just ignore the last bit at the decoder

			const std::bitset<40> &coded = d_rx_bits.at(idx);
			bool flipped;

			// Compare 4 coded bits, skip the first bit for sync
			if(coded.test(39) && !coded.test(38) && coded.test(37) &&
					!coded.test(36) && coded.test(35)) {
				// Preamble detected 
				flipped = false;
			} else if(!coded.test(39) && coded.test(38) && !coded.test(37) &&
					coded.test(36) && !coded.test(35)) {
				// Flipped preamble detected
				flipped = true;
			} else {
				return;
			}

			std::array<uint8_t, 4> bytes{0u, 0u, 0u, 0u};

			for(int i = 0; i < 4; ++i) {
				for(int j = 0; j < 4; ++j) {
					bytes.at(i) += bytes.at(i) +
						(flipped ^ coded.test(39 - (i * 10) - j));
				}
				for(int j = 0; j < 4; ++j) {
					bytes.at(i) += bytes.at(i) +
						(flipped ^ coded.test(34 - (i * 10) - j));
				}
			}

			uint8_t crc = 0u;
			for(int i = 0; i < 4; ++i) {
				crc = CRC_TABLE[crc ^ bytes.at(i)];
			}
			if(crc == 0u) {
				d_zmq_msg res;
				memcpy(res.data, bytes.data(), sizeof(bytes));
				res.idx = static_cast<uint16_t>(idx);
				res.num_antennas = static_cast<uint16_t>(d_num_antennas);

				zmq::message_t msg(sizeof(d_zmq_msg));
				memcpy(msg.data(), &res, sizeof(d_zmq_msg));
				d_socket->send(msg);
			}

		}

		void demodulator_impl::flush_buffer()
		{
			for(int i = 0; i < d_vlen; ++i) {
				d_sample_sum[i] = std::accumulate(d_sample_buf[i].begin(), d_sample_buf[i].end(), gr_complex(0.0f, 0.0f));
			}
		}

		int demodulator_impl::work(int noutput_items,
				gr_vector_const_void_star &input_items,
				gr_vector_void_star &output_items)
		{
			const int nread = noutput_items;
			const gr_complex* in = (const gr_complex*) input_items[0];
			gr_complex sample;

			for(int i = 0; i < nread; ++i) {

				d_sample_idx = (d_sample_idx + 1) % d_sps;

				for(int j = 0; j < d_vlen; ++j) {
					sample = in[i * d_vlen + j];
					d_sample_sum[j] += sample - d_sample_buf[j][d_sample_idx];
					d_sample_buf[j][d_sample_idx] = sample;
					timing_sync(j, d_sample_sum[j]);
				}

				++d_flush_cnt;
				if(d_flush_cnt >= d_symbol_rate) {
					flush_buffer();
					d_flush_cnt = 0;
				}
			}

			return noutput_items;
		}

		const std::vector<uint8_t> demodulator_impl::CRC_TABLE({
				0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
				0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
				0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
				0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
				0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
				0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
				0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
				0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
				0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
				0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
				0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
				0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
				0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
				0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
				0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
				0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
				0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
				0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
				0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
				0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
				0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
				0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
				0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
				0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
				0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
				0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
				0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
				0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
				0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
				0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
				0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
				0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3});
	} /* namespace AnyScatter */
} /* namespace gr */

