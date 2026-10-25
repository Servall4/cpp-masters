#include <algorithm>
#include <cmath>
#include <complex>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace np {

// ============================================================================
// 1. Type categories
// ============================================================================

template <class T> inline constexpr bool is_complex_v = false;
template <class T> inline constexpr bool is_complex_v<std::complex<T>> = true;

template <class T>
concept arithmetic = std::integral<T> || std::floating_point<T>;

template <class T>
concept Complex =
    is_complex_v<T> && std::floating_point<typename T::value_type>;

template <class T>
concept Scalar = arithmetic<T> || Complex<T>;

// Element of a field: has division, so matrices over it can be inverted.
template <class T>
concept FieldElement = std::floating_point<T> || Complex<T>;

// ============================================================================
// 2. Customization points for scalars
//
// Built-in types have no associated namespaces, so ADL cannot find these;
// they must be declared before the concepts that use them.
// User types (Matrix below) are found through ADL at instantiation.
// ============================================================================

template <arithmetic T> constexpr T identity_of(const T &) { return T(1); }

template <std::floating_point T>
constexpr std::complex<T> identity_of(const std::complex<T> &) {
  return T(1);
}

template <std::floating_point T> constexpr T inverse(const T &x) {
  return T(1) / x;
}

template <std::floating_point T>
constexpr std::complex<T> inverse(const std::complex<T> &x) {
  return T(1) / x;
}

// ============================================================================
// 3. Algebraic concepts for the base
// ============================================================================

template <class T>
concept Multiplicative = std::copyable<T> && requires(const T &a, const T &b) {
  { a * b } -> std::convertible_to<T>;
};

// Multiplication + identity  ->  x^n for n >= 0.
template <class T>
concept Monoid = Multiplicative<T> && requires(const T &a) {
  { identity_of(a) } -> std::convertible_to<T>;
};

// Monoid + inverse  ->  x^n for any integer n.
// Subsumes Monoid, so its overloads win when both apply.
template <class T>
concept MultiplicativeGroup = Monoid<T> && requires(const T &a) {
  { inverse(a) } -> std::convertible_to<T>;
};

// exp + log  ->  x^y = exp(y * log x) for real/complex y.
template <class T>
concept HasExpLog = requires(const T &a) {
  { exp_of(a) } -> std::convertible_to<T>;
  { log_of(a) } -> std::convertible_to<T>;
};

// y * a stays in T (Matrix<double> cannot be scaled by a complex number).
template <class T, class S>
concept ScalesBy = requires(const S &y, const T &a) {
  { y * a } -> std::convertible_to<T>;
};

// ============================================================================
// 4. Exponent concepts
// ============================================================================

template <class N>
concept IntegerExponent =
    std::integral<N> && !std::same_as<std::remove_cv_t<N>, bool>;

template <class S>
concept RealExponent = std::floating_point<S>;

template <class S>
concept ComplexExponent = Complex<S>;

template <class S>
concept AnalyticExponent = RealExponent<S> || ComplexExponent<S>;

// |n| without overflow: -INT_MIN does not fit into int, but fits into unsigned.
template <IntegerExponent N> constexpr std::make_unsigned_t<N> magnitude(N n) {
  using U = std::make_unsigned_t<N>;
  return n < 0 ? U(0) - static_cast<U>(n) : static_cast<U>(n);
}

// ============================================================================
// 5. nth_power: integer exponent (exact, repeated multiplication)
// ============================================================================

// Unsigned exponent, any monoid: O(log n).
// Trailing zero bits are skipped first, so the identity is never multiplied.
template <Monoid T, std::unsigned_integral N>
  requires IntegerExponent<N>
constexpr T nth_power(T x, N n) {
  if (n == 0)
    return identity_of(x);

  while ((n & 1u) == 0) {
    x = x * x;
    n >>= 1;
  }
  T acc = x;
  n >>= 1;

  while (n != 0) {
    x = x * x;
    if (n & 1u)
      acc = acc * x;
    n >>= 1;
  }
  return acc;
}

// Signed exponent, monoid without inverse (integers, integer matrices).
template <Monoid T, std::signed_integral N>
  requires IntegerExponent<N>
