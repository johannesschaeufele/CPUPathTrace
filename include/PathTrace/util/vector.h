#ifndef PATHTRACE_VECTOR_H
#define PATHTRACE_VECTOR_H

#include <cstddef>
#include <type_traits>
#include <functional>
#include <cmath>
#include <algorithm>
#include <cassert>

namespace impl {

    template<typename TYPE, int SIZE>
    struct rt_vector {
        using value_type = TYPE;

        auto &operator[](std::size_t index) noexcept { return elements[index]; }

        constexpr auto operator[](std::size_t index) const noexcept { return elements[index]; };

        constexpr size_t size() const noexcept { return SIZE; }

        constexpr bool operator==(const rt_vector<TYPE, SIZE> &other) const noexcept {
            for(int i = 0; i < SIZE; i++) {
                if(this->elements[i] != other[i]) {
                    return false;
                }
            }

            return true;
        };

        constexpr bool operator!=(const rt_vector<TYPE, SIZE> &other) const noexcept { return !this->operator==(other); };

        TYPE *data() noexcept { return &elements[0]; }

        constexpr TYPE *data() const noexcept { return &elements[0]; }

        auto operator-(const rt_vector<TYPE, SIZE> &other) const noexcept {
            rt_vector<TYPE, SIZE> difference;

            for(int i = 0; i < SIZE; i++) {
                difference[i] = this->elements[i] - other[i];
            }

            return difference;
        }

        auto operator+(const rt_vector<TYPE, SIZE> &other) const noexcept {
            rt_vector<TYPE, SIZE> sum;

            for(int i = 0; i < SIZE; i++) {
                sum[i] = this->elements[i] + other[i];
            }

            return sum;
        }

        auto &operator-=(const rt_vector<TYPE, SIZE> &other) noexcept {
            for(int i = 0; i < SIZE; i++) {
                this->elements[i] -= other[i];
            }

            return *this;
        }

        auto &operator+=(const rt_vector<TYPE, SIZE> &other) noexcept {
            for(int i = 0; i < SIZE; i++) {
                this->elements[i] += other[i];
            }

            return *this;
        }

        auto operator*(TYPE factor) const noexcept {
            rt_vector<TYPE, SIZE> product;

            for(int i = 0; i < SIZE; i++) {
                product[i] = this->elements[i] * factor;
            }

            return product;
        }

        auto &operator*=(TYPE factor) noexcept {
            for(int i = 0; i < SIZE; i++) {
                this->elements[i] *= factor;
            }

            return *this;
        }

        auto operator/(TYPE divisor) const noexcept {
            rt_vector<TYPE, SIZE> result;

            for(int i = 0; i < SIZE; i++) {
                result[i] = this->elements[i] / divisor;
            }

            return result;
        }

        auto &operator/=(TYPE divisor) noexcept {
            for(int i = 0; i < SIZE; i++) {
                this->elements[i] /= divisor;
            }

            return *this;
        }

        auto operator*(const rt_vector<TYPE, SIZE> &other) const noexcept {
            rt_vector<TYPE, SIZE> product;

            for(int i = 0; i < SIZE; i++) {
                product[i] = this->elements[i] * other[i];
            }

            return product;
        }

        auto operator-() const noexcept {
            rt_vector<TYPE, SIZE> negated;

            for(int i = 0; i < SIZE; i++) {
                negated[i] = -this->elements[i];
            }

            return negated;
        }

        /**
         * Returns the squared length of the vector with respect to euclidean distance
         *
         * @return Squared length of the vector
         */
        TYPE getLengthSquared() const noexcept {
            TYPE length2 = static_cast<TYPE>(0);

            for(int i = 0; i < SIZE; i++) {
                auto v = this->elements[i];
                length2 += v * v;
            }

            return length2;
        }

        /**
         * Returns the length of the vector with respect to euclidean distance
         *
         * @return Vector length
         */
        TYPE getLength() const noexcept { return std::sqrt(this->getLengthSquared()); }

        /**
         * Normalizes the vector with respect to euclidean distance
         * The behaviour is unspecified if the vector has length 0
         *
         * @return A vector of length 1 in euclidean distance pointing in the same direction as this
         */
        auto normalize() const noexcept {
            auto length = this->getLength();

            TYPE inv_length = static_cast<TYPE>(1) / length;

            return (*this) * inv_length;
        }

        auto normalizeSafely() noexcept {
            auto length = this->getLength();

            if(std::abs(length) > static_cast<TYPE>(0)) {
                return this->normalize();
            }

            return *this;
        }

        alignas(alignof(TYPE)) TYPE elements[SIZE];
    };

}

/**
 * Computes dot product between two vectors
 *
 * @tparam TYPE scalar type
 * @tparam SIZE vector size
 * @param a first vector
 * @param b second vector
 * @return scalar product
 */
