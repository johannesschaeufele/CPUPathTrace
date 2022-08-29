#ifndef PATHTRACE_MESH_H
#define PATHTRACE_MESH_H

#include <PathTrace/scene/object.h>

#include <filesystem>
#include <vector>
#include <memory>
#include <istream>

namespace io {

    /**
     * Loads a triangle mesh from an input stream
     *
     * @param stream The input stream to read from
     * @param transformation An optional transformation matrix to apply to all loaded vertices
     * @param cull_backface When true, the backfaces of triangles will be culled
     * @param smooth Whether to smooth normals, resulting in potentially different normals
     *  for different vertices of the same triangle
     * @return The loaded triangles
     */
    std::vector<Triangle> loadMesh(std::basic_istream<char> &stream, mat4<float> transformation = mat4_identity<float>, bool cull_backface = true,
                                   bool smooth = true);

    /**
     * Loads a triangle mesh from the file at the specified path
     *
     * @param path The path to the file to read from
     * @param transformation An optional transformation matrix to apply to all loaded vertices
     * @param cull_backface When true, the backfaces of triangles will be culled
     * @param smooth Whether to smooth normals, resulting in potentially different normals
     *  for different vertices of the same triangle
     * @return The loaded triangles
     */
    std::vector<Triangle> loadMesh(const std::filesystem::path &path, mat4<float> transformation = mat4_identity<float>, bool cull_backface = true,
                                   bool smooth = true);

}

/**
 * Constructs a flat rectangular surface out of two triangles based on the given points
 * The two given points will form two corners connected by a diagonal of the rectangle
 * Returns an empty vector for invalid arguments
 *
 * @param a First corner of the rectangle
 * @param b Corner of the rectangle lying diagonally opposite of the first corner
 * @param cull_backface Whether to cull the back faces of the triangles
 * @return vector of triangles making up the rectangle, or an empty vector for invalid arguments
 */
std::vector<Triangle> makePlane(vec3<float> a, vec3<float> b, bool cull_backface = false);

/**
 * Constructs a box with 6 rectangular surfaces, made up of triangles
 * The two given points will form two corners connected by a diagonal of the box
 *  that does not lie on any surface of the box
 * Returns an empty vector for invalid arguments
 *
 * @param a First corner of the box
 * @param b Corner of the box lying diagonally opposite in two dimensions,
 *  not sharing the same coordinate value as the first corner in any of the three dimensions
 * @param cull_backface Whether to cull the back faces of the triangles
 * @return vector of triangles making up the box, or an empty vector for invalid arguments
 */
std::vector<Triangle> makeBox(vec3<float> a, vec3<float> b, bool cull_backface = false);

/**
 * Move plain objects into a vector of std::unique_ptrs to objects after encapsulation
 *
 * @tparam T The plain object type
 * @param objects The vector of std::unique_ptrs
 * @param extension The vector of plain objects
 */
template<typename T>
void moveObjects(std::vector<std::unique_ptr<Object>> &objects, std::vector<T> &extension) {
    objects.reserve(objects.size() + extension.size());

    for(auto &t : extension) {
        objects.emplace_back(std::move(std::make_unique<T>(t)));
    }
}

#endif // PATHTRACE_MESH_H
