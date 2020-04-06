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

#ifndef INCLUDED_ANYSCATTER_DEMODULATOR_IMPL_H
#define INCLUDED_ANYSCATTER_DEMODULATOR_IMPL_H

#include <AnyScatter/demodulator.h>
#include <zmq.hpp>
#include <bitset>
#include <numeric>

namespace gr {
	namespace AnyScatter {

		class demodulator_impl : public demodulator
		{
			private:
				const float d_symbol_rate;
				const float d_tag_rate;
				const int d_sps;
				const int d_num_antennas;
				const int d_num_pairs;
				const int d_vlen;

				zmq::context_t *d_context;
				zmq::socket_t *d_socket;
				struct d_zmq_msg {
					uint8_t data[4];
					uint16_t idx;
					uint16_t num_antennas;
				};

				std::vector<std::vector<gr_complex>> d_sample_buf;
				std::vector<gr_complex> d_sample_sum;
				int d_sample_idx;
				int d_flush_cnt;

				std::vector<gr_complex> d_channel0;
				std::vector<gr_complex> d_channel1;
				std::vector<std::vector<gr_complex>> d_gate_prev;
				std::vector<std::vector<gr_complex>> d_gate_curr;
				std::vector<int> d_gate_cnt;
				int d_gate_delta;
				std::vector<int> d_run_cnt0;
				std::vector<int> d_run_cnt1;

				std::vector<std::bitset<40>> d_rx_bits;

				void timing_sync(const int idx, const gr_complex sample);
				void demodulate(const int idx, const gr_complex sample);
				void decoding(const int idx);
				void flush_buffer();

				static const std::vector<uint8_t> CRC_TABLE;
			public:
				demodulator_impl(int num_antennas, float symbol_rate, float tag_rate);
				~demodulator_impl();

				// Where all the action really happens
				int work(
						int noutput_items,
						gr_vector_const_void_star &input_items,
						gr_vector_void_star &output_items
						);
		};

	} // namespace AnyScatter
} // namespace gr

#endif /* INCLUDED_ANYSCATTER_DEMODULATOR_IMPL_H */

