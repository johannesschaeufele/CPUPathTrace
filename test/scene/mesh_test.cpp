#include <PathTrace/scene/mesh.h>
#include <PathTrace/util/matrix.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <string>
#include <vector>
#include <sstream>

TEST(MeshTest, EmptyMeshTest) { // NOLINT
    std::array<std::string, 3> mesh_sources = {"", "  \t  \n\t  \r\n \r", "\n# f 0 1 2\n"};

    for(const auto &mesh_source : mesh_sources) {
        std::istringstream stream(mesh_source);

        auto triangles = io::loadMesh(stream);

        EXPECT_THAT(triangles.size(), testing::Eq(0)) << mesh_source;
    }
}

TEST(MeshTest, SimpleMeshTest) { // NOLINT
    std::string mesh_source = "v 0 0 0\nv 1 0 0\nv 1 0 1\nv 0 0 1\nf 1 2 3\nf 3 4 1";
    std::istringstream stream(mesh_source);

    auto triangles = io::loadMesh(stream);

    EXPECT_THAT(triangles.size(), testing::Eq(2));
    // TODO: Test triangle vertex positions
}
