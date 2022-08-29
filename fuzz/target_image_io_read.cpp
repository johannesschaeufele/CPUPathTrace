#include <PathTrace/image/image_io.h>
#include <PathTrace/image/image.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <sstream>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    std::string data_string(reinterpret_cast<const char *>(data), size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    std::istringstream data_stream(data_string);

    try {
        auto image = io::readRGBImage(data_stream);
    }
    catch(const std::logic_error &e) {
    }

    return EXIT_SUCCESS;
}
