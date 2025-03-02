#include "common.hpp"

namespace xarrow {
template <class T, std::enable_if_t<std::is_pointer<T>::value, bool> = true>
using owner = T;

} // namespace xarrow
