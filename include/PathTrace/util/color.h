#ifndef PATHTRACE_COLOR_H
#define PATHTRACE_COLOR_H

#include <PathTrace/util/vector.h>

/**
 * RGBA Color type based on scalar numeric type
 *
 * @tparam TYPE scalar type
 */
template<typename TYPE>
struct Color : public impl::rt_vector<TYPE, 4> {
    using T = TYPE;
    static constexpr int SIZE = 4;

    T &r() noexcept { return this->elements[0]; }
    T &g() noexcept { return this->elements[1]; }
    T &b() noexcept { return this->elements[2]; }
    T &a() noexcept { return this->elements[3]; }

    constexpr T r() const noexcept { return this->elements[0]; }
    constexpr T g() const noexcept { return this->elements[1]; }
    constexpr T b() const noexcept { return this->elements[2]; }
    constexpr T a() const noexcept { return this->elements[3]; }

    Color() noexcept = default;
    template<typename... T>
    Color(T... ts) : impl::rt_vector<TYPE, SIZE>{ts...} {}
    Color(const impl::rt_vector<TYPE, SIZE> &&vec) noexcept : impl::rt_vector<TYPE, SIZE>(std::move(vec)) {}
};

#endif /* PATHTRACE_COLOR_H */
