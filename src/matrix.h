#pragma once
#include <algorithm> // std::copy_n, std::transform
#include <cassert>
#include <cstddef>
#include <functional> // std::plus, std::minus
#include <numeric> // std::inner_product

namespace ct {

template <typename T>
class Matrix {
private:
  template <typename Iterator>
  class BasicView {
  public:
    BasicView() = default;
    BasicView(const BasicView&) = default;
    BasicView& operator=(const BasicView&) = default;

    std::iterator_traits<Iterator>::reference operator[](size_t idx) const {
      Iterator tmp = _begin;
      std::advance(tmp, idx);
      return *tmp;
    }

    friend bool operator==(const BasicView& lhs, const BasicView& rhs) {
      return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    friend bool operator!=(const BasicView& lhs, const BasicView& rhs) {
      return !(lhs == rhs);
    }

    Iterator begin() const {
      return _begin;
    }

    Iterator end() const {
      return _end;
    }

    friend BasicView operator*(const BasicView& lhs, const BasicView& rhs) {
      std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), std::multiplies<T>());
      return lhs;
    }

  private:
    Iterator _begin;
    Iterator _end;

    BasicView(Iterator begin, Iterator end)
        : _begin(begin)
        , _end(end) {}

    friend Matrix;
  };

  template <typename Iterator>
  class MutableBasicView : public BasicView<Iterator> {
  public:
    const MutableBasicView& operator*=(const T& x) const {
      std::transform(this->begin(), this->end(), this->begin(), [x](T i) {
        return i * x;
      });
      return *this;
    }

  private:
    MutableBasicView(Iterator begin, Iterator end)
        : BasicView<Iterator>(begin, end) {}

    friend Matrix;
  };

  template <typename U>
  class ColumnIterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_cv_t<U>;
    using reference = U&;
    using pointer = U*;

    ColumnIterator() = default;
    ColumnIterator(const ColumnIterator&) = default;

    operator ColumnIterator<const U>() const {
      return ColumnIterator<const U>(_ptr, _col, _step);
    }

    ~ColumnIterator() = default;

    reference operator*() const {
      return _ptr[_col];
    }

    reference operator[](const difference_type idx) const {
      return *(_ptr + (idx * _step) + _col);
    }

    pointer operator->() const {
      return _ptr + _col;
    }

    ColumnIterator& operator++() {
      _ptr += _step;
      return *this;
    }

    ColumnIterator operator++(int) {
      ColumnIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    ColumnIterator& operator--() {
      _ptr -= _step;
      return *this;
    }

    ColumnIterator operator--(int) {
      ColumnIterator tmp = *this;
      --(*this);
      return tmp;
    }

    friend ColumnIterator operator+(difference_type n, const ColumnIterator& other) {
      return other + n;
    }

    friend ptrdiff_t operator-(const ColumnIterator& lhs, const ColumnIterator& rhs) {
      return (lhs._ptr - rhs._ptr) / lhs._step;
    }

    ColumnIterator& operator+=(const difference_type n) {
      _ptr += n * _step;
      return *this;
    }

    ColumnIterator& operator-=(const difference_type n) {
      _ptr -= n * _step;
      return *this;
    }

    ColumnIterator operator+(const difference_type n) const {
      ColumnIterator tmp = *this;
      tmp += n;
      return tmp;
    }

    ColumnIterator operator-(const difference_type n) const {
      ColumnIterator tmp = *this;
      tmp -= n;
      return tmp;
    }

    friend bool operator==(const ColumnIterator& lhs, const ColumnIterator& rhs) {
      return lhs._ptr == rhs._ptr && lhs._col == rhs._col;
    }

    friend bool operator!=(const ColumnIterator& lhs, const ColumnIterator& rhs) {
      return !(lhs == rhs);
    }

    auto operator<=>(const ColumnIterator& other) const {
      if (_ptr == other._ptr) {
        return _col <=> other._col;
      }
      return _ptr <=> other._ptr;
    }

  private:
    U* _ptr;
    ptrdiff_t _col;
    ptrdiff_t _step;

    ColumnIterator(U* ptr, size_t col, size_t step)
        : _ptr(ptr)
        , _col(static_cast<ptrdiff_t>(col))
        , _step(static_cast<ptrdiff_t>(step)) {}

    template <typename>
    friend class Matrix;
  };

public:
  using ValueType = T;

