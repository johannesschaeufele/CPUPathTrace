#include <PathTrace/scene/mesh.h>

#include <stdexcept>
#include <fstream>
#include <string>
#include <vector>
#include <limits>

namespace impl {

    class ObjParser final {
      private:
        std::basic_istream<char> &stream;
        mat4<float> transformation;

        std::vector<vec3<float>> vertices;
        std::vector<std::vector<std::tuple<int, int>>> vertex_faces;
        std::vector<Triangle> faces;

        bool cull_backface;
        bool smooth;

        bool hasNext() {
            stream.peek();

            return stream.good() && (!stream.eof());
        }

        char peek() {
            if(hasNext()) {
                return static_cast<char>(stream.peek());
            }

            return -1;
        }

        char read() {
            if(hasNext()) {
                return static_cast<char>(stream.get());
            }

            return -1;
        }

        void eatSpace() {
            while(expect(' ')) {
            }
        }

        bool expect(char c) {
            if(!hasNext()) {
                return false;
            }

            bool passed = peek() == c;

            if(passed) {
                read();
            }

            return passed;
        }

        void skipLine() {
            while(hasNext()) {
                auto c = read();
                if(c == '\r' || c == '\n') {
                    return;
                }
            }
        }

        int readInt() {
            eatSpace();

            std::vector<char> buffer;
            while(hasNext()) {
                auto c = read();

                if((c >= '0' && c <= '9') || c == '-' || c == '+' || c == 'e' || c == 'E') {
                    buffer.push_back(c);
                }
                else {
                    break;
                }
            }
            std::string word = {buffer.data(), buffer.size()};

            try {
                return std::stoi(word);
            }
            catch(const std::exception &e) {
                return -1;
            }
        }

        float readFloat() {
            eatSpace();

            std::vector<char> buffer;
            while(hasNext()) {
                auto c = read();

                if((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E') {
                    buffer.push_back(c);
                }
                else {
                    break;
                }
            }
            std::string word = {buffer.data(), buffer.size()};

            try {
                return std::stof(word);
            }
            catch(const std::exception &e) {
                return std::numeric_limits<float>::quiet_NaN();
            }
        }

        void processVertex() {
            auto x = readFloat();
            auto y = readFloat();
            auto z = readFloat();

            vec3<float> vertex = {x, y, z};
            vertex = this->transformation * vertex;

            this->vertices.push_back(vertex);
            this->vertex_faces.emplace_back();
        }

        void processFace() {
            int a = readInt() - 1;
            while(expect('/')) {
                readInt();
            }

            int b = readInt() - 1;
            while(expect('/')) {
                readInt();
            }

            int c = readInt() - 1;
            while(expect('/')) {
                readInt();
            }

            const int vertex_count = static_cast<int>(this->vertices.size());
            if(a < 0 || a >= vertex_count) {
                return;
            }
            if(b < 0 || b >= vertex_count) {
                return;
            }
            if(c < 0 || c >= vertex_count) {
                return;
            }

            const auto &vertex_a = this->vertices[a];
            const auto &vertex_b = this->vertices[b];
            const auto &vertex_c = this->vertices[c];

            // Ensure points are distinct
            // The comparison is inverted, such that the function is exited when any length is NaN
            if(!((vertex_b - vertex_a).getLengthSquared() > 0.0F && (vertex_c - vertex_a).getLengthSquared() > 0.0F &&
                 (vertex_c - vertex_b).getLengthSquared() > 0.0F)) {
                return;
            }

            // Ensure that the three vertices don't form a line by verifying the length of the implied normal vector
            if((cross(vertex_b - vertex_a, vertex_c - vertex_a).getLengthSquared() <= 0.0F)) {
                return;
            }

            Triangle triangle = {vertex_a, vertex_b, vertex_c, this->cull_backface};

            this->vertex_faces[a].push_back(std::make_tuple(this->faces.size(), 0));
            this->vertex_faces[b].push_back(std::make_tuple(this->faces.size(), 1));
            this->vertex_faces[c].push_back(std::make_tuple(this->faces.size(), 2));

            this->faces.push_back(triangle);
        }

        void processLine() {
            eatSpace();

            auto c = read();

            switch(c) {
                default:
                    skipLine();
                    break;
                case '\r':
                case '\n':
                    break;
                case '#':
                    skipLine();
                    break;
                case 'v':
                    if(expect(' ')) {
                        processVertex();
                    }
                    else {
                        skipLine();
                    }
                    break;
                case 'f':
                    if(expect(' ')) {
                        processFace();
                    }
                    else {
                        skipLine();
                    }
                    break;
            }
        }

      public:
        ObjParser(std::basic_istream<char> &stream, mat4<float> transformation, bool cull_backface, bool smooth) :
          stream(stream), transformation(transformation), cull_backface(cull_backface), smooth(smooth) {}

        std::vector<Triangle> parse() {
            while(hasNext()) {
                processLine();
            }

            if(this->smooth) {
                std::vector<vec3<float>> face_normals;
                face_normals.reserve(this->faces.size());

                for(const auto &face : this->faces) {
                    auto face_normal = cross(face.b - face.a, face.c - face.a);

                    face_normals.emplace_back(face_normal);
                }

                for(const auto &face_descriptors : this->vertex_faces) {
                    vec3<float> vertex_normal{};

                    for(auto [face_index, face_vertex] : face_descriptors) {
                        vertex_normal = vertex_normal + face_normals[face_index].normalize();
                    }

                    if(vertex_normal.getLengthSquared() <= 0.0F) {
                        continue;
                    }

                    vertex_normal = vertex_normal.normalize();

                    for(auto [face_index, face_vertex] : face_descriptors) {
                        auto &face = this->faces[face_index];

                        switch(face_vertex) {
                            case 0:
                                face.normal_a = vertex_normal;
                                break;
                            case 1:
                                face.normal_b = vertex_normal;
                                break;
                            case 2:
                                face.normal_c = vertex_normal;
                                break;
                        }
                    }
                }
            }

            return this->faces;
        }
    };

}

namespace io {

