// [1.3] Protect from UB

int foo(int *a, int base, int off) {
  if (a == nullptr) {
	abort();
  }

  if (off > 0 && base > INT_MAX - off) {
	abort();
  }

  if (off < 0 && base < INT_MIN - off) {
	abort();
  }

  return a[base + off];
}