constexpr T nth_power(const T &x, N n) {
  if (n < 0)
    throw std::domain_error(
        "nth_power: negative exponent for a type without inverse");
  return nth_power(x, magnitude(n));
}

// Signed exponent, group (reals, complex, matrices over a field).
// x^-n = (x^n)^-1: one inversion instead of one per step.
template <MultiplicativeGroup T, std::signed_integral N>
  requires IntegerExponent<N>
constexpr T nth_power(const T &x, N n) {
  const T p = nth_power(x, magnitude(n));
  return n < 0 ? inverse(p) : p;
}

// ============================================================================
// 6. nth_power: real / complex exponent (approximate, via exp and log)
// ============================================================================

// Scalars: delegate to std::pow, which is more accurate than exp(y * log x).
// Result type follows std::pow: int^double -> double, double^complex ->
// complex.
template <Scalar T, AnalyticExponent S> auto nth_power(const T &x, const S &y) {
  if constexpr (arithmetic<T> && RealExponent<S>) {
    // No real result; std::pow would return NaN.
    if (x < T(0) && std::trunc(y) != y)
      throw std::domain_error(
          "nth_power: negative base with non-integer real exponent");
  }
  return std::pow(x, y);
}

// Non-scalars with exp and log (matrices over a field).
template <HasExpLog T, AnalyticExponent S>
  requires(!Scalar<T> && ScalesBy<T, S>)
T nth_power(const T &x, const S &y) {
  return exp_of(y * log_of(x));
}

// ============================================================================
// 7. Square matrix
// ============================================================================

template <class T> class Matrix {
public:
  explicit Matrix(std::size_t n) : n_(n), a_(n * n) {}

  Matrix(std::initializer_list<std::initializer_list<T>> rows)
      : n_(rows.size()) {
    a_.reserve(n_ * n_);
    for (const auto &r : rows) {
      if (r.size() != n_)
        throw std::invalid_argument("Matrix: not square");
      a_.insert(a_.end(), r.begin(), r.end());
    }
  }

  static Matrix identity(std::size_t n) {
    Matrix m(n);
    for (std::size_t i = 0; i < n; ++i)
      m(i, i) = T(1);
    return m;
  }

  std::size_t size() const { return n_; }

  T &operator()(std::size_t i, std::size_t j) { return a_[i * n_ + j]; }
  const T &operator()(std::size_t i, std::size_t j) const {
    return a_[i * n_ + j];
  }

  friend bool operator==(const Matrix &, const Matrix &) = default;

  friend Matrix operator*(const Matrix &l, const Matrix &r) {
    check_same_size(l, r);
    Matrix m(l.n_);
    for (std::size_t i = 0; i < l.n_; ++i)
      for (std::size_t k = 0; k < l.n_; ++k) {
        const T lik = l(i, k);
        for (std::size_t j = 0; j < l.n_; ++j)
          m(i, j) += lik * r(k, j);
      }
    return m;
  }

  // Linear operations, used by exp_of / log_of.
  friend Matrix operator+(Matrix l, const Matrix &r) {
    check_same_size(l, r);
    for (std::size_t k = 0; k < l.a_.size(); ++k)
      l.a_[k] += r.a_[k];
    return l;
  }

  friend Matrix operator-(Matrix l, const Matrix &r) {
    check_same_size(l, r);
    for (std::size_t k = 0; k < l.a_.size(); ++k)
      l.a_[k] -= r.a_[k];
    return l;
  }

  friend Matrix operator*(const T &s, Matrix m) {
    for (auto &e : m.a_)
      e *= s;
    return m;
  }

  friend Matrix operator/(Matrix m, const T &s) {
    for (auto &e : m.a_)
      e /= s;
    return m;
  }

  // Frobenius norm; std::norm(e) is |e|^2 for both real and complex e.
  friend double norm(const Matrix &m) {
    double s = 0;
    for (const auto &e : m.a_)
      s += std::norm(e);
    return std::sqrt(s);
  }

private:
  static void check_same_size(const Matrix &l, const Matrix &r) {
    if (l.n_ != r.n_)
      throw std::invalid_argument("Matrix: size mismatch");
  }

  std::size_t n_;
  std::vector<T> a_;
};

// ============================================================================
// 8. Customization points for Matrix (found through ADL)
// ============================================================================

