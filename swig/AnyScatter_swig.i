/* -*- c++ -*- */

#define ANYSCATTER_API

%include "gnuradio.i"           // the common stuff

//load generated python docstrings
%include "AnyScatter_swig_doc.i"

%{
#include "AnyScatter/decimator.h"
#include "AnyScatter/demodulator.h"
%}

%include "AnyScatter/decimator.h"
GR_SWIG_BLOCK_MAGIC2(AnyScatter, decimator);
%include "AnyScatter/demodulator.h"
GR_SWIG_BLOCK_MAGIC2(AnyScatter, demodulator);
