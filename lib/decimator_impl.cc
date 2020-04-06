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
#include "decimator_impl.h"

namespace gr {
	namespace AnyScatter {

		decimator::sptr decimator::make(int num_antennas, float sample_rate, float symbol_rate)
		{
			return gnuradio::get_initial_sptr
				(new decimator_impl(num_antennas, sample_rate, symbol_rate));
		}

		decimator_impl::decimator_impl(int num_antennas, float sample_rate, float symbol_rate)
			: gr::sync_decimator("decimator",
					gr::io_signature::make(num_antennas, num_antennas, sizeof(gr_complex)),
					gr::io_signature::make(1, 1, sizeof(gr_complex) * (num_antennas * (num_antennas + 1) / 2)),
					int(std::round(sample_rate / symbol_rate))),
			d_sample_rate(sample_rate),
			d_symbol_rate(symbol_rate),
			d_decim_rate(std::round(sample_rate / symbol_rate)),
			d_num_antennas(num_antennas),
			d_num_pairs(num_antennas * (num_antennas - 1) / 2),
			d_vlen(num_antennas * (num_antennas + 1) / 2)
		{
			// out[:, : #pairs] -> conj
			// out[:, #pairs :] -> magsq (imag == 0)

			const unsigned int alignment = volk_get_alignment();
			set_alignment(std::max(1, static_cast<int>(alignment / sizeof(gr_complex))));
			for(int i = 0; i < d_num_pairs; ++i) {
				d_conj_buf.push_back((gr_complex*)volk_malloc(8192 * sizeof(gr_complex), alignment));
			}
			for(int i = 0; i < d_num_antennas; ++i) {
				d_magsq_buf.push_back((float*)volk_malloc(8192 * sizeof(float), alignment));
			}
		}

		decimator_impl::~decimator_impl()
		{
			for(auto &buf : d_conj_buf) volk_free(buf);
			for(auto &buf : d_magsq_buf) volk_free(buf);
		}

		int decimator_impl::work(int noutput_items,
				gr_vector_const_void_star &input_items,
				gr_vector_void_star &output_items)
		{
			const int nread = noutput_items * d_decim_rate;
			std::vector<const gr_complex*> in;
			gr_complex* out = (gr_complex *) output_items[0];

			for(int i = 0; i < d_num_antennas; ++i) {
				in.push_back((const gr_complex*) input_items[i]);
			}

			for(int i = 0, pair = 0; i < d_num_antennas; ++i) {
				for(int j = i + 1; j < d_num_antennas; ++j, ++pair) {
					volk_32fc_x2_multiply_conjugate_32fc(d_conj_buf[pair],
							in[i], in[j], nread); 
				}
				volk_32fc_magnitude_squared_32f(d_magsq_buf[i], in[i], nread);
			}

			for(int i = 0; i < noutput_items; ++i) {

				for(int j = 0; j < d_num_pairs; ++j) {
					out[i * d_vlen + j] = std::accumulate(
							&d_conj_buf[j][i * d_decim_rate],
							&d_conj_buf[j][(i + 1) * d_decim_rate],
							gr_complex(0.0f, 0.0f));
				}

				for(int j = 0; j < d_num_antennas; ++j) {
					out[i * d_vlen + d_num_pairs + j] = std::accumulate(
							&d_magsq_buf[j][i * d_decim_rate],
							&d_magsq_buf[j][(i + 1) * d_decim_rate], 0.0f);
				}

			}

			return noutput_items;
		}

	} /* namespace AnyScatter */
} /* namespace gr */

