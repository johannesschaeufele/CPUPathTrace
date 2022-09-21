#include <PathTrace/base.h>
#include <PathTrace/post_processing.h>
#include <PathTrace/worker.h>
#include <PathTrace/scene/scene.h>
#include <PathTrace/scene/object.h>
#include <PathTrace/scene/mesh.h>
#include <PathTrace/scene/light.h>
#include <PathTrace/camera.h>
#include <PathTrace/image/image_io.h>

#include <cstdlib>
#include <memory>
#include <filesystem>
#include <random>
#include <functional>
#include <string>
#include <sstream>
#include <iostream>
#include <exception>
#include <atomic>

int main(int argc, char *argv[]) {
    std::filesystem::path output_path = "out.png";

    if(argc == 2) {
        output_path = argv[1]; // NOLINT
    }

    if(argc > 2) {
        const char *program_name = argv[0]; // NOLINT
        std::cerr << "Invalid arguments\nUsage: " << program_name << " [output image path]\nSample usage: " << program_name << " out.png" << std::endl;

        return EXIT_FAILURE;
    }

    int width = 256;
    int height = 256;
    float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
    int min_sample_count = 16;
    int max_sample_count = 64;

    float epsilon = 1.0E-3F;

    auto aperture_size = 0.05F;
    auto focal_plane_dist = 3.5F;

    Camera camera({0.0F, 0.0F, -3.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, 1.0F, 1.0F, -aspect_ratio, //
                  aperture_size, aperture_size, std::make_unique<CircularApertureSampler>(), focal_plane_dist);

    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<LightSource>> light_sources;

    objects.reserve(10);

    auto lambertian_brdf = std::make_shared<LambertianBRDF>();
    auto glass_bdf = std::make_shared<GlassBDF>();

    {
        auto light_intensity = 1.0F;

        float ground_y = -1.0F;
        float ceiling_y = 1.0F;
        float walls_x = 1.0F;
        float walls_z = 1.0F;

        auto ground_objects = makePlane(vec3<float>(20.0F, ground_y, -20.0F), vec3<float>(-20.0F, ground_y, 20.0F), true);
        auto ceiling_objects = makePlane(vec3<float>(-20.0F, ceiling_y, -20.0F), vec3<float>(20.0F, ceiling_y, 20.0F), true);
        auto ceiling_light_objects = makePlane(vec3<float>(-0.25F, ceiling_y - epsilon, -0.25F), vec3<float>(0.25F, ceiling_y - epsilon, 0.25F), true);

        std::vector<Triangle> walls_objects;
        walls_objects.reserve(4 * 2);

        {
            auto wall_objects = makePlane(vec3<float>(-walls_x, ground_y, -walls_z), vec3<float>(walls_x, ceiling_y, -walls_z), true);

            auto wall_material = std::make_shared<ConstantMaterial>(Color<float>(0.0F, 0.0F, 1.0F, 1.0F));
            auto wall_material_handler = std::make_shared<ConstantMaterialHandler>(wall_material, lambertian_brdf);
            for(auto &object : wall_objects) {
                object.setMaterialHandler(wall_material_handler);
            }

            walls_objects.insert(walls_objects.end(), wall_objects.begin(), wall_objects.end());
        }
        {
            auto wall_objects = makePlane(vec3<float>(-walls_x, ground_y, -walls_z), vec3<float>(-walls_x, ceiling_y, walls_z), true);

            auto wall_material = std::make_shared<ConstantMaterial>(Color<float>(1.0F, 0.0F, 0.0F, 1.0F));
            auto wall_material_handler = std::make_shared<ConstantMaterialHandler>(wall_material, lambertian_brdf);
            for(auto &object : wall_objects) {
                object.setMaterialHandler(wall_material_handler);
            }

            walls_objects.insert(walls_objects.end(), wall_objects.begin(), wall_objects.end());
        }
        {
            auto wall_objects = makePlane(vec3<float>(walls_x, ground_y, walls_z), vec3<float>(-walls_x, ceiling_y, walls_z), true);

            auto wall_material = std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.0F));
            auto wall_material_handler = std::make_shared<ConstantMaterialHandler>(wall_material, lambertian_brdf);
            for(auto &object : wall_objects) {
                object.setMaterialHandler(wall_material_handler);
            }

            walls_objects.insert(walls_objects.end(), wall_objects.begin(), wall_objects.end());
        }
        {
            auto wall_objects = makePlane(vec3<float>(walls_x, ground_y, walls_z), vec3<float>(walls_x, ceiling_y, -walls_z), true);

            auto wall_material = std::make_shared<ConstantMaterial>(Color<float>(0.0F, 1.0F, 0.0F, 1.0F));
            auto wall_material_handler = std::make_shared<ConstantMaterialHandler>(wall_material, lambertian_brdf);
            for(auto &object : wall_objects) {
                object.setMaterialHandler(wall_material_handler);
            }

            walls_objects.insert(walls_objects.end(), wall_objects.begin(), wall_objects.end());
        }

        auto ground_material = std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.0F));
        auto ground_material_handler = std::make_shared<ConstantMaterialHandler>(ground_material, lambertian_brdf);
        for(auto &object : ground_objects) {
            object.setMaterialHandler(ground_material_handler);
        }

        auto ceiling_material = std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.0F));
        auto ceiling_material_handler = std::make_shared<ConstantMaterialHandler>(ceiling_material, lambertian_brdf);
        for(auto &object : ceiling_objects) {
            object.setMaterialHandler(ceiling_material_handler);
        }

        auto ceiling_light_material = std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.0F), 1.0F,
                                                                         Spectrum(Color<float>{light_intensity, light_intensity, light_intensity, 1.0F}));
        auto ceiling_light_material_handler = std::make_shared<ConstantMaterialHandler>(ceiling_light_material, lambertian_brdf);
        for(auto &object : ceiling_light_objects) {
            object.setMaterialHandler(ceiling_light_material_handler);
        }

        moveObjects(objects, ground_objects);
        moveObjects(objects, ceiling_objects);
        moveObjects(objects, ceiling_light_objects);
        moveObjects(objects, walls_objects);
    }

    {
        mat4<float> transformation{vec4<float>{0.005F, 0.0F, 0.0F, 0.4F}, //
                                   vec4<float>{0.0F, 0.005F, 0.0F, -0.8F}, //
                                   vec4<float>{0.0F, 0.0F, 0.005F, -0.75F}, //
                                   vec4<float>{0.0F, 0.0F, 0.0F, 1.0F}};

        auto mesh_triangles = io::loadMesh("assets/xyzrgb_dragon.obj", transformation, false, true);

        auto dragon_material = std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.0F), 1.5F);
        auto dragon_material_handler = std::make_shared<ConstantMaterialHandler>(dragon_material, glass_bdf);

        for(auto &triangle : mesh_triangles) {
            triangle.setMaterialHandler(dragon_material_handler);
        }

        if(mesh_triangles.empty()) {
            std::cerr << "Failed to load triangle mesh at assets/xyzrgb_dragon.obj (check the working directory of this program and existence of the obj file)"
                      << std::endl;

            return EXIT_FAILURE;
        }

        moveObjects(objects, mesh_triangles);
    }

    {
        auto radius = 0.5F;
        auto sphere = std::make_unique<Sphere>(vec3<float>(0.5F, -1.0F + radius, 0.5F), radius);

        auto sphere_material = std::make_shared<ConstantMaterial>(Color<float>(0.0F, 0.0F, 1.0F, 1.0F));
        auto sphere_material_handler = std::make_shared<ConstantMaterialHandler>(sphere_material, std::make_shared<MirrorBRDF>(false));
        sphere->setMaterialHandler(sphere_material_handler);

        objects.emplace_back(std::move(sphere));
    }

    {
        auto box_triangles = makeBox(vec3<float>{-1.0F, -1.0F, -1.0F} * 0.3F, vec3<float>{1.0F, 1.0F, 1.0F} * 0.3F);
        std::vector<Triangle> transformed_triangles;
        transformed_triangles.reserve(box_triangles.size());

        float rot_y = 0.25F;

        mat4<float> transformation{vec4<float>{std::cos(rot_y), 0.0F, std::sin(rot_y), -0.5F}, //
                                   vec4<float>{0.0F, 3.0F, 0.0F, -0.25F}, //
                                   vec4<float>{-std::sin(rot_y), 0.0F, std::cos(rot_y), 0.5F}, //
                                   vec4<float>{0.0F, 0.0F, 0.0F, 1.0F}};

        for(auto &triangle : box_triangles) {
            transformed_triangles.emplace_back(transformation * triangle.a, transformation * triangle.b, transformation * triangle.c);
        }

        auto box_material = std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.0F));
        auto box_material_handler = std::make_shared<ConstantMaterialHandler>(box_material, lambertian_brdf);

        for(auto &triangle : transformed_triangles) {
            triangle.setMaterialHandler(box_material_handler);
        }

        moveObjects(objects, transformed_triangles);
    }

    Scene scene(std::move(objects), std::move(light_sources));

    RenderOptions options{width, height, min_sample_count, max_sample_count, epsilon, true};

    FrameRenderJob job{camera, scene, options};

    std::atomic<int> last_line_length = 0;
    auto progress_callback = [&last_line_length](int completed_tiles, int total_tiles) {
        std::stringstream progress_stream;
        progress_stream << "Rendering progress: " << std::fixed << std::setprecision(2)
                        << 100 * (static_cast<float>(completed_tiles) / static_cast<float>(total_tiles)) << "% (" << completed_tiles << " / " << total_tiles
                        << " tiles)";

        std::string progress_string = progress_stream.str();

        int space_count = last_line_length.load(std::memory_order_relaxed);
        last_line_length.store(static_cast<int>(progress_string.length()), std::memory_order_relaxed);

        std::cout << "\r" << std::string(space_count, ' ') << "\r" << progress_string << std::flush;
    };

    auto output_image = processJob(job, progress_callback);

    postProcess(output_image);

    try {
        std::filesystem::create_directories(std::filesystem::absolute(output_path).parent_path());
        io::writeRGBImage(output_path, output_image);
    }
    catch(const std::exception &e) {
        std::cerr << "Failed to write rendered image to output file:\n" << e.what() << std::endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