  using Reference = T&;
  using ConstReference = const T&;

  using Pointer = T*;
  using ConstPointer = const T*;

  using Iterator = Pointer;
  using ConstIterator = ConstPointer;

  using RowIterator = Pointer;
  using ConstRowIterator = ConstPointer;

  using ColIterator = ColumnIterator<T>;
  using ConstColIterator = ColumnIterator<const T>;

  using RowView = MutableBasicView<RowIterator>;
  using ConstRowView = BasicView<ConstRowIterator>;

  using ColView = MutableBasicView<ColIterator>;
  using ConstColView = BasicView<ConstColIterator>;

public:
  // дефолт конструктор
  Matrix()
      : _data(nullptr)
      , _rows(0)
      , _cols(0) {}

  // конструктор с заданными размерам
  Matrix(const std::size_t n_rows, const std::size_t n_cols)
      : Matrix() {
    if (n_cols > 0 && n_rows > 0) {
      _data = new T[n_rows * n_cols]();
      _cols = n_cols;
      _rows = n_rows;
    }
  }

  Matrix(const Matrix& other)
      : Matrix() {
    if (other._data != nullptr) {
      _data = new T[other.cols() * other.rows()];
      _rows = other._rows;
      _cols = other._cols;
      std::copy_n(other.begin(), other.size(), begin());
    }
  }

  template <std::size_t ROWS, std::size_t COLS>
  Matrix(const T (&init)[ROWS][COLS])
      : _rows(ROWS)
      , _cols(COLS) {
    if (ROWS == 0 || COLS == 0) {
      _data = nullptr;
    } else {
      _data = new T[ROWS * COLS];
      auto it = begin();
      for (size_t i = 0; i < rows(); ++i) {
        it = std::copy_n(init[i], _cols, it);
      }
    }
  }

  template <std::size_t ROWS, std::size_t COLS>
  Matrix& operator=(const T (&init)[ROWS][COLS]) {
    Matrix tmp(init);
    swap(tmp);
    return *this;
  }

  void swap(Matrix& other) {
    using std::swap;
    swap(_rows, other._rows);
    swap(_cols, other._cols);
    swap(_data, other._data);
  }

  Matrix& operator=(const Matrix& other) {
    if (this == &other) {
      return *this;
    }
    if (other._data == nullptr) {
      _rows = 0;
      _cols = 0;
      delete[] _data;
      _data = nullptr;
      return *this;
    }
    Matrix tmp = other;
    swap(tmp);
    return *this;
  }

  ~Matrix() {
    delete[] _data;
  }

  // Iterators

  Iterator begin() {
    return _data;
  }

  ConstIterator begin() const {
    return _data;
  }

  Iterator end() {
    return begin() + size();
  }

  ConstIterator end() const {
    return begin() + size();
  }

  RowIterator row_begin(const std::size_t row_idx) {
    return begin() + (cols() * row_idx);
  }

  ConstRowIterator row_begin(const std::size_t row_idx) const {
    return begin() + (cols() * row_idx);
  }

  RowIterator row_end(const std::size_t row_idx) {
    return begin() + (cols() * (row_idx + 1));
  }

  ConstRowIterator row_end(const std::size_t row_idx) const {
    return begin() + (cols() * (row_idx + 1));
  }

  ColIterator col_begin(std::size_t col_idx) {
    return {begin(), col_idx, cols()};
  }

  ConstColIterator col_begin(std::size_t col_idx) const {
    return {begin(), col_idx, cols()};
  }

