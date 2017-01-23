#ifndef COMMON_H
#define COMMON_H

#include <functional>
#include <cstdint>
#include <cmath>

#ifdef __NVCC__
#include <iostream>
#endif

namespace hagrid {

/// Returns the number of milliseconds elapsed on the device for the given function
HOST float profile(std::function<void()>);

/// Rounds the division by an integer so that round_div(i, j) * j > i
HOST DEVICE inline int round_div(int i, int j) {
    return i / j + (i % j ? 1 : 0);
}

/// Computes the minimum between two values
template <typename T> HOST DEVICE T min(T a, T b) { return a < b ? a : b; }
/// Computes the maximum between two values
template <typename T> HOST DEVICE T max(T a, T b) { return a > b ? a : b; }
/// Clamps the first value in the range defined by the last two arguments
template <typename T> HOST DEVICE T clamp(T a, T b, T c) { return min(c, max(b, a)); }
/// Swaps the contents of two references
template <typename T> HOST DEVICE void swap(T& a, T& b) { auto tmp = a; a = b; b = tmp; }

/// Reinterprets a values as unsigned int
template <typename U, typename T>
HOST DEVICE U as(T t) {
    union { T t; U u; } v;
    v.t = t;
    return v.u;
}

/// Returns x with the sign of x * y
HOST DEVICE inline float safe_rcp(float x) {
    return x != 0 ? 1.0f / x : copysign(as<float>(0x7f800000u), x);
}

/// Returns x with the sign of x * y
HOST DEVICE inline float prodsign(float x, float y) {
    return as<float>(as<uint32_t>(x) ^ (as<uint32_t>(y) & 0x80000000));
}

/// Converts a float to an ordered float
HOST DEVICE inline uint32_t float_to_ordered(float f) {
    auto u = as<uint32_t>(f);
    auto mask = -(int)(u >> 31u) | 0x80000000u;
    return u ^ mask;
}

/// Converts back an ordered integer to float
HOST DEVICE inline float ordered_to_float(uint32_t u) {
    auto mask = ((u >> 31u) - 1u) | 0x80000000u;
    return as<float>(u ^ mask);
}

/// Computes the cubic root of an integer
HOST DEVICE inline int icbrt(int x) {
    unsigned y = 0;
    for (int s = 30; s >= 0; s = s - 3) {
        y = 2 * y;
        const unsigned b = (3 * y * (y + 1) + 1) << s;
        if (x >= b) {
            x = x - b;
            y = y + 1;
        }
    }
    return y;
}

/// Computes the logarithm in base 2 of an integer such that (1 << log2(x)) >= x
HOST DEVICE inline int ilog2(int x) {
    int p = 1, q = 0;
    while (p < x) {
        p <<= 1;
        q++;
    }
    return q;
}

/// Swaps two blocks of elements of the same size in the same array, preserving order
template <typename T>
HOST DEVICE void block_swap_equal(T* ptr, int a, int b, int n) {
    for (auto i = a, j = b, m = a + n; i != m; ++i, ++j) swap(ptr[i], ptr[j]);
}

/// Swaps two non-overlapping contiguous blocks of elements in the same array, preserving order
template <typename T>
HOST DEVICE void block_swap_contiguous(T* ptr, int a, int b, int c) {
    auto d1 = b - a;
    auto d2 = c - b;

    while (min(d1, d2) > 0) {
        block_swap_equal(ptr, a, d1 < d2 ? c - d1 : b, min(d1, d2));
        c = d1 < d2 ? c - d1 : c;
        a = d1 < d2 ? a : a + d2;
        d1 = b - a;
        d2 = c - b;
    }
}

/// Swaps two non-overlapping disjoint blocks of elements in the same array, preserving order
template <typename T>
HOST DEVICE void block_swap_disjoint(T* ptr, int a, int b, int c, int d) {
    if (d < a) {
        swap(a, c);
        swap(c, d);
    }
    int d1 = b - a;
    int d2 = c - b;
    block_swap_contiguous(ptr, a, b, c);
    block_swap_contiguous(ptr, c - d1, c, d);
    block_swap_contiguous(ptr, a, a + d2, d - d1);
}

#ifdef __NVCC__
#ifndef NDEBUG
#define DEBUG_SYNC() CHECK_CUDA_CALL(cudaDeviceSynchronize())
#else
#define DEBUG_SYNC() do{} while(0)
#endif
#define CHECK_CUDA_CALL(x) check_cuda_call(x, __FILE__, __LINE__)

__host__ static void check_cuda_call(cudaError_t err, const char* file, int line) {
    if (err != cudaSuccess) {
        std::cerr << file << "(" << line << "): " << cudaGetErrorString(err) << std::endl;
        abort();
    }
}

template <typename T>
__host__ void set_global(T& symbol, const T* ptr) {
    size_t size;
    CHECK_CUDA_CALL(cudaGetSymbolSize(&size, symbol));
    CHECK_CUDA_CALL(cudaMemcpyToSymbol(symbol, ptr, size));
}
#endif // __NVCC__

} // namespace hagrid

#endif
