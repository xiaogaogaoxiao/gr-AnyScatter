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

#ifndef INCLUDED_ANYSCATTER_DECIMATOR_IMPL_H
#define INCLUDED_ANYSCATTER_DECIMATOR_IMPL_H

#include <AnyScatter/decimator.h>
#include <numeric>
#include <volk/volk.h>

namespace gr {
	namespace AnyScatter {

		class decimator_impl : public decimator
		{
			private:
				const float d_sample_rate;
				const float d_symbol_rate;
				const int d_decim_rate;
				const int d_num_antennas;
				const int d_num_pairs;
				const int d_vlen;

				std::vector<gr_complex*> d_conj_buf;
				std::vector<float*> d_magsq_buf;

			public:
				decimator_impl(int num_antennas, float sample_rate, float symbol_rate);
				~decimator_impl();

				int work(int noutput_items,
						gr_vector_const_void_star &input_items,
						gr_vector_void_star &output_items);

		};

	} // namespace AnyScatter
} // namespace gr

#endif /* INCLUDED_ANYSCATTER_DECIMATOR_IMPL_H */

