#include "common.hpp"

namespace xarrow {
#define OPT(type, enum_name, format_str) enum_name,
#define END(type, enum_name, format_str) enum_name
enum class Type : uint8_t {
#include "types.def"
};
#undef OPT
#undef END

static constexpr auto type_enum2format(Type type) {
#define OPT(type, enum_name, format_str)                                       \
  case Type::enum_name:                                                        \
    return " #format_str ";
#define END(type, enum_name, format_str)                                       \
  case Type::enum_name:                                                        \
    return " #format_str ";

  switch (type) {
#include "types.def"
  default:
    throw std::runtime_error("Unsupported Type");
  }
#undef OPT
#undef END
}

static auto format2type_enum(const char *format) {
#define OPT(type, enum_name, format_str)                                       \
  if (strcmp(format, " #format_str ") == 0) {                                  \
    return Type::enum_name;                                                    \
  }
#define END(type, enum_name, format_str) OPT(type, enum_name, format_str)
#include "types.def"
#undef OPT
#undef END
  throw std::runtime_error("Unsupported format");
}

template <class T> constexpr static auto type2format() {
#define OPT(type, enum_name, format_str)                                       \
  if constexpr (std::is_same_v<T, type>) {                                     \
    return " #format_str ";                                                    \
  }

#define END(type, enum_name, format_str) OPT(type, enum_name, format_str)
#include "types.def"
  static_assert(false, "Unsupported type");
#undef OPT
#undef END
}

} // namespace xarrow
