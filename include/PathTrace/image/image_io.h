#ifndef PATHTRACE_IMAGE_IO_H
#define PATHTRACE_IMAGE_IO_H

#include <PathTrace/image/image.h>
#include <PathTrace/util/color.h>

#include <string>
#include <filesystem>
#include <istream>
#include <ostream>

namespace io {

    /**
     * Attempts to read a 2D RGBA image from an input stream
     * Individual color channel values will be mapped to the range [0, 1]
     *
     * @param stream Input stream to read from
     * @return The decoded image
     * @throw std::logic_error when decoding fails
     */
    Image<Color<float>> readRGBImage(std::basic_istream<char> &stream) noexcept(false);

    /**
     * Attempts to read a 2D RGBA image from the file specified by the given path
     * Individual color channel values will be mapped to the range [0, 1]
     *
     * @param path Path to the image file
     * @return The decoded image
     * @throw std::logic_error when decoding fails
     */
    Image<Color<float>> readRGBImage(const std::string &path) noexcept(false);

    /**
     * Attempts to read a 2D RGBA image from the file specified by the given path
     * Individual color channel values will be mapped to the range [0, 1]
     *
     * @param path Path to the image file
     * @return The decoded image
     * @throw std::logic_error when decoding fails
     */
    Image<Color<float>> readRGBImage(const std::filesystem::path &path) noexcept(false);

    /**
     * Writes a 2D RGBA image to an output stream
     * Individual color channel values will be mapped from the range [0, 1]
     *
     * @param stream The stream to encode the image to
     * @param image The image to encode
     * @throw std::logic_error when encoding fails
     */
    void writeRGBImage(std::basic_ostream<char> &stream, const Image<Color<float>> &image) noexcept(false);

    /**
     * Writes a 2D RGBA image to the file specified by the given path
     * Individual color channel values will be mapped from the range [0, 1]
     *
     * @param path The path to the target file
     * @param image The image to encode
     * @throw std::logic_error when encoding fails
     */
    void writeRGBImage(const std::string &path, const Image<Color<float>> &image) noexcept(false);

    /**
     * Writes a 2D RGBA image to the file specified by the given path
     * Individual color channel values will be mapped from the range [0, 1]
     *
     * @param path The path to the target file
     * @param image The image to encode
     * @throw std::logic_error when encoding fails
     */
    void writeRGBImage(const std::filesystem::path &path, const Image<Color<float>> &image) noexcept(false);

} // namespace io

#endif /* PATHTRACE_IMAGE_IO_H */
