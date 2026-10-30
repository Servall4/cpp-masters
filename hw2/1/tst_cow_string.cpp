#include "cow_string.h"

#include <gtest/gtest.h>

TEST(CowString, Unique) {
  cow_string str("Hello, world");

  ASSERT_EQ(str, "Hello, world");
  ASSERT_EQ(str.size(), 12);
  ASSERT_EQ(str.use_count(), 1);

  str.append("!");

  ASSERT_EQ(str, "Hello, world!");
  ASSERT_EQ(str.use_count(), 1);
}

TEST(CowString, Shared) {
  cow_string str("Hello, world");
  cow_string copy(str);

  ASSERT_EQ(copy, "Hello, world");
  ASSERT_EQ(str.use_count(), 2);
  ASSERT_EQ(copy.use_count(), 2);

  {
    cow_string tmp(copy);
    ASSERT_EQ(str.use_count(), 3);
  }

  ASSERT_EQ(str.use_count(), 2);
}

TEST(CowString, MoveAssignment) {
  cow_string a("A");
  cow_string b("B");

  b = std::move(a);

  ASSERT_EQ(b, "A");
  ASSERT_EQ(b.use_count(), 1);
  ASSERT_TRUE(a.empty());
}

TEST(CowString, CopyAssignment) {
  cow_string a("Hello");
  cow_string b("World");

  a = b;

  ASSERT_EQ(a, "World");
  ASSERT_EQ(b, "World");
  ASSERT_EQ(a.use_count(), 2);
  ASSERT_EQ(b.use_count(), 2);
}

TEST(CowString, SelfAssignment) {
  cow_string str("Hello");

  str = str;

  ASSERT_EQ(str, "Hello");
  ASSERT_EQ(str.use_count(), 1);
}

TEST(CowString, Detach) {
  cow_string str("Hello, world");
  cow_string copy(str);

  str.append("!");

  ASSERT_EQ(str.use_count(), 1);

  ASSERT_EQ(copy, "Hello, world");
  ASSERT_EQ(copy.use_count(), 1);
}

TEST(CowString, SelfAppend) {
  cow_string str("Hello");

  str.append(str);

  ASSERT_EQ(str, "HelloHello");
}

TEST(CowString, CopyOutlivesOrig) {
  cow_string copy;

  {
    cow_string orig("Hello");
    copy = orig;

    ASSERT_EQ(orig.use_count(), 2);
  }

  ASSERT_EQ(copy, "Hello");
  ASSERT_EQ(copy.use_count(), 1);
}
