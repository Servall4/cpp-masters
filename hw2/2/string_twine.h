#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

// Node class
class twine {
public:
  std::size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

  std::string str() const {
    std::string out;
    out.reserve(size_); // exact size is known
    append_to(out);
    return out;
  }

  void append_to(std::string &out) const {
    append(out, lhs_);
    append(out, rhs_);
  }

  twine(const twine &) = delete;
  twine &operator=(const twine &) = delete;

protected:
  struct child {
    const twine *node = nullptr; // a subtree, or
    std::string_view view;       // a piece of text (when node == nullptr)
    std::size_t size() const noexcept {
      return node ? node->size_ : view.size();
    }
  };

  twine() = default;

  void set(child l, child r) noexcept {
    lhs_ = l;
    rhs_ = r;
    size_ = l.size() + r.size();
  }

  static child as_child(std::string_view sv) noexcept { return {nullptr, sv}; }
  static child as_child(const twine &t) noexcept {
    // a unary twine (one piece) is folded in: point at its piece, not at the
    // twine
    if (!t.rhs_.node && t.rhs_.view.empty())
      return t.lhs_;
    return {&t, {}};
  }

private:
  static void append(std::string &out, const child &c) {
    if (c.node)
      c.node->append_to(out);
    else
      out.append(c.view);
  }

  child lhs_, rhs_;
  std::size_t size_ = 0;
};

template <class T>
concept twine_piece = std::derived_from<std::remove_cvref_t<T>, twine> ||
                      std::convertible_to<const T &, std::string_view>;

// A twine of N pieces. The root node is the object itself; the N-2 inner
// nodes a balanced tree of N leaves needs are stored inside it, on the stack.
template <std::size_t N> class string_twine : public twine {
  static_assert(N > 0, "string_twine needs at least one piece");

public:
  template <twine_piece... Pieces>
    requires(sizeof...(Pieces) == N)
  explicit string_twine(const Pieces &...pieces) {
    const std::array<child, N> leaves{as_child(pieces)...};
    if constexpr (N == 1) {
      set(leaves[0], {});
    } else {
      std::size_t next = 0;
      build(*this, leaves.data(), 0, N, next);
    }
  }

private:
  struct inner_node : twine {
    using twine::set; // lets string_twine fill its inner nodes
  };

  // Fill `node` with the subtree over leaves[lo, hi), hi - lo >= 2.
  template <class Node>
  void build(Node &node, const child *leaves, std::size_t lo, std::size_t hi,
             std::size_t &next) {
    const std::size_t mid = lo + (hi - lo) / 2;
    node.set(subtree(leaves, lo, mid, next), subtree(leaves, mid, hi, next));
  }

  child subtree(const child *leaves, std::size_t lo, std::size_t hi,
                std::size_t &next) {
    if (hi - lo == 1)
      return leaves[lo];
    inner_node &n = inner_[next++];
    build(n, leaves, lo, hi, next);
    return {&n, {}};
  }

  std::array<inner_node, (N > 2 ? N - 2 : 0)> inner_;
};

template <class... Pieces>
string_twine(const Pieces &...) -> string_twine<sizeof...(Pieces)>;

template <twine_piece A, twine_piece B>
  requires(std::derived_from<std::remove_cvref_t<A>, twine> ||
           std::derived_from<std::remove_cvref_t<B>, twine>)
string_twine<2> operator+(const A &a, const B &b) {
  return string_twine<2>(a, b);
}
