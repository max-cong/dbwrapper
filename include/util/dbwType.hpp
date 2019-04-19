#pragma once
namespace dbw
{
#if 0//__cplusplus >= 201703L
// use std::any
#include <any>
#define DBW_ANY std::any
#define DBW_ANY_CAST std::any_cast
#else
#include <boost/any.hpp>
#define DBW_ANY boost::any
#define DBW_ANY_CAST boost::any_cast
#endif




} // namespace dbw