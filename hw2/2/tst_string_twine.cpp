#include "string_twine.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>

using namespace std::string_view_literals;

// --- heap allocation counter
// --------------------------------------------------
static std::size_t g_allocs = 0;

void *operator new(std::size_t n) {
  ++g_allocs;
  if (void *p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return operator new(n); }
void *operator new(std::size_t n, const std::nothrow_t &) noexcept {
  ++g_allocs;
  return std::malloc(n ? n : 1);
}
void *operator new[](std::size_t n, const std::nothrow_t &) noexcept {
  ++g_allocs;
  return std::malloc(n ? n : 1);
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

// --- compile-time properties
// --------------------------------------------------
static_assert(std::is_same_v<decltype(string_twine("a"sv, "b"sv, "c"sv)),
                             string_twine<3>>,
              "CTAD deduces the number of pieces");
static_assert(!std::is_copy_constructible_v<string_twine<3>>,
              "a twine cannot be copied");
static_assert(!std::is_copy_constructible_v<twine>, "a twine cannot be copied");

// --- fixture with common pieces
// -----------------------------------------------
class StringTwineTest : public ::testing::Test {
protected:
  std::string_view hello = "Hello,";
  std::string_view world = "World!";
  std::string owner = "a std::string that is long enough to live on the heap";
};

TEST_F(StringTwineTest, Simple) {
  EXPECT_EQ(string_twine(hello, " ", world).str(), "Hello, World!");
}

TEST_F(StringTwineTest, SizeIsKnownWithoutBuilding) {
  string_twine t(hello, " ", world);
  EXPECT_EQ(t.size(), 13u);
  EXPECT_FALSE(t.empty());
}

TEST_F(StringTwineTest, ConcatenationChain) {
  EXPECT_EQ((string_twine(hello) + " " + world + "!").str(), "Hello, World!!");
  EXPECT_EQ(("<"sv + string_twine(hello) + ">").str(), "<Hello,>");
}

TEST_F(StringTwineTest, TwinesAsPieces) {
  string_twine left(hello, " "sv);
  string_twine right("big"sv, " "sv, world);
  string_twine all(left, "[", right, "]");

  EXPECT_EQ(all.str(), "Hello, [big World!]");
  EXPECT_EQ(all.size(), 19u);
}

TEST_F(StringTwineTest, ManyPieces) {
  string_twine t("a"sv, "b"sv, "c"sv, "d"sv, "e"sv, "f"sv, "g"sv, "h"sv, "i"sv,
                 "j"sv, "k"sv);

  EXPECT_EQ(t.str(), "abcdefghijk");
  EXPECT_EQ(t.size(), 11u);
}

TEST_F(StringTwineTest, SinglePiece) {
  EXPECT_EQ(string_twine("solo"sv).str(), "solo");
}

TEST_F(StringTwineTest, EmptyPieces) {
  EXPECT_TRUE(string_twine(""sv, ""sv).empty());
  EXPECT_EQ(string_twine(""sv, ""sv).str(), "");
  EXPECT_EQ(string_twine(""sv, "x"sv, ""sv).str(), "x");
}

TEST_F(StringTwineTest, StdStringPieces) {
  EXPECT_EQ(string_twine(owner, "!").str(), owner + "!");
}

TEST_F(StringTwineTest, BuildingAllocatesNothingAndStrAllocatesOnce) {
  std::size_t at_str = 0;
  auto render = [&](const twine &t) { // LLVM style: a twine is taken by const&
    at_str = g_allocs;
    return t.str();
  };

  const std::size_t start = g_allocs;
  std::string result = render(string_twine(hello, " ", owner) + " / " +
                              string_twine(world, world, world));
  const std::size_t end = g_allocs;

  EXPECT_EQ(at_str - start, 0u) << "building the tree must not allocate";
  EXPECT_EQ(end - at_str, 1u) << "str() must allocate exactly once";
  EXPECT_EQ(result, "Hello, " + owner + " / World!World!World!");
}
