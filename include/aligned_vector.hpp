#include "common.hpp"
#include "utils.hpp"

namespace xarrow {
static constexpr auto align_round(size_t const size, size_t const align) {
  return (size + align - 1) / align * align;
}

template <size_t Alignment> struct AlignedBuffer {
  static_assert(Alignment > 0 && (Alignment & (Alignment - 1)) == 0,
                "Alignment must be a power of 2");

  AlignedBuffer() noexcept : data_(nullptr) {}
  explicit AlignedBuffer(size_t const size)
      : data_(std::aligned_alloc(Alignment, align_round(size, Alignment))) {
    if (data_ == nullptr) {
      throw std::bad_alloc();
    }
  }
  ~AlignedBuffer() noexcept {
    std::free(data_); // NOLINT
  }
  AlignedBuffer(AlignedBuffer &&other) noexcept : data_(other.data_) {
    other.data_ = nullptr;
  }
  auto operator=(AlignedBuffer &&other) noexcept -> AlignedBuffer & {
    if (this != &other) {
      std::swap(data_, other.data_);
      // rvalue other will be destructed afterwards
    }
    return *this;
  }
  AlignedBuffer(const AlignedBuffer &) = delete;
  auto operator=(const AlignedBuffer &) -> AlignedBuffer & = delete;

  [[nodiscard]] auto data() -> void * { return data_; }
  [[nodiscard]] auto data() const -> const void * { return data_; }

  /**
   * @brief Never call this function unless you know what you are doing!
   *
   */
  void _leak() { data_ = nullptr; }

private:
  owner<void *> data_;
};

template <class T, size_t Alignment> struct AlignedArray {
  AlignedArray() noexcept : size_(0), buffer_() {}
  explicit AlignedArray(size_t const element_size)
      : size_(element_size), buffer_(element_size * sizeof(T)) {}
  ~AlignedArray() noexcept = default;

  AlignedArray(AlignedArray const &other) : AlignedArray(other.size_) {
    std::memcpy(data(), other.data(), size_ * sizeof(T));
  }
  auto operator=(AlignedArray const &other) -> AlignedArray & {
    if (this != &other) {
      AlignedArray tmp(other);
      std::swap(tmp);
    }
    return *this;
  }

  AlignedArray(AlignedArray &&other) noexcept
      : size_(std::exchange(other.size_, 0)),
        buffer_(std::move(other.buffer_)) {}
  auto operator=(AlignedArray &&other) noexcept -> AlignedArray & {
    if (this != &other) {
      std::swap(buffer_, other.buffer_);
      std::swap(size_, other.size_);
    }
    return *this;
  }

  [[nodiscard]] auto data() noexcept -> T * {
    return static_cast<T *>(buffer_.data());
  }
  [[nodiscard]] auto data() const noexcept -> T const * {
    return static_cast<T const *>(buffer_.data());
  }
  [[nodiscard]] auto size() const noexcept -> size_t { return size_; }

  [[nodiscard]] auto begin() noexcept -> T * { return data(); }
  [[nodiscard]] auto begin() const noexcept -> T const * { return data(); }
  [[nodiscard]] auto end() noexcept -> T * { return data() + size(); }
  [[nodiscard]] auto end() const noexcept -> T const * {
    return data() + size();
  }

  [[nodiscard]] auto rbegin() noexcept -> T * { return data() + size() - 1; }
  [[nodiscard]] auto rbegin() const noexcept -> T const * {
    return data() + size() - 1;
  }
  [[nodiscard]] auto rend() noexcept -> T * { return data() - 1; }
  [[nodiscard]] auto rend() const noexcept -> T const * { return data() - 1; }

  [[nodiscard]] auto operator[](size_t const index) noexcept -> T & {
    return data()[index];
  }
  [[nodiscard]] auto operator[](size_t const index) const noexcept
      -> T const & {
    return data()[index];
  }
  [[nodiscard]] auto at(size_t const index) -> T & {
    if (index >= size_) {
      throw std::out_of_range("AlignedArray::at: index out of range");
    }
    return data()[index];
  }
  [[nodiscard]] auto at(size_t const index) const -> T const & {
    if (index >= size_) {
      throw std::out_of_range("AlignedArray::at: index out of range");
    }
    return data()[index];
  }

  /**
   * @brief Never call this function unless you know what you are doing!
   *
   */
  void _leak() {
    size_ = 0;
    buffer_._leak();
  }

private:
  size_t size_;
  AlignedBuffer<Alignment> buffer_;
};

template <typename T, size_t Alignment> class AlignedVector {
public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static_assert(std::is_fundamental_v<T>,
                "T must be a fundamental type for AlignedVector");
  static_assert(Alignment >= alignof(T),
                "Alignment must be at least alignof(T)");

  // Constructors
  AlignedVector() noexcept : size_(0), array_(4) {}

  explicit AlignedVector(size_type count, const T &value = T())
      : AlignedVector() {
    resize(count, value);
  }

  AlignedVector(const AlignedVector &other)
      : size_(other.size_), array_(other.array_) {}

  // Assignment operators
  auto operator=(const AlignedVector &other) -> AlignedVector & {
    if (this != &other) {
      AlignedVector<T, Alignment> tmp(other);
      std::swap(size_, tmp.size_);
      std::swap(array_, tmp.array_);
    }
    return *this;
  }

  AlignedVector(AlignedVector &&other) noexcept
      : array_(std::move(other.array_)), size_(std::exchange(other.size_, 0)) {}

  // Destructor
  ~AlignedVector() = default;

  auto operator=(AlignedVector &&other) noexcept -> AlignedVector & {
    if (this != &other) {
      clear();
      array_ = std::move(other.array_);
      size_ = std::exchange(other.size_, 0);
    }
    return *this;
  }

  // Assign methods
  void assign(size_type count, const T &value) {
    clear();
    if (count > 0) {
      reserve(count);
      std::uninitialized_fill_n(data(), count, value);
      size_ = count;
    }
  }
  // Element access
  auto at(size_type pos) -> reference {
    if (pos >= size_) [[unlikely]] {
      throw std::out_of_range("AlignedVector::at: pos out of range");
    }
    return data()[pos];
  }

  auto at(size_type pos) const -> const_reference {
    if (pos >= size_) [[unlikely]] {
      throw std::out_of_range("AlignedVector::at: pos out of range");
    }
    return data()[pos];
  }

  auto operator[](size_type pos) noexcept -> reference { return data()[pos]; }
  auto operator[](size_type pos) const noexcept -> const_reference {
    return data()[pos];
  }

  auto front() noexcept -> reference { return data()[0]; }
  auto front() const noexcept -> const_reference { return data()[0]; }

  auto back() noexcept -> reference { return data()[size_ - 1]; }
  auto back() const noexcept -> const_reference { return data()[size_ - 1]; }

  auto data() noexcept -> pointer { return array_.data(); }
  auto data() const noexcept -> const_pointer { return array_.data(); }

  // Iterators
  auto begin() noexcept -> iterator { return data(); }
  auto begin() const noexcept -> const_iterator { return data(); }

  auto end() noexcept -> iterator { return data() + size_; }
  auto end() const noexcept -> const_iterator { return data() + size_; }

  auto rbegin() noexcept -> reverse_iterator { return reverse_iterator(end()); }
  auto rbegin() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator(end());
  }