  ColIterator col_end(std::size_t col_idx) {
    return {end(), col_idx, cols()};
  }

  ConstColIterator col_end(std::size_t col_idx) const {
    return {end(), col_idx, cols()};
  }

  // Views

  RowView row(const std::size_t row_idx) {
    return {row_begin(row_idx), row_end(row_idx)};
  }

  ConstRowView row(const std::size_t row_idx) const {
    return {row_begin(row_idx), row_end(row_idx)};
  }

  ColView col(const std::size_t col_idx) {
    return {col_begin(col_idx), col_end(col_idx)};
  }

  ConstColView col(const std::size_t col_idx) const {
    return {col_begin(col_idx), col_end(col_idx)};
  }

  // Size

  std::size_t rows() const {
    return _rows;
  }

  std::size_t cols() const {
    return _cols;
  }

  std::size_t size() const {
    return rows() * cols();
  }

  bool empty() const {
    return rows() == 0 || cols() == 0;
  }

  // Elements access

  Reference operator[](const std::size_t row_idx, const std::size_t col_idx) {
    return _data[(row_idx * cols()) + col_idx];
  }

  ConstReference operator[](const std::size_t row_idx, const std::size_t col_idx) const {
    return _data[(row_idx * cols()) + col_idx];
  }

  Pointer data() {
    return _data;
  }

  ConstPointer data() const {
    return _data;
  }

  friend bool operator==(const Matrix& lhs, const Matrix& rhs) {
    if (lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols()) {
      return false;
    }
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }

  friend bool operator!=(const Matrix& lhs, const Matrix& rhs) {
    return !(lhs == rhs);
  }

  // Arithmetic operations

  Matrix& operator+=(const Matrix& other) {
    assert(rows() == other.rows() && cols() == other.cols());
    std::transform(begin(), end(), other.begin(), begin(), std::plus());
    return *this;
  }

  Matrix& operator-=(const Matrix& other) {
    assert(rows() == other.rows() && cols() == other.cols());
    std::transform(begin(), end(), other.begin(), begin(), std::minus());
    return *this;
  }

  Matrix& operator*=(ConstReference factor) {
    std::transform(begin(), end(), begin(), [factor](T x) {
      return x * factor;
    });
    return *this;
  }

  friend Matrix operator+(const Matrix& lhs, const Matrix& rhs) {
    assert(lhs.rows() == rhs.rows() && lhs.cols() == rhs.cols());
    Matrix result = lhs;
    result += rhs;
    return result;
  }

  friend Matrix operator-(const Matrix& lhs, const Matrix& rhs) {
    assert(lhs.rows() == rhs.rows() && lhs.cols() == rhs.cols());
    Matrix result = lhs;
    result -= rhs;
    return result;
  }

  friend Matrix operator*(const Matrix& lhs, const Matrix& rhs) {
    assert(lhs.cols() == rhs.rows());
    Matrix result(lhs.rows(), rhs.cols());
    for (size_t row_idx = 0; row_idx < lhs.rows(); row_idx++) {
      for (size_t col_idx = 0; col_idx < rhs.cols(); col_idx++) {
        result[row_idx, col_idx] =
            std::inner_product(lhs.row(row_idx).begin(), lhs.row(row_idx).end(), rhs.col(col_idx).begin(), T{});
      }
    }
    return result;
  }

  Matrix& operator*=(const Matrix& other) {
    assert(cols() == other.rows());
    Matrix result = *this * other;
    swap(result);
    return *this;
  }

  friend Matrix operator*(const Matrix& lhs, ConstReference rhs) {
    Matrix result = lhs;
    for (size_t row_idx = 0; row_idx < lhs.rows(); row_idx++) {
      result.row(row_idx) *= rhs;
    }
    return result;
  }

  friend Matrix operator*(ConstReference lhs, const Matrix& rhs) {
    return rhs * lhs;
  }

private:
  T* _data;
  size_t _rows;
  size_t _cols;
};

} // namespace ct
