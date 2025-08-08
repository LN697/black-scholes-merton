#pragma once
#include <cmath>
#include <random>
#include <cstdint>
#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884L
#endif

namespace bsm {

inline double norm_cdf(double x) {
    return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
}

struct RNG {
    std::mt19937_64 gen;
    std::normal_distribution<double> nd{0.0, 1.0};
    std::uniform_real_distribution<double> ud{0.0, 1.0};

    RNG() : gen(std::random_device{}()) {}
    explicit RNG(uint64_t seed) : gen(seed) {}

    inline double gauss() { return nd(gen); }
    inline double uni() { return std::min(ud(gen), std::nextafter(1.0, 0.0)); }
};

inline void correlated_gaussians(double rho, RNG &rng, double &z1, double &z2) {
    double u1 = rng.gauss();
    double u2 = rng.gauss();
    z1 = u1;
    z2 = rho * u1 + std::sqrt(std::max(0.0, 1.0 - rho * rho)) * u2;
}

// Simple Halton sequence generator for bases 2 and 3
inline double radical_inverse(uint64_t n, uint32_t base) {
    double inv = 1.0 / static_cast<double>(base);
    double f = inv;
    double result = 0.0;
    while (n > 0) {
        uint64_t digit = n % base;
        result += f * static_cast<double>(digit);
        n /= base;
        f *= inv;
    }
    return result;
}

struct Halton2D {
    uint64_t n{1};
    double shift1{0.0}, shift2{0.0};
    explicit Halton2D(uint64_t seed = 0) {
        RNG r(seed);
        shift1 = r.uni();
        shift2 = r.uni();
    }
    inline void next(double &u1, double &u2) {
        u1 = std::fmod(radical_inverse(n, 2u) + shift1, 1.0);
        u2 = std::fmod(radical_inverse(n, 3u) + shift2, 1.0);
        ++n;
        // avoid exact 0/1
        const double eps = 1e-12;
        if (u1 < eps) {
            u1 = eps;
        }
        if (u1 > 1.0 - eps) {
            u1 = 1.0 - eps;
        }
        if (u2 < eps) {
            u2 = eps;
        }
        if (u2 > 1.0 - eps) {
            u2 = 1.0 - eps;
        }
    }
};

inline void box_muller(double u1, double u2, double &z1, double &z2) {
    double r = std::sqrt(-2.0 * std::log(u1));
    double theta = 2.0 * M_PI * u2;
    z1 = r * std::cos(theta);
    z2 = r * std::sin(theta);
}

// Stateless hash-based RNG helpers (OpenMP-friendly)
inline uint64_t mix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ull;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    x = x ^ (x >> 31);
    return x;
}

inline double u01_from_u64(uint64_t x) {
    // Use top 53 bits for a double in [0,1)
    const double inv = 1.0 / 9007199254740992.0; // 2^53
    return (double)((x >> 11) & 0x1fffffffffffffull) * inv;
}

inline void normals_from_index(uint64_t seed, uint64_t i, uint64_t k, double &z1, double &z2) {
    uint64_t h1 = mix64(seed ^ (i * 0x9e3779b97f4a7c15ull + k * 0x94d049bb133111ebull));
    uint64_t h2 = mix64((seed + 0xdeadbeefcafebabeull) ^ (i * 0xbf58476d1ce4e5b9ull + k * 0x9e3779b97f4a7c15ull));
    double u1 = std::max(1e-12, u01_from_u64(h1));
    double u2 = std::max(1e-12, u01_from_u64(h2));
    box_muller(u1, u2, z1, z2);
}

inline void halton_from_index(uint64_t seed, uint64_t i, double &u1, double &u2) {
    double shift1 = u01_from_u64(mix64(seed));
    double shift2 = u01_from_u64(mix64(seed ^ 0xabcdef0123456789ull));
    u1 = std::fmod(radical_inverse((uint64_t)i + 1, 2u) + shift1, 1.0);
    u2 = std::fmod(radical_inverse((uint64_t)i + 1, 3u) + shift2, 1.0);
    const double eps = 1e-12;
    if (u1 < eps) {
        u1 = eps;
    }
    if (u1 > 1.0 - eps) {
        u1 = 1.0 - eps;
    }
    if (u2 < eps) {
        u2 = eps;
    }
    if (u2 > 1.0 - eps) {
        u2 = 1.0 - eps;
    }
}

}
