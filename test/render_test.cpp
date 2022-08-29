#include <PathTrace/base.h>
#include <PathTrace/worker.h>
#include <PathTrace/scene/scene.h>
#include <PathTrace/scene/object.h>
#include <PathTrace/scene/light.h>
#include <PathTrace/camera.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <memory>

TEST(RenderTest, EmptySceneRenderTest) { // NOLINT
    Camera camera({0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, 1.0F, 1.0F, 1.0F);

    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<LightSource>> light_sources;

    Scene scene(std::move(objects), std::move(light_sources));

    RenderOptions options{1, 1, 1, 1, 1E-3F};

    FrameRenderJob job{camera, scene, options};

    auto output_image = processJob(job);

    EXPECT_THAT(output_image(0, 0), testing::Eq(Color<float>{0.0F, 0.0F, 0.0F, 0.0F}));
}

TEST(RenderTest, SimpleSceneRenderTest) { // NOLINT
    Camera camera({0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, 0.1F, 1.0F, 1.0F);

    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<LightSource>> light_sources;

    light_sources.emplace_back(std::make_unique<PointLightSource>(vec3<float>{0.0F, 1.0F, 0.0F}, Color<float>{1.0F, 1.0F, 1.0F, 1.0F}));

    auto sphere = std::make_unique<Sphere>(vec3<float>{0.0F, 0.0F, 0.6F}, 0.5F);
    objects.emplace_back(std::move(sphere));

    Scene scene(std::move(objects), std::move(light_sources));

    RenderOptions options{16, 16, 2, 2, 1E-3F};

    FrameRenderJob job{camera, scene, options};

    auto output_image = processJob(job);

    EXPECT_THAT(output_image(0, 0), testing::Eq(Color<float>{0.0F, 0.0F, 0.0F, 0.0F}));
    EXPECT_THAT(output_image(8, 8)[3], testing::Gt(0.0F));
}

TEST(RenderTest, AdvancedSceneRenderTest) { // NOLINT
    Camera camera({0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, 0.2F, 0.5F, 1.94F);

    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<LightSource>> light_sources;

    light_sources.emplace_back(std::make_unique<PointLightSource>(vec3<float>{0.0F, 1.0F, 0.0F}, Color<float>{1.0F, 1.0F, 1.0F, 1.0F}));

    auto lambertian_bsdf = std::make_shared<LambertianBRDF>();
    auto glass_bsdf = std::make_shared<GlassBDF>();

    auto sphere1 = std::make_unique<Sphere>(vec3<float>{0.1F, 0.1F, 1.0F}, 0.5F);
    auto sphere1_material = std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.5F));
    sphere1->setMaterialHandler(std::make_shared<ConstantMaterialHandler>(sphere1_material, glass_bsdf));
    objects.emplace_back(std::move(sphere1));

    auto sphere2 = std::make_unique<Sphere>(vec3<float>{-0.1F, 0.2F, 2.0F}, 0.6F);
    auto sphere2_material = std::make_shared<ConstantMaterial>(Color<float>(0.8F, 0.4F, 0.6F, 1.0F), 1.0F, Spectrum(Color<float>{0.2F, 0.1F, 0.3F, 1.0F}));
    sphere2->setMaterialHandler(std::make_shared<ConstantMaterialHandler>(sphere2_material, lambertian_bsdf));
    objects.emplace_back(std::move(sphere2));

    auto ground = std::make_unique<Triangle>(vec3<float>{5.0F, -1.0F, 5.0F}, vec3<float>{0.0F, -1.0F, -5.0F}, vec3<float>{-5.0F, -1.0F, 5.0F});
    auto ground_material = std::make_shared<ConstantMaterial>(Color<float>(0.4F, 0.6F, 0.4F, 1.0F));
    ground->setMaterialHandler(std::make_shared<ConstantMaterialHandler>(ground_material, lambertian_bsdf));
    objects.emplace_back(std::move(ground));

    Scene scene(std::move(objects), std::move(light_sources));

    RenderOptions options{132, 68, 5, 10, 1E-3F};

    FrameRenderJob job{camera, scene, options};

    auto output_image = processJob(job);

    EXPECT_THAT(output_image(0, 0), testing::Eq(Color<float>{0.0F, 0.0F, 0.0F, 0.0F}));
    EXPECT_THAT(output_image(64, 32)[3], testing::Gt(0.0F));
}