  auto rend() noexcept -> reverse_iterator { return reverse_iterator(begin()); }
  auto rend() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator(begin());
  }

  // Capacity
  [[nodiscard]] auto empty() const noexcept -> bool { return size_ == 0; }
  [[nodiscard]] auto size() const noexcept -> size_type { return size_; }
  [[nodiscard]] auto capacity() const noexcept -> size_type {
    return array_.size();
  }

  void reserve(size_type new_cap) {
    if (new_cap > capacity()) {
      reallocate(new_cap);
    }
  }

  void shrink_to_fit() {
    if (size_ < capacity()) {
      reallocate(size_);
    }
  }

  // Modifiers
  void clear() noexcept {
    // clear would not release memory
    size_ = 0;
  }

  void push_back(const T &value) { emplace_back(value); }

  void push_back(T &&value) { emplace_back(std::move(value)); }

  template <typename... Args> auto emplace_back(Args &&...args) -> reference {
    ensure_capacity(size_ + 1);
    new (data() + size_) T(std::forward<Args>(args)...);
    return data()[size_++];
  }

  void pop_back() {
    if (size_ > 0) [[likely]] {
      --size_;
    }
  }

  void resize(size_type count) { resize(count, T()); }

  void resize(size_type count, const value_type &value) {
    if (count > size_) {
      // Grow
      ensure_capacity(count);
      std::uninitialized_fill(data() + size_, data() + count, value);
    }
    size_ = count;
  }

  void swap(AlignedVector &other) noexcept {
    std::swap(size_, other.size_);
    std::swap(array_, other.array_);
  }

  /**
   * @brief Never call  this function unless you know what you are doing!
   *
   */
  void _leak() {
    size_ = 0;
    array_._leak();
  }