    std::vector<Triangle> loadMesh(std::basic_istream<char> &stream, mat4<float> transformation, bool cull_backface, bool smooth) {
        using namespace impl;

        ObjParser parser = {stream, transformation, cull_backface, smooth};
        auto triangles = parser.parse();

        return triangles;
    }

    std::vector<Triangle> loadMesh(const std::filesystem::path &path, mat4<float> transformation, bool cull_backface, bool smooth) {
        std::ifstream stream(path, std::ios_base::in | std::ios_base::binary);

        return loadMesh(stream, transformation, cull_backface, smooth);
    }

}

std::vector<Triangle> makePlane(vec3<float> a, vec3<float> b, bool cull_backface) {
    std::vector<Triangle> triangles;

    auto eps = 1E-4F;

    int plane_dim = -1;
    for(int i = 0; i < 3; i++) {
        if(std::abs(a[i] - b[i]) < eps) {
            plane_dim = i;
        }
    }

    bool others_separate = true;
    for(int i = 0; i < 3; i++) {
        if(i == plane_dim) {
            continue;
        }

        if(std::abs(a[i] - b[i]) < eps) {
            others_separate = false;
        }
    }

    if(plane_dim < 0 || !others_separate) {
        return triangles;
    }

    int dim1 = plane_dim == 0 ? 1 : 0;
    int dim2 = plane_dim == 2 ? 1 : 2;
    (void)dim2;

    auto vec2 = a;
    auto vec4 = b;

    vec2[dim1] = b[dim1];
    vec4[dim1] = a[dim1];

    triangles.reserve(2);
    triangles.emplace_back(a, vec2, b, cull_backface);
    triangles.emplace_back(b, vec4, a, cull_backface);

    return triangles;
}

std::vector<Triangle> makeBox(vec3<float> a, vec3<float> b, bool cull_backface) {
    std::vector<Triangle> triangles;

    auto eps = 1E-4F;

    for(int i = 0; i < 3; i++) {
        if(std::abs(a[i] - b[i]) < eps) {
            return triangles;
        }
    }

    triangles.reserve(6 * 2);

    for(int i = 0; i < 3; i++) {
        vec3<float> plane_a = a;
        vec3<float> plane_b = a;

        for(int dim = 0; dim < 3; dim++) {
            if(dim == i) {
                continue;
            }

            plane_a[dim] = a[dim];
            plane_b[dim] = b[dim];
        }

        auto plane1_triangles = makePlane(plane_a, plane_b, cull_backface);

        plane_a[i] = b[i];
        plane_b[i] = b[i];
        auto plane2_triangles = makePlane(plane_a, plane_b, cull_backface);

        triangles.insert(triangles.end(), plane1_triangles.begin(), plane1_triangles.end());
        triangles.insert(triangles.end(), plane2_triangles.begin(), plane2_triangles.end());
    }

    return triangles;
}
