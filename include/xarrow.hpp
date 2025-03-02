#include "aligned_vector.hpp"
#include "arrow.hpp"
#include "common.hpp"
#include "data_types.hpp"
#include <cstdlib>
#include <cstring>

namespace xarrow {
constexpr static size_t alignment = 64;
namespace detail {} // namespace detail

template <class T> struct XArrowNonNull {
  constexpr static auto format = type2format<T>();

  XArrowNonNull(std::string_view name)
      : name_(name), data_(), buffers_({nullptr, data_.data()}) {}

  XArrowNonNull(std::string_view name, AlignedVector<T, alignment> &&data)
      : name_(name), data_(std::move(data)), buffers_({nullptr, data_.data()}) {
  }

  auto name() noexcept -> std::string_view { return name_; }
  auto data() noexcept -> AlignedVector<T, alignment> & { return data_; }
  auto data() const noexcept -> const AlignedVector<T, alignment> & {
    return data_;
  }
  void to_schema_ref(ArrowSchema &schema) const {
    schema.format = format;
    schema.name = name_.c_str();
    schema.metadata = nullptr;
    schema.flags = 0;
    schema.n_children = 0;
    schema.children = nullptr;
    schema.dictionary = nullptr;
    schema.release = []() {};
    schema.private_data = nullptr;
  }
  void to_schema_move(ArrowSchema &schema) const {
    schema.format = format;
    auto *t_name = new char[name_.size() + 1];
    std::memcpy(t_name, name_.data(), name_.size() + 1);
    schema.name = t_name;
    schema.metadata = nullptr;
    schema.flags = 0;
    schema.n_children = 0;
    schema.children = nullptr;
    schema.dictionary = nullptr;
    schema.release = [](struct ArrowSchema *now) { delete now->name; };
    schema.private_data = nullptr;
  }
  void to_array_ref(ArrowArray &array) const {
    array.length = data_.size();
    array.null_count = 0;
    array.offset = 0;
    array.n_buffers = 2;
    array.n_children = 0;
    // direclty points to buffers_, as this is reference
    array.buffers = const_cast<const void **>(buffers_.data());
    array.children = nullptr;
    array.dictionary = nullptr;
    array.release = [](struct ArrowArray *_) {};
    array.private_data = nullptr;
  }

  /**
   * @brief construct ArrowArray and transfer ownership
   * Never use XArrow object afterwards, as it has been moved away
   * @param array target output place
   */
  void to_array_move(ArrowArray &array) {
    array.length = data_.size();
    array.null_count = 0;
    array.offset = 0;
    array.n_buffers = 2;
    array.n_children = 0;
    auto *b = new std::array<void *, 2>();
    b->at(0) = nullptr;
    b->at(1) = data_.data();
    data_._leak(); /// NOTICE: ownership transferred to ArrowArray
    array.buffers = const_cast<const void **>(b->data());
    array.children = nullptr;
    array.dictionary = nullptr;
    array.release = [](struct ArrowArray *now) {
      std::free(const_cast<void *>(now->buffers[1]));        // NOLINT
      auto *n = reinterpret_cast<decltype(b)>(now->buffers); // NOLINT
      delete n;
    };
    array.private_data = nullptr;
  }

private:
  std::string name_;
  AlignedVector<T, alignment> data_;
  std::array<void *, 2> buffers_;
};
#define OPT(type, enum_class, format_str) XArrowNonNull<type>, // NOLINT
#define END(type, enum_class, format_str) XArrowNonNull<type>  // NOLINT
using XArrowVariant = std::variant<
#include "types.def"
    >;
#undef OPT
#undef END

template <class T> struct ReleaseManager {
  ReleaseManager() noexcept = default;
  ReleaseManager(const ReleaseManager &) = delete;
  auto operator=(const ReleaseManager &) -> ReleaseManager & = delete;

  ReleaseManager(ReleaseManager &&other) noexcept : data_(other.data_) {
    other.data_.release = nullptr; // takes away ownership
  }
  auto operator=(ReleaseManager &&other) noexcept -> ReleaseManager & {
    if (this != &other) [[likely]] {
      if (data_.release != nullptr) {
        data_.release(&data_);
      }
      data_ = other.data_;
      other.data_.release = nullptr; // takes away ownership
    }
    return *this;
  }
  ~ReleaseManager() {
    if (data_.release != nullptr) {
      data_.release(&data_);
    }
  }

  [[nodiscard]] auto import() -> T * { return &data_; }
  [[nodiscard]] auto schema() const -> const T & { return data_; }

protected:
  T data_{};
};

struct XArrowRawSchema : ReleaseManager<ArrowSchema> {

  XArrowRawSchema(const XArrowRawSchema &other) {
    auto copy_str = [](const char *src, const char **dest) {
      auto fmt_len = std::strlen(src);
      auto *p_fmt = static_cast<char *>(std::malloc(sizeof(char) * fmt_len));
      std::memcpy(p_fmt, src, sizeof(char) * fmt_len);
      *dest = p_fmt;
    };
    copy_str(other.data_.format, &data_.format);
    copy_str(other.data_.name, &data_.name);
    copy_str(other.data_.metadata, &data_.metadata);
  }
};
using XArrowRawArray = ReleaseManager<ArrowArray>;

struct XArrowSchema {
  XArrowSchema(XArrowRawSchema &&schema)
      : type_(format2type_enum(schema.schema().format)),
        name_(schema.schema().name), flags_(schema.schema().flags),
        schema_(std::make_unique<XArrowRawSchema>(std::move(schema))) {}

private:
  Type type_;
  std::string name_;
  int64_t flags_;
  std::vector<XArrowSchema> children_;
  std::unique_ptr<XArrowRawSchema> schema_;
};
struct XArrowArray {};
} // namespace xarrow
