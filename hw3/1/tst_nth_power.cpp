#include "nth_power.h"

#include <numbers>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using cd = std::complex<double>;

// ============================================================================
// Compile-time checks
// ============================================================================

template <class T, class N>
concept PowCallable = requires(T x, N n) { np::nth_power(x, n); };

// Base categories
static_assert(np::Monoid<int> && !np::MultiplicativeGroup<int>);
static_assert(np::MultiplicativeGroup<double>);
static_assert(np::MultiplicativeGroup<cd>);
static_assert(np::Monoid<np::Matrix<int>> &&
              !np::MultiplicativeGroup<np::Matrix<int>>);
static_assert(np::MultiplicativeGroup<np::Matrix<double>>);
static_assert(np::HasExpLog<np::Matrix<double>> &&
              !np::HasExpLog<np::Matrix<int>>);

// Exponent types
static_assert(!PowCallable<int, bool>);
static_assert(PowCallable<int, double>);
static_assert(PowCallable<double, cd>);
static_assert(PowCallable<np::Matrix<double>, double>);
static_assert(PowCallable<np::Matrix<cd>, cd>);
static_assert(!PowCallable<np::Matrix<double>, cd>,
              "result would leave Matrix<double>");
static_assert(!PowCallable<np::Matrix<int>, double>,
              "integer matrices have no logarithm");

// Result types
static_assert(std::same_as<decltype(np::nth_power(2, 3)), int>);
static_assert(std::same_as<decltype(np::nth_power(2, 0.5)), double>);
static_assert(std::same_as<decltype(np::nth_power(2.0, cd{})), cd>);
static_assert(
    std::same_as<decltype(np::nth_power(np::Matrix<double>{{1}}, 0.5)),
                 np::Matrix<double>>);

// constexpr for integer exponents
static_assert(np::nth_power(3, 4) == 81);
static_assert(np::nth_power(2.0, -2) == 0.25);

// ============================================================================
// Helpers
// ============================================================================

MATCHER_P2(ComplexNear, expected, eps,
           "is within eps of " + ::testing::PrintToString(expected)) {
  return std::abs(arg - expected) <= eps;
}

template <class T>
void ExpectMatrixNear(const np::Matrix<T> &a, const np::Matrix<T> &b,
                      double eps) {
  ASSERT_EQ(a.size(), b.size());
  for (std::size_t r = 0; r < a.size(); ++r)
    for (std::size_t c = 0; c < a.size(); ++c)
      EXPECT_LE(std::abs(a(r, c) - b(r, c)), eps)
          << "at (" << r << ", " << c << ")";
}

// ============================================================================
// Integer exponent
// ============================================================================

TEST(IntegerExponent, Integers) {
  EXPECT_EQ(np::nth_power(2, 10u), 1024);
  EXPECT_EQ(np::nth_power(7, 0), 1);
  EXPECT_EQ(np::nth_power(-2, 3), -8);
  EXPECT_EQ(np::nth_power(2LL, 62), 1LL << 62);
  EXPECT_THROW(np::nth_power(2, -1), std::domain_error);
}

TEST(IntegerExponent, Reals) {
  EXPECT_DOUBLE_EQ(np::nth_power(2.0, -3), 0.125);
  EXPECT_DOUBLE_EQ(np::nth_power(1.5, 7), std::pow(1.5, 7));
  EXPECT_DOUBLE_EQ(np::nth_power(2.0, INT_MIN), 0.0);
}

TEST(IntegerExponent, Complex) {
  const cd i{0, 1};
  EXPECT_THAT(np::nth_power(i, -1), ComplexNear(-i, 1e-15));
  EXPECT_THAT(np::nth_power(i, 0), ComplexNear(cd{1, 0}, 1e-15));
  EXPECT_THAT(np::nth_power(i, 1), ComplexNear(i, 1e-15));
  EXPECT_THAT(np::nth_power(i, 2), ComplexNear(cd{-1, 0}, 1e-15));
  EXPECT_THAT(np::nth_power(i, 3), ComplexNear(-i, 1e-15));
  EXPECT_THAT(np::nth_power(i, 4), ComplexNear(cd{1, 0}, 1e-15));
}

