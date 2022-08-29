#ifndef PATHTRACE_MATRIX_H
#define PATHTRACE_MATRIX_H

#include <PathTrace/util/vector.h>

namespace impl {

    template<typename TYPE, int WIDTH, int HEIGHT>
    struct matrix {
        auto operator*(TYPE factor) const noexcept {
            matrix<TYPE, WIDTH, HEIGHT> product;

            for(int i = 0; i < HEIGHT; i++) {
                product[i] = this->rows[i] * factor;
            }

            return product;
        }

        auto operator*(const rt_vector<TYPE, WIDTH> vec) const noexcept {
            rt_vector<TYPE, HEIGHT> product;

            for(int i = 0; i < HEIGHT; i++) {
                product[i] = dot(this->rows[i], vec);
            }

            return product;
        }

        alignas(alignof(TYPE)) rt_vector<TYPE, WIDTH> rows[HEIGHT];
    };

}

/**
 * 3x3 matrix
 *
 * @tparam T scalar type
 */
template<typename T>
using mat3 = impl::matrix<T, 3, 3>;

/**
 * 4x4 matrix, typically used as linear transformations in 3D space with affine coordinates
 *
 * @tparam T scalar type
 */
template<typename T>
struct mat4 final : public impl::matrix<T, 4, 4> {
    impl::rt_vector<T, 3> operator*(const impl::rt_vector<T, 3> vec) const noexcept {
        auto affine = this->matrix::operator*(impl::rt_vector<T, 4>{vec[0], vec[1], vec[2], static_cast<T>(1)});
        affine = affine * (static_cast<T>(1) / affine[3]);

        return {affine[0], affine[1], affine[2]};
    }
};

/**
 * The 4x4 identity matrix
 *
 * @tparam T scalar type
 */
template<typename T>
const mat4<T> mat4_identity{vec4<float>{1.0F, 0.0F, 0.0F, 0.0F}, //
                            vec4<float>{0.0F, 1.0F, 0.0F, 0.0F}, //
                            vec4<float>{0.0F, 0.0F, 1.0F, 0.0F}, //
                            vec4<float>{0.0F, 0.0F, 0.0F, 1.0F}};

#endif // PATHTRACE_MATRIX_H