template <class T> Matrix<T> identity_of(const Matrix<T> &m) {
  return Matrix<T>::identity(m.size());
}

// Gauss-Jordan elimination with partial pivoting.
// Only over a field: Matrix<int> stays a monoid, like int itself.
template <FieldElement T> Matrix<T> inverse(Matrix<T> m) {
  const std::size_t n = m.size();
  Matrix<T> inv = Matrix<T>::identity(n);

  for (std::size_t c = 0; c < n; ++c) {
    std::size_t piv = c;
    for (std::size_t r = c + 1; r < n; ++r)
      if (std::abs(m(r, c)) > std::abs(m(piv, c)))
        piv = r;
    if (std::abs(m(piv, c)) == 0)
      throw std::domain_error("Matrix: singular");

    if (piv != c)
      for (std::size_t j = 0; j < n; ++j) {
        std::swap(m(c, j), m(piv, j));
        std::swap(inv(c, j), inv(piv, j));
      }

    const T d = m(c, c);
    for (std::size_t j = 0; j < n; ++j) {
      m(c, j) /= d;
      inv(c, j) /= d;
    }

    for (std::size_t r = 0; r < n; ++r) {
      if (r == c)
        continue;
      const T f = m(r, c);
      if (f == T(0))
        continue;
      for (std::size_t j = 0; j < n; ++j) {
        m(r, j) -= f * m(c, j);
        inv(r, j) -= f * inv(c, j);
      }
    }
  }
  return inv;
}

// exp(A) by scaling and squaring: exp(A) = exp(A / 2^s)^(2^s).
// Taylor series for the small-norm part, then s squarings via nth_power.
template <FieldElement T> Matrix<T> exp_of(const Matrix<T> &a) {
  const std::size_t n = a.size();
  const double nrm = norm(a);
  const unsigned s =
      nrm > 0.5 ? static_cast<unsigned>(std::ceil(std::log2(nrm / 0.5))) : 0u;
  const Matrix<T> b = a / T(std::ldexp(1.0, static_cast<int>(s)));

  Matrix<T> sum = Matrix<T>::identity(n);
  Matrix<T> term = Matrix<T>::identity(n);
  for (int k = 1; k <= 30; ++k) {
    term = (term * b) / T(k);
    sum = sum + term;
    if (norm(term) <= 1e-17 * norm(sum))
      break;
  }
  return nth_power(sum, std::uint64_t{1} << s);
}

// Principal square root, Denman-Beavers iteration: Y -> sqrt(A), Z ->
// sqrt(A)^-1. Converges if A has no eigenvalues on (-inf, 0].
template <FieldElement T> Matrix<T> sqrt_of(const Matrix<T> &a) {
  Matrix<T> y = a;
  Matrix<T> z = Matrix<T>::identity(a.size());
  for (int it = 0; it < 100; ++it) {
    const Matrix<T> yn = (y + inverse(z)) / T(2);
    const Matrix<T> zn = (z + inverse(y)) / T(2);
    const double diff = norm(yn - y);
    y = yn;
    z = zn;
    if (diff <= 1e-15 * norm(y))
      return y;
  }
  throw std::domain_error("Matrix: principal square root does not exist");
}

// log(A) by inverse scaling and squaring: log A = 2^k * log(A^(1/2^k)).
// Take square roots until A is close to E, then use the series
// log(E + X) = X - X^2/2 + X^3/3 - ...
template <FieldElement T> Matrix<T> log_of(Matrix<T> a) {
  const std::size_t n = a.size();
  const Matrix<T> e = Matrix<T>::identity(n);

  int k = 0;
  while (norm(a - e) > 0.25) {
    if (++k > 60)
      throw std::domain_error("Matrix: logarithm does not converge");
    a = sqrt_of(a);
  }

  const Matrix<T> x = a - e;
  Matrix<T> sum(n);
  Matrix<T> pw = x;
  for (int j = 1; j <= 60; ++j) {
    const Matrix<T> t = pw / T(j);
    sum = (j % 2 ? sum + t : sum - t);
    if (norm(t) <= 1e-17 * std::max(norm(sum), 1e-300))
      break;
    pw = pw * x;
  }
  return T(std::ldexp(1.0, k)) * sum;
}
} // namespace np