TEST(IntegerExponent, Matrix) {
  // Q^n = [[F(n+1), F(n)], [F(n), F(n-1)]]
  const np::Matrix<long long> fib{{1, 1}, {1, 0}};
  EXPECT_EQ(np::nth_power(fib, 90)(0, 1), 2880067194370816120LL); // F(90)
  EXPECT_EQ(np::nth_power(fib, 0), np::Matrix<long long>::identity(2));

  const np::Matrix<double> a{{2, 1}, {1, 3}};
  ExpectMatrixNear(np::nth_power(a, 3) * np::nth_power(a, -3),
                   np::Matrix<double>::identity(2), 1e-9);
}

TEST(IntegerExponent, MatrixErrors) {
  const np::Matrix<int> mi{{1, 1}, {0, 1}};
  EXPECT_THROW(np::nth_power(mi, -1), std::domain_error); // monoid: no inverse

  const np::Matrix<double> singular{{1, 2}, {2, 4}};
  EXPECT_THROW(np::nth_power(singular, -1), std::domain_error);

  const np::Matrix<double> swap{{0, 1}, {1, 0}}; // needs pivoting
  EXPECT_EQ(np::nth_power(swap, -1), swap);
}

// ============================================================================
// Real exponent: scalars
// ============================================================================

TEST(RealExponent, MatchesStdPow) {
  EXPECT_DOUBLE_EQ(np::nth_power(1.5, 1.2), std::pow(1.5, 1.2));
  EXPECT_DOUBLE_EQ(np::nth_power(10.0, -1.5), std::pow(10.0, -1.5));
  EXPECT_FLOAT_EQ(np::nth_power(9.0f, 0.5f), 3.0f);
}

TEST(RealExponent, Roots) {
  EXPECT_DOUBLE_EQ(np::nth_power(2.0, 0.5), std::sqrt(2.0));
  EXPECT_DOUBLE_EQ(np::nth_power(8, 1.0 / 3),
                   std::cbrt(8.0)); // int ^ double -> double
}

TEST(RealExponent, IntegerValuedAgreesWithIntegerPath) {
  for (int n = -5; n <= 5; ++n)
    EXPECT_DOUBLE_EQ(np::nth_power(1.5, double(n)), np::nth_power(1.5, n))
        << "n = " << n;
}

TEST(RealExponent, ZeroBase) {
  EXPECT_EQ(np::nth_power(0.0, 2.5), 0.0);
  EXPECT_EQ(np::nth_power(0.0, 0.0), 1.0);
  EXPECT_EQ(np::nth_power(0.0, -0.5), std::numeric_limits<double>::infinity());
}

TEST(RealExponent, NegativeBase) {
  EXPECT_DOUBLE_EQ(np::nth_power(-2.0, 3.0),
                   -8.0); // integer-valued exponent is fine
  EXPECT_THROW(np::nth_power(-8.0, 1.0 / 3), std::domain_error);
  EXPECT_THROW(np::nth_power(-1, 0.5), std::domain_error);
}

// ============================================================================
// Complex exponent / complex base: scalars (principal branch)
// ============================================================================

TEST(ComplexExponent, IToTheI) {
  // i^i = exp(i * Log i) = exp(i * i*pi/2) = exp(-pi/2), a real number
  const cd i{0, 1};
  EXPECT_THAT(np::nth_power(i, i),
              ComplexNear(cd{std::exp(-std::numbers::pi / 2), 0}, 1e-15));
}

TEST(ComplexExponent, EulerIdentity) {
  // e^(i*pi) = -1, real base with complex exponent gives complex
  EXPECT_THAT(np::nth_power(std::numbers::e, cd{0, std::numbers::pi}),
              ComplexNear(cd{-1, 0}, 1e-15));
}

TEST(ComplexExponent, MatchesExpLogDefinition) {
  const cd z{1, 1};
  const cd w{1, 1};
  EXPECT_THAT(np::nth_power(z, w),
              ComplexNear(std::exp(w * std::log(z)), 1e-14));
}

TEST(ComplexExponent, NegativeBaseRealExponentPrincipalRoot) {
  // In complex numbers (-8)^(1/3) has three values; the principal one is 1 +
  // i*sqrt(3), not -2
  EXPECT_THAT(np::nth_power(cd{-8, 0}, 1.0 / 3),
              ComplexNear(cd{1, std::sqrt(3.0)}, 1e-14));
}