template<typename TYPE, int SIZE>
TYPE dot(const impl::rt_vector<TYPE, SIZE> &a, const impl::rt_vector<TYPE, SIZE> &b) noexcept {
    TYPE dot_product = static_cast<TYPE>(0);

    for(int i = 0; i < SIZE; i++) {
        dot_product += a[i] * b[i];
    }

    return dot_product;
}

template<typename TYPE, int SIZE>
impl::rt_vector<TYPE, SIZE> min(const impl::rt_vector<TYPE, SIZE> &a, const impl::rt_vector<TYPE, SIZE> &b) noexcept {
    impl::rt_vector<TYPE, SIZE> result;

    for(int i = 0; i < SIZE; i++) {
        result[i] = std::min(a[i], b[i]);
    }

    return result;
}

template<typename TYPE, int SIZE>
impl::rt_vector<TYPE, SIZE> max(const impl::rt_vector<TYPE, SIZE> &a, const impl::rt_vector<TYPE, SIZE> &b) noexcept {
    impl::rt_vector<TYPE, SIZE> result;

    for(int i = 0; i < SIZE; i++) {
        result[i] = std::max(a[i], b[i]);
    }

    return result;
}

/**
 * Computes the cross product or vector product of the two given vectors
 *
 * @tparam TYPE scalar type
 * @param a first vector
 * @param b second vector
 * @return cross product of the vectors
 */
template<typename TYPE>
impl::rt_vector<TYPE, 3> cross(const impl::rt_vector<TYPE, 3> &a, const impl::rt_vector<TYPE, 3> &b) noexcept {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

/**
 * Reflects vector at a surface with the given normal
 * The vector should point towards the surface,
 *  from above the surface in normal direction
 *
 * @tparam TYPE scalar type
 * @tparam SIZE vector size
 * @param v vector to reflect, should have length 1 and point towards surface
 * @param n surface normal, should have length 1
 * @return reflected vector
 */
template<typename TYPE, int SIZE>
impl::rt_vector<TYPE, SIZE> reflect(const impl::rt_vector<TYPE, SIZE> &v, const impl::rt_vector<TYPE, SIZE> &n) noexcept {
    auto d = dot(v, n);
    auto r = v - n * 2 * d;

    return r;
}

/**
 * 2D vector type
 *
 * @tparam TYPE scalar type
 */
template<typename TYPE>
struct vec2 final : public impl::rt_vector<TYPE, 2> {
    using T = TYPE;
    static constexpr int SIZE = 2;

    T &x() noexcept { return this->elements[0]; }
    T &y() noexcept { return this->elements[1]; }

    T &u() noexcept { return this->elements[0]; }
    T &v() noexcept { return this->elements[1]; }

    constexpr T x() const noexcept { return this->elements[0]; }
    constexpr T y() const noexcept { return this->elements[1]; }

    constexpr T u() const noexcept { return this->elements[0]; }
    constexpr T v() const noexcept { return this->elements[1]; }

    vec2() noexcept = default;
    template<typename... T>
    vec2(T... ts) noexcept : impl::rt_vector<TYPE, SIZE>{ts...} {}
    vec2(const impl::rt_vector<TYPE, SIZE> &&vec) noexcept : impl::rt_vector<TYPE, SIZE>(std::move(vec)) {}
};

/**
 * 3D vector type
 *
 * @tparam TYPE scalar type
 */
template<typename TYPE>
struct vec3 final : public impl::rt_vector<TYPE, 3> {
    using T = TYPE;
    static constexpr int SIZE = 3;

    T &x() noexcept { return this->elements[0]; }
    T &y() noexcept { return this->elements[1]; }
    T &z() noexcept { return this->elements[2]; }

    T &u() noexcept { return this->elements[0]; }
    T &v() noexcept { return this->elements[1]; }
    T &w() noexcept { return this->elements[2]; }

    constexpr T x() const noexcept { return this->elements[0]; }
    constexpr T y() const noexcept { return this->elements[1]; }
    constexpr T z() const noexcept { return this->elements[2]; }

    constexpr T u() const noexcept { return this->elements[0]; }
    constexpr T v() const noexcept { return this->elements[1]; }
    constexpr T w() const noexcept { return this->elements[2]; }

    vec3() noexcept = default;
    template<typename... T>
    vec3(T... ts) noexcept : impl::rt_vector<TYPE, SIZE>{ts...} {}
    vec3(const impl::rt_vector<TYPE, SIZE> &&vec) noexcept : impl::rt_vector<TYPE, SIZE>(std::move(vec)) {}
};

/**
 * 4D vector type, usable as 3D space affine coordinate type
 *
 * @tparam T scalar type
 */
template<typename T>
using vec4 = impl::rt_vector<T, 4>;

#endif /* PATHTRACE_VECTOR_H */
