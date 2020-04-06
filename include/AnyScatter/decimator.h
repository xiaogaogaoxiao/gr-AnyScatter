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

#ifndef INCLUDED_ANYSCATTER_DECIMATOR_H
#define INCLUDED_ANYSCATTER_DECIMATOR_H

#include <AnyScatter/api.h>
#include <gnuradio/sync_decimator.h>

namespace gr {
	namespace AnyScatter {

		class ANYSCATTER_API decimator : virtual public gr::sync_decimator
		{
			public:
				typedef boost::shared_ptr<decimator> sptr;
				static sptr make(int num_antennas, float sample_rate, float symbol_rate);
		};

	} // namespace AnyScatter
} // namespace gr

#endif /* INCLUDED_ANYSCATTER_DECIMATOR_H */

