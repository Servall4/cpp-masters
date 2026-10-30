#include <algorithm>
#include <cstddef>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

template <class CharT, class Traits = std::char_traits<CharT>>
class basic_cow_string {
public:
  using traits_type = Traits;
  using value_type = CharT;
  using size_type = std::size_t;
  using view_type = std::basic_string_view<CharT, Traits>;

  basic_cow_string() noexcept = default;
  basic_cow_string(const CharT *s) : basic_cow_string(view_type(s)) {}

  explicit basic_cow_string(view_type sv) {
    if (!sv.empty()) {
      rep_ = Rep::create(sv.size());
      Traits::copy(rep_->chars, sv.data(), sv.size());
      rep_->set_size(sv.size());
    }
  }

  basic_cow_string(const basic_cow_string &o) noexcept
      : rep_(o.rep_) { // copy = share
    if (rep_)
      ++rep_->refs;
  }

  basic_cow_string(basic_cow_string &&o) noexcept
      : rep_(std::exchange(o.rep_, nullptr)) {}
  basic_cow_string &operator=(basic_cow_string o) noexcept { // copy-and-swap
    std::swap(rep_, o.rep_);
    return *this;
  }
  ~basic_cow_string() { release(rep_); }

  size_type size() const noexcept { return rep_ ? rep_->size : 0; }
  bool empty() const noexcept { return size() == 0; }
  const CharT *data() const noexcept { return c_str(); }
  const CharT *c_str() const noexcept { return rep_ ? rep_->chars : &kNul; }
  view_type view() const noexcept { return {c_str(), size()}; }
  operator view_type() const noexcept { return view(); }
  const CharT &operator[](size_type i) const noexcept { return c_str()[i]; }
  long use_count() const noexcept { return rep_ ? long(rep_->refs) : 0; }

  basic_cow_string &append(view_type sv) {
    if (sv.empty())
      return *this;
    const size_type old = size(), n = old + sv.size();
    if (is_unique() && rep_->capacity >= n) {
      Traits::move(rep_->chars + old, sv.data(),
                   sv.size()); // sv may point into *this
    } else {
      Rep *r = Rep::create(std::max(n, 2 * capacity()));
      if (old)
        Traits::copy(r->chars, rep_->chars, old);
      Traits::copy(r->chars + old, sv.data(),
                   sv.size()); // old block still alive here
      release(std::exchange(rep_, r));
    }
    rep_->set_size(n);
    return *this;
  }

  void push_back(CharT c) { append(view_type(&c, 1)); }

  friend bool operator==(const basic_cow_string &a,
                         const basic_cow_string &b) noexcept {
    return a.rep_ == b.rep_ || a.view() == b.view(); // same block: O(1)
  }

private:
  struct Rep {
    size_type refs{1};
    size_type size = 0;
    size_type capacity = 0;
    CharT *chars = nullptr;

    static Rep *create(size_type cap) {
      void *raw = ::operator new(sizeof(Rep) + (cap + 1) * sizeof(CharT));
      Rep *r = ::new (raw) Rep;
      r->capacity = cap;
      r->chars =
          ::new (static_cast<std::byte *>(raw) + sizeof(Rep)) CharT[cap + 1];
      return r;
    }
    void set_size(size_type n) noexcept {
      size = n;
      Traits::assign(chars[n], CharT());
    }
  };

  size_type capacity() const noexcept { return rep_ ? rep_->capacity : 0; }

  bool is_unique() const noexcept { return rep_ && rep_->refs == 1; }

  static void release(Rep *r) noexcept {
    if (r && --r->refs == 0) {
      r->~Rep();
      ::operator delete(static_cast<void *>(r));
    }
  }

  // The "copy on write": make sure we own the block alone and it fits min_cap
  // chars.
  void detach(size_type min_cap) {
    if (is_unique() && rep_->capacity >= min_cap)
      return;
    Rep *r = Rep::create(std::max(min_cap, size()));
    if (size())
      Traits::copy(r->chars, rep_->chars, size());
    r->set_size(size());
    release(std::exchange(rep_, r));
  }

  static constexpr CharT kNul{};
  Rep *rep_ = nullptr;
};

using cow_string = basic_cow_string<char>;
