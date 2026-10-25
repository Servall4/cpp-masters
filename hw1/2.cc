// [1.2] Some cases of deref nullptr with no UB

#include <iostream>
#include <type_traits>

int main() {
  int *p = nullptr;

  std::cout << sizeof(*p) << '\n';
  static_assert(std::is_same_v<decltype(*p), int &>);

  using L = decltype([p] { return *static_cast<int *>(p); }());
  static_assert(std::is_same_v<L, int>);

  std::cout << noexcept(*p) << '\n';
  std::cout << requires { *p; } << '\n';
}
