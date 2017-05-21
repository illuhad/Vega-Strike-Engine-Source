//  Copyright John Maddock 2012.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Basic sanity check that header <boost/math/special_functions/bernoulli.hpp>
// #includes all the files that it needs to.
//
#include <boost/math/special_functions/bernoulli.hpp>
//
// Note this header includes no other headers, this is
// important if this test is to be meaningful:
//
#include "test_compile_result.hpp"

void compile_and_link_test()
{
   check_result<float>(boost::math::bernoulli_b2n<float>(i));
   check_result<double>(boost::math::bernoulli_b2n<double>(i));
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   check_result<long double>(boost::math::bernoulli_b2n<long double>(i));
#endif

   check_result<float>(boost::math::tangent_t2n<float>(i));
   check_result<double>(boost::math::tangent_t2n<double>(i));
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   check_result<long double>(boost::math::tangent_t2n<long double>(i));
#endif
}