#ifndef TEST
private:
#endif
  size_t size_;
  AlignedArray<T, Alignment> array_;

  // Helper methods
  void ensure_capacity(size_type required_capacity) {
    if (required_capacity > capacity()) {
      // Growth strategy similar to std::vector (1.5x or 2x)
      size_type new_capacity =
          std::max(required_capacity, capacity() + (capacity() / 2));
      new_capacity =
          std::max<std::size_t>(new_capacity, 4); // Minimum initial capacity
      reallocate(new_capacity);
    }
  }

  void reallocate(size_type new_capacity) {
    if (new_capacity == 0) {
      size_ = 0;
      AlignedArray<T, Alignment> tmp;
      std::swap(array_, tmp);
      return;
    }

    // Allocate new array
    auto new_array = AlignedArray<T, Alignment>(new_capacity);

    if (size_ > 0) {
      // Move existing elements to new array
      std::memcpy(new_array.data(), data(), size_ * sizeof(T));
    }

    std::swap(new_array, array_);
  }
};

// Non-member functions
template <typename T, size_t Alignment>
auto operator==(const AlignedVector<T, Alignment> &lhs,
                const AlignedVector<T, Alignment> &rhs) -> bool {
  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, size_t Alignment>
auto operator!=(const AlignedVector<T, Alignment> &lhs,
                const AlignedVector<T, Alignment> &rhs) -> bool {
  return !(lhs == rhs);
}

template <typename T, size_t Alignment>
auto operator<(const AlignedVector<T, Alignment> &lhs,
               const AlignedVector<T, Alignment> &rhs) -> bool {
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
                                      rhs.end());
}

template <typename T, size_t Alignment>
auto operator<=(const AlignedVector<T, Alignment> &lhs,
                const AlignedVector<T, Alignment> &rhs) -> bool {
  return !(rhs < lhs);
}

template <typename T, size_t Alignment>
auto operator>(const AlignedVector<T, Alignment> &lhs,
               const AlignedVector<T, Alignment> &rhs) -> bool {
  return rhs < lhs;
}

template <typename T, size_t Alignment>
auto operator>=(const AlignedVector<T, Alignment> &lhs,
                const AlignedVector<T, Alignment> &rhs) -> bool {
  return !(lhs < rhs);
}

template <typename T, size_t Alignment>
void swap(AlignedVector<T, Alignment> &lhs,
          AlignedVector<T, Alignment> &rhs) noexcept {
  lhs.swap(rhs);
}
} // namespace xarrow