TEST(ComplexExponent, SquareRootOfComplex) {
  const cd z{1, 1};
  const cd r = np::nth_power(z, 0.5);
  EXPECT_THAT(r * r, ComplexNear(z, 1e-15));
  EXPECT_GT(r.real(), 0); // principal root lies in the right half-plane
}

// ============================================================================
// Real exponent: matrices
// ============================================================================

TEST(RealExponentMatrix, DiagonalSquareRoot) {
  const np::Matrix<double> d{{4, 0}, {0, 9}};
  ExpectMatrixNear(np::nth_power(d, 0.5), np::Matrix<double>{{2, 0}, {0, 3}},
                   1e-12);
}

TEST(RealExponentMatrix, SquareRootSquaredGivesBack) {
  const np::Matrix<double> a{{4, 1}, {2, 3}}; // eigenvalues 2 and 5
  const auto r = np::nth_power(a, 0.5);
  ExpectMatrixNear(r * r, a, 1e-12);
}

TEST(RealExponentMatrix, IntegerValuedAgreesWithIntegerPath) {
  const np::Matrix<double> a{{2, 1}, {1, 3}};
  ExpectMatrixNear(np::nth_power(a, 3.0), np::nth_power(a, 3), 1e-10);
  ExpectMatrixNear(np::nth_power(a, -2.0), np::nth_power(a, -2), 1e-12);
}

TEST(RealExponentMatrix, ExponentAdditionLaw) {
  const np::Matrix<double> a{{2, 1}, {1, 3}};
  ExpectMatrixNear(np::nth_power(a, 0.3) * np::nth_power(a, 0.7), a, 1e-12);
  ExpectMatrixNear(np::nth_power(a, 0.5) * np::nth_power(a, -0.5),
                   np::Matrix<double>::identity(2), 1e-12);
}

TEST(RealExponentMatrix, NegativeEigenvalueThrows) {
  // diag(-1, 1) has no real square root: the principal logarithm would be
  // complex
  const np::Matrix<double> m{{-1, 0}, {0, 1}};
  EXPECT_THROW(np::nth_power(m, 0.5), std::domain_error);
}

TEST(RealExponentMatrix, ExpAndLog) {
  const np::Matrix<double> a{{0.5, 1}, {-1, 0.5}};
  ExpectMatrixNear(np::exp_of(np::log_of(a)), a, 1e-12);

  // exp([[0, -t], [t, 0]]) is a rotation by t
  const double t = 1.2;
  ExpectMatrixNear(np::exp_of(np::Matrix<double>{{0, -t}, {t, 0}}),
                   np::Matrix<double>{{std::cos(t), -std::sin(t)},
                                      {std::sin(t), std::cos(t)}},
                   1e-13);
}

// ============================================================================
// Complex exponent: matrices
// ============================================================================

TEST(ComplexExponentMatrix, DiagonalMatchesElementwisePow) {
  const np::Matrix<cd> d{{cd{4, 0}, 0}, {0, cd{0, 1}}};
  const cd y{0.5, 0.25};
  ExpectMatrixNear(
      np::nth_power(d, y),
      np::Matrix<cd>{{std::pow(cd{4, 0}, y), 0}, {0, std::pow(cd{0, 1}, y)}},
      1e-12);
}

TEST(ComplexExponentMatrix, RealExponentOnComplexMatrix) {
  // sqrt(diag(i, 4)) = diag(e^(i*pi/4), 2)
  const np::Matrix<cd> d{{cd{0, 1}, 0}, {0, cd{4, 0}}};
  ExpectMatrixNear(
      np::nth_power(d, 0.5),
      np::Matrix<cd>{{std::polar(1.0, std::numbers::pi / 4), 0}, {0, cd{2, 0}}},
      1e-12);
}

TEST(ComplexExponentMatrix, ExponentAdditionLaw) {
  const np::Matrix<cd> a{{cd{2, 1}, cd{1, 0}}, {cd{0, 1}, cd{3, 0}}};
  const cd y{0.4, 0.3};
  ExpectMatrixNear(np::nth_power(a, y) * np::nth_power(a, cd{1, 0} - y), a,
                   1e-11);
}
