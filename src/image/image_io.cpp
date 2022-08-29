#include <PathTrace/image/image_io.h>
#include <PathTrace/image/image.h>
#include <PathTrace/util/color.h>

#include <png.h>

#include <fstream>
#include <filesystem>
#include <string_view>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <csetjmp>

namespace io {

    namespace impl {

        void readPNGCallback(png_structp png_ptr, png_bytep buffer, png_size_t count) {
            png_voidp io_ptr = png_get_io_ptr(png_ptr);
            if(io_ptr == nullptr) {
                png_error(png_ptr, "PNG read error");
                return;
            }

            auto &stream = *reinterpret_cast<std::basic_istream<char> *>(io_ptr); // NOLINT
            stream.read(reinterpret_cast<char *>(buffer), count); // NOLINT
        }

        Image<Color<float>> readPNGImage(std::basic_istream<char> &stream) {
            png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if(png_ptr == nullptr) {
                throw std::logic_error("Couldn't create PNG read struct");
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if(info_ptr == nullptr) {
                png_destroy_read_struct(&png_ptr, nullptr, nullptr);
                throw std::logic_error("Couldn't create PNG info struct");
            }

            // NOLINTNEXTLINE
            if(setjmp(png_jmpbuf(png_ptr))) {
                png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
                throw std::logic_error("Error reading PNG");
            }

            png_set_read_fn(png_ptr, static_cast<void *>(&stream), readPNGCallback);

            int sig_read = 0;
            png_set_sig_bytes(png_ptr, sig_read);

            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_STRIP_16, nullptr);

            std::int32_t width = png_get_image_width(png_ptr, info_ptr);
            std::int32_t height = png_get_image_height(png_ptr, info_ptr);

            Image image = {width, height};

            png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
            int channel_count = png_get_channels(png_ptr, info_ptr);
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    if(channel_count == 3) {
                        float r = static_cast<float>(row_pointers[y][3 * x + 0]) / 255.0F; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        float g = static_cast<float>(row_pointers[y][3 * x + 1]) / 255.0F; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        float b = static_cast<float>(row_pointers[y][3 * x + 2]) / 255.0F; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        image(x, y) = {r, g, b, 1.0F};
                    }
                    else if(channel_count == 4) {
                        float r = static_cast<float>(row_pointers[y][4 * x + 0]) / 255.0F; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        float g = static_cast<float>(row_pointers[y][4 * x + 1]) / 255.0F; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        float b = static_cast<float>(row_pointers[y][4 * x + 2]) / 255.0F; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        float a = static_cast<float>(row_pointers[y][4 * x + 3]) / 255.0F; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        image(x, y) = {r, g, b, a};
                    }
                }
            }

            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

            return image;
        }

        void writePNGCallback(png_structp png_ptr, png_bytep data, png_size_t count) {
            png_voidp io_ptr = png_get_io_ptr(png_ptr);
            if(io_ptr == nullptr) {
                png_error(png_ptr, "PNG write error");
                return;
            }

            auto &stream = *reinterpret_cast<std::basic_ostream<char> *>(io_ptr); // NOLINT
            stream.write(reinterpret_cast<char *>(data), count); // NOLINT
        }

        void flushPNGCallback(png_structp png_ptr) {
            png_voidp io_ptr = png_get_io_ptr(png_ptr);
            if(io_ptr == nullptr) {
                png_error(png_ptr, "PNG write flush error");
                return;
            }

            auto &stream = *reinterpret_cast<std::basic_ostream<char> *>(io_ptr); // NOLINT
            stream.flush();
        }

        void writePNGImage(std::basic_ostream<char> &stream, const Image<Color<float>> &image) {
            png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if(png_ptr == nullptr) {
                throw std::logic_error("Couldn't create PNG write struct");
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if(info_ptr == nullptr) {
                png_destroy_read_struct(&png_ptr, nullptr, nullptr);
                throw std::logic_error("Couldn't create PNG info struct");
            }

            // NOLINTNEXTLINE
            if(setjmp(png_jmpbuf(png_ptr))) {
                png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
                throw std::logic_error("Error writing PNG");
            }

            png_set_write_fn(png_ptr, static_cast<void *>(&stream), writePNGCallback, flushPNGCallback);

            int width = image.getWidth();
            int height = image.getHeight();

            png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

            std::vector<png_bytep> rows(height, nullptr);
            std::vector<std::vector<png_byte>> row_data(height);
            for(int y = 0; y < height; y++) {
                row_data[y] = std::vector<png_byte>(4 * width);
                for(int x = 0; x < width; x++) {
                    row_data[y][4 * x + 0] = std::min(std::max(static_cast<int>(std::round(255.0 * image(x, y).r())), 0), 255);
                    row_data[y][4 * x + 1] = std::min(std::max(static_cast<int>(std::round(255.0 * image(x, y).g())), 0), 255);
                    row_data[y][4 * x + 2] = std::min(std::max(static_cast<int>(std::round(255.0 * image(x, y).b())), 0), 255);
                    row_data[y][4 * x + 3] = std::min(std::max(static_cast<int>(std::round(255.0 * image(x, y).a())), 0), 255);
                }
                rows[y] = row_data[y].data();
            }

            png_set_rows(png_ptr, info_ptr, rows.data());

            png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

            png_destroy_write_struct(&png_ptr, &info_ptr);
        }

    } // namespace impl

    Image<Color<float>> readRGBImage(std::basic_istream<char> &stream) noexcept(false) {
        return impl::readPNGImage(stream);
    }

    Image<Color<float>> readRGBImage(const std::filesystem::path &path) noexcept(false) {
        std::ifstream stream(path, std::ios_base::in | std::ios_base::binary);
        return readRGBImage(stream);
    }

    Image<Color<float>> readRGBImage(const std::string &path) noexcept(false) {
        std::ifstream stream(path, std::ios_base::in | std::ios_base::binary);
        return readRGBImage(stream);
    }

    void writeRGBImage(std::basic_ostream<char> &stream, const Image<Color<float>> &image) noexcept(false) {
        return impl::writePNGImage(stream, image);
    }

    void writeRGBImage(const std::string &path, const Image<Color<float>> &image) noexcept(false) {
        std::ofstream stream(path, std::ios_base::out | std::ios_base::binary);
        return writeRGBImage(stream, image);
    }

    void writeRGBImage(const std::filesystem::path &path, const Image<Color<float>> &image) noexcept(false) {
        std::ofstream stream(path, std::ios_base::out | std::ios_base::binary);
        return writeRGBImage(stream, image);
    }

} // namespace io
