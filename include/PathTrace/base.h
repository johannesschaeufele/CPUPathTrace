#ifndef PATHTRACE_BASE_H
#define PATHTRACE_BASE_H

#include <PathTrace/util/vector.h>
#include <PathTrace/util/matrix.h>

#include <random>
#include <cassert>
#include <cmath>

/**
 * A 3-dimensional ray with an origin and a direction
 *
 * Ray directions should always be a vector of length 1
 */
struct Ray {
    //! Starting point of the ray
    vec3<float> origin;

    //! Direction in which the ray is shot from the origin
    vec3<float> dir;
};

class xorshift {
  public:
    xorshift(uint64_t seed) noexcept : m_seed(seed ^ (~seed << 32)) {}

    uint32_t operator()() noexcept {
        uint64_t result = m_seed * 0xD989BCACC137DCD5LLU;
        m_seed ^= m_seed >> 11;
        m_seed ^= m_seed << 31;
        m_seed ^= m_seed >> 18;

        return static_cast<uint32_t>(result >> 32);
    }

    static constexpr uint32_t min() noexcept { return std::numeric_limits<uint32_t>::min(); }
    static constexpr uint32_t max() noexcept { return std::numeric_limits<uint32_t>::max(); }

  private:
    uint64_t m_seed;
};

/**
 * Wrapper class for generating random bits
 */
class RandomEngine {
  private:
    xorshift engine;

  public:
    RandomEngine(auto seed) noexcept : engine(seed) {}

    auto operator()() noexcept { return this->engine(); }

    static constexpr auto min() noexcept { return decltype(engine)::min(); }
    static constexpr auto max() noexcept { return decltype(engine)::max(); }
};

template<typename T, int SIZE>
bool isNormalized(impl::rt_vector<T, SIZE> vec) noexcept {
    return std::abs(vec.getLengthSquared() - static_cast<T>(1.0)) < static_cast<T>(1E-4);
}

#define assertNormalized(x) assert(isNormalized(x)) // NOLINT

template<typename T, int SIZE>
bool isNonNegative(impl::rt_vector<T, SIZE> vec) noexcept {
    for(int i = 0; i < SIZE; i++) {
        // Use negated comparison to return false for NaN values
        if(!(vec[i] >= static_cast<T>(0))) {
            return false;
        }
    }
    return true;
}

#define assertNonNegative(x) assert(isNonNegative(x)) // NOLINT

#define assertFinite(x) assert(std::isfinite(x)) // NOLINT

#endif /* PATHTRACE_BASE_H */
