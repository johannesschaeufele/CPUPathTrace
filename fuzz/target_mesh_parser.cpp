#include <PathTrace/scene/mesh.h>
#include <PathTrace/util/matrix.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <sstream>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    constexpr int string_offset = 2;

    if(size < string_offset) {
        return EXIT_SUCCESS;
    }

    bool cull_backface = data[0] != 0;
    bool smooth = data[1] != 0;

    auto string_size = size - string_offset;
    auto string_data = data + string_offset;

    std::string data_string(reinterpret_cast<const char *>(string_data), string_size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    std::istringstream data_stream(data_string);

    mat4<float> transformation{vec4<float>{1.0F, 0.0F, 0.0F, 0.0F}, //
                               vec4<float>{0.0F, 1.0F, 0.0F, 0.0F}, //
                               vec4<float>{0.0F, 0.0F, 1.0F, 0.0F}, //
                               vec4<float>{0.0F, 0.0F, 0.0F, 1.0F}};

    auto triangles = io::loadMesh(data_stream, transformation, cull_backface, smooth);

    return EXIT_SUCCESS;
}
