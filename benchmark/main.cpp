#include <PathTrace/base.h>
#include <PathTrace/post_processing.h>
#include <PathTrace/worker.h>
#include <PathTrace/scene/scene.h>
#include <PathTrace/scene/object.h>
#include <PathTrace/scene/mesh.h>
#include <PathTrace/scene/light.h>
#include <PathTrace/camera.h>

#include <benchmark/benchmark.h>

#include <cstdlib>
#include <exception>

void benchmarkRenderScene(benchmark::State &state, const Scene &scene, const Camera &camera) {
    auto image_width = 128;
    auto image_height = 128;
    auto sample_count = 256;

    RenderOptions options{image_width, image_height, sample_count, sample_count, 1E-3F};
    FrameRenderJob job{camera, scene, options};

    for(auto _ : state) {
        auto output_image = processJob(job);

        benchmark::DoNotOptimize(output_image.data());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(image_width * image_height * sample_count);
    // state.SetBytesProcessed(image_width * image_height * sizeof(T));
}

void benchmarkRenderSceneBox(benchmark::State &state) {
    Camera camera({0.0F, 0.0F, -3.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, 1.0F, 1.0F, -1.0F);

    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<LightSource>> light_sources;

    auto lambertian_brdf = std::make_shared<LambertianBRDF>();

    auto box_triangles = makeBox(vec3<float>{-1.0F, -1.0F, -1.0F}, vec3<float>{1.0F, 1.0F, 1.0F});
    moveObjects(objects, box_triangles);

    auto ceiling_light_objects = makePlane(vec3<float>{-0.25F, 1.0F - 0.01F, -0.25F}, vec3<float>{0.25F, 1.0F - 0.01F, 0.25F});
    auto ceiling_light_material =
      std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.0F), 1.0F, Spectrum(Color<float>{1.0F, 1.0F, 1.0F, 1.0F}));
    auto ceiling_light_material_handler = std::make_shared<ConstantMaterialHandler>(ceiling_light_material, lambertian_brdf);
    for(auto &object : ceiling_light_objects) {
        object.setMaterialHandler(ceiling_light_material_handler);
    }
    moveObjects(objects, ceiling_light_objects);

    Scene scene(std::move(objects), std::move(light_sources));

    benchmarkRenderScene(state, scene, camera);
}

void benchmarkRenderSceneDragonBox(benchmark::State &state) {
    Camera camera({0.0F, 0.0F, -3.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, 1.0F, 1.0F, -1.0F);

    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<LightSource>> light_sources;

    auto lambertian_brdf = std::make_shared<LambertianBRDF>();
    auto glass_bdf = std::make_shared<GlassBDF>();

    auto box_triangles = makeBox(vec3<float>{-1.0F, -1.0F, -1.0F}, vec3<float>{1.0F, 1.0F, 1.0F});
    moveObjects(objects, box_triangles);

    auto ceiling_light_objects = makePlane(vec3<float>{-0.25F, 1.0F - 0.01F, -0.25F}, vec3<float>{0.25F, 1.0F - 0.01F, 0.25F}, true);
    auto ceiling_light_material =
      std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.0F), 1.0F, Spectrum(Color<float>{1.0F, 1.0F, 1.0F, 1.0F}));
    auto ceiling_light_material_handler = std::make_shared<ConstantMaterialHandler>(ceiling_light_material, lambertian_brdf);
    for(auto &object : ceiling_light_objects) {
        object.setMaterialHandler(ceiling_light_material_handler);
    }
    moveObjects(objects, ceiling_light_objects);

    {
        mat4<float> transformation{vec4<float>{0.01F, 0.0F, 0.0F, 0.0F}, //
                                   vec4<float>{0.0F, 0.01F, 0.0F, -0.5F}, //
                                   vec4<float>{0.0F, 0.0F, 0.01F, 0.0F}, //
                                   vec4<float>{0.0F, 0.0F, 0.0F, 1.0F}};

        auto mesh_triangles = io::loadMesh("assets/xyzrgb_dragon.obj", transformation, false, true);

        auto dragon_material = std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.0F), 1.5F);
        auto dragon_material_handler = std::make_shared<ConstantMaterialHandler>(dragon_material, glass_bdf);

        for(auto &triangle : mesh_triangles) {
            triangle.setMaterialHandler(dragon_material_handler);
        }

        if(mesh_triangles.empty()) {
            throw std::runtime_error("Failed to load dragon mesh");
        }

        moveObjects(objects, mesh_triangles);
    }

    Scene scene(std::move(objects), std::move(light_sources));

    benchmarkRenderScene(state, scene, camera);
}

void registerBenchmarks() {
    benchmark::RegisterBenchmark("renderSceneBox", &benchmarkRenderSceneBox)->UseRealTime()->Unit(benchmark::TimeUnit::kMillisecond); // NOLINT
    benchmark::RegisterBenchmark("renderSceneDragonBox", &benchmarkRenderSceneDragonBox)->UseRealTime()->Unit(benchmark::TimeUnit::kMillisecond); // NOLINT
}

int main(int argc, char *argv[]) {
    registerBenchmarks();

    benchmark::Initialize(&argc, argv);
    if(benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return EXIT_FAILURE;
    }
    benchmark::RunSpecifiedBenchmarks();

    return EXIT_SUCCESS;
}
