#pragma once

#include <boost/range/irange.hpp>
#include <gsl/pointers>
#include <memory>

namespace chat {

using gsl::not_null, gsl::owner;
using std::unique_ptr, std::weak_ptr, std::shared_ptr;
using boost::irange;

} // namespace chat
