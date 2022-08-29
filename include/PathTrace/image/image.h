#ifndef PATHTRACE_IMAGE_H
#define PATHTRACE_IMAGE_H

#include <PathTrace/util/color.h>

#include <cstddef>
#include <vector>
#include <cassert>
#include <type_traits>

/**
 * The image class represents a 2-dimensional rectangular grid of values
 *
 * By default it represents a grid of RGBA colors, though it can contain
 * any suitable type
 *
 * @tparam T The type of values stored in each grid cell
 */
template<typename T = Color<float>>
class Image {
  public:
    using value_type = T;

    Image() = default;

    /**
     * Constructs an image with the given dimensions
     *
     * @param width The width of the image
     * @param height The height of the image
     */
    Image(int width, int height);

    /**
     * Retrieves the image value at the specified cell
     *
     * @param x x-position of the cell
     * @param y y-position of the cell
     * @return Value stored at the cell
     */
    T operator()(int x, int y) const noexcept;

    /**
     * Provides a reference to the image value at the specified cell
     *
     * @param x x-position of the cell
     * @param y y-position of the cell
     * @return Reference to the value stored at the cell
     */

    T &operator()(int x, int y) noexcept;

    /**
     * Returns the total number of cells or pixels in the image
     *
     * @return Total number of cells
     */
    std::size_t size() const noexcept;

    const T *data() const noexcept;

    T *data() noexcept;

    int getWidth() const noexcept;
    int getHeight() const noexcept;

  protected:
    void assertContainsPoint(int x, int y) const noexcept;

  private:
    int width;
    int height;
    std::vector<T> data_;
};

template<typename T>
Image<T>::Image(int width, int height) : width(width), height(height), data_(width * height) {}

template<typename T>
T Image<T>::operator()(int x, int y) const noexcept {
    assertContainsPoint(x, y);
    return data_[y * width + x];
}

template<typename T>
T &Image<T>::operator()(int x, int y) noexcept {
    assertContainsPoint(x, y);
    return data_[y * width + x];
}

template<typename T>
const T *Image<T>::data() const noexcept {
    return data_.data();
}

template<typename T>
T *Image<T>::data() noexcept {
    return data_.data();
}

template<typename T>
std::size_t Image<T>::size() const noexcept {
    return data_.size();
}

template<typename T>
void Image<T>::assertContainsPoint([[maybe_unused]] int x, [[maybe_unused]] int y) const noexcept {
    assert(x >= 0);
    assert(x < width);
    assert(y >= 0);
    assert(y < height);
}

template<typename T>
int Image<T>::getWidth() const noexcept {
    return width;
}

template<typename T>
int Image<T>::getHeight() const noexcept {
    return height;
}

#endif /* PATHTRACE_IMAGE_H */
