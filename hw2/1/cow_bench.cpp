#include <benchmark/benchmark.h>
#include <string>
#include <type_traits>
#include <vector>

#include "cow_string.h"

template <class S> S make(std::size_t n) { return S(std::string(n, 'x')); }

// 1. Copy: O(1) for COW, O(n) + malloc for std::string (except SSO-sized strings)
template <class S> static void Copy(benchmark::State& state) {
    S src = make<S>(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        S copy = src;
        benchmark::DoNotOptimize(copy);
    }
}
BENCHMARK_TEMPLATE(Copy, std::string)->Arg(15)->Arg(64)->Arg(1024)->Arg(16384);
BENCHMARK_TEMPLATE(Copy, cow_string)->Arg(15)->Arg(64)->Arg(1024)->Arg(16384);

// 2. Copying a container of strings
template <class S> static void CopyVector(benchmark::State& state) {
    std::vector<S> src;
    for (int i = 0; i < 1000; ++i) src.push_back(make<S>(64));
    for (auto _ : state) {
        std::vector<S> copy = src;
        benchmark::DoNotOptimize(copy.data());
    }
}
BENCHMARK_TEMPLATE(CopyVector, std::string);
BENCHMARK_TEMPLATE(CopyVector, cow_string);

// 3. The same value stored in many places
template <class S> static void FanOut(benchmark::State& state) {
    S tag = make<S>(256);
    for (auto _ : state) {
        std::vector<S> records;
        records.reserve(1000);
        for (int i = 0; i < 1000; ++i) records.push_back(tag);
        benchmark::DoNotOptimize(records.data());
    }
}
BENCHMARK_TEMPLATE(FanOut, std::string);
BENCHMARK_TEMPLATE(FanOut, cow_string);

// 4. Comparing copies: COW sees the same block
template <class S> static void CompareCopies(benchmark::State& state) {
    S a = make<S>(16384);
    S b = a;
    for (auto _ : state) {
        benchmark::DoNotOptimize(a);
        benchmark::DoNotOptimize(b);
        bool eq = (a == b);
        benchmark::DoNotOptimize(eq);
    }
}
BENCHMARK_TEMPLATE(CompareCopies, std::string);
BENCHMARK_TEMPLATE(CompareCopies, cow_string);
